#include "mapper.h"

#ifdef INITIALPLACE
#include <thread>
#include <mutex>
#include <condition_variable>
#include <lemon/lp.h>

using namespace lemon;
// =========================================================================================
// InitialPlace: initial placement solved as an MIP, mixed integer linear program
// the initial placement is modelled as a Quadratic Assignment Problem
// by Lingling Lao in her mapping paper:
//  
// variables:
//     forall i: forall k: x[i][k], x[i][k] is integral and 0 or 1, meaning qubit i is in location k
// objective:
//     min z = sum i: sum j: sum k: sum l: refcount[i][j] * distance(k,l) * x[i][k] * x[j][l]
// subject to:
//     forall k: ( sum i: x[i][k] <= 1 )        allow more locations than qubits
//     forall i: ( sum k: x[i][k] == 1 )        but each qubit must have one locations
//  
// the article "An algorithm for the quadratic assignment problem using Benders' decomposition"
// by L. Kaufman and F. Broeckx, transforms this problem by introducing w[i][k] as follows:
//  
// forall i: forall k: w[i][k] =  x[i][k] * ( sum j: sum l: refcount[i][j] * distance(k,l) * x[j][l] )
//  
// to the following mixed integer linear problem:
//  
//  precompute:
//      forall i: forall k: costmax[i][k] = sum j: sum l: refcount[i][j] * distance(k,l)
//      (note: each of these costmax[][] is >= 0, so the "max(this,0)" around this is not needed)
//  variables:
//      forall i: forall k: x[i][k], x[i][k] is integral and 0 or 1
//      forall i: forall k: w[i][k], w[i][k] is real and >= 0
//  objective:
//      min z = sum i: sum k: w[i][k]
//  subject to:
//      forall k: ( sum i: x[i][k] <= 1 )
//      forall i: ( sum k: x[i][k] == 1 )
//      forall i: forall k: costmax[i][k] * x[i][k]
//          + ( sum j: sum l: refcount[i][j]*distance(k,l)*x[j][l] ) - w[i][k] <= costmax[i][k]
//
// This model is coded in lemon/mip below.
// The latter is mapped onto glpk.
//
// Since solving takes a while, two ways are offered to deal with this; these can be combined:
// 1. option initialplace2qhorizon: one of: 0,10,20,30,40,50,60,70,80,90,100
// The initialplace algorithm considers only this number of initial two-qubit gates to determine a mapping.
// When 0 is specified as option value, there is no limit.
// 2. option initialplace: an option steerable timeout mechanism around it is implemented, using threads:
// The solver runs in a subthread which can succeed or be timed out by the main thread waiting for it.
// When timed out, it can stop the compiler by raising an exception or continue mapping as if it were not called.
// When INITIALPLACE is not defined, the compiler doesn't contain initial placement support and ignores calls to it;
// then all this: lemon/mip, glpk, thread support is avoided making OpenQL much easier build and run.
// Otherwise, depending on the initialplace option value, initial placement is attempted before the heuristic.
// Options values of initialplace:
//  no      don't run initial placement ('ip')
//  yes     run ip until the solver is ready
//  1hx     run ip max for 1 hour; when timed out, stop the compiler
//  1h      run ip max for 1 hour; when timed out, just use heuristics
//  10mx    run ip max for 10 minutes; when timed out, stop the compiler
//  10m     run ip max for 10 minutes; when timed out, just use heuristics
//  1mx     run ip max for 1 minute; when timed out, stop the compiler
//  1m      run ip max for 1 minute; when timed out, just use heuristics
//  10sx    run ip max for 10 seconds; when timed out, stop the compiler
//  10s     run ip max for 10 seconds; when timed out, just use heuristics
//  1sx     run ip max for 1 second; when timed out, stop the compiler
//  1s      run ip max for 1 second; when timed out, just use heuristics

typedef
enum InitialPlaceResults
{
    ipr_any,            // any mapping will do because there are no two-qubit gates in the circuit
    ipr_current,        // current mapping will do because all two-qubit gates are NN
    ipr_newmap,         // initial placement solution found a mapping
    ipr_failed,         // initial placement solution failed
    ipr_timedout        // initial placement solution timed out and thus failed
} ipr_t;

class InitialPlace
{
private:
                                        // parameters, constant for a kernel
    const ql::quantum_platform   *platformp;  // platform
    size_t                  nlocs;      // number of locations, real qubits; index variables k and l
    size_t                  nvq;        // same range as nlocs; when not, take set from config and create v2i earlier
    Grid                   *gridp;      // current grid with Distance function

                                        // remaining attributes are computed per circuit
    size_t                  nfac;       // number of facilities, actually used virtual qubits; index variables i and j
                                        // nfac <= nlocs: e.g. nlocs == 7, but only v2 and v5 are used; nfac then is 2

public:

std::string ipr2string(ipr_t ipr)
{
    switch(ipr)
    {
    case ipr_any:       return "any";
    case ipr_current:   return "current";
    case ipr_newmap:    return "newmap";
    case ipr_failed:    return "failed";
    case ipr_timedout:  return "timedout";
    }
    return "unknown";
}

// kernel-once initialization
void Init(Grid* g, const ql::quantum_platform *p)
{
    // DOUT("InitialPlace Init ...");
    platformp = p;
    nlocs = p->qubit_number;
    nvq = p->qubit_number;  // same range; when not, take set from config and create v2i earlier
    // DOUT("... number of real qubits (locations): " << nlocs);
    gridp = g;
    DOUT("Init: platformp=" << platformp << " nlocs=" << nlocs << " nvq=" << nvq << " gridp=" << gridp);
}

// find an initial placement of the virtual qubits for the given circuit
// the resulting placement is put in the provided virt2real map
// result indicates one of the result indicators (ipr_t, see above)
void PlaceBody( ql::circuit& circ, Virt2Real& v2r, ipr_t &result, double& iptimetaken)
{
    DOUT("InitialPlace.PlaceBody ...");

    // check validity of circuit
    for ( auto& gp : circ )
    {
        auto&   q = gp->operands;
        if (q.size() > 2)
        {
            FATAL(" gate: " << gp->qasm() << " has more than 2 operand qubits; please decompose such gates first before mapping.");
        }
    }

    // only consider first number of two-qubit gates as specified by option initialplace2qhorizon
    // this influences refcount (so constraints) and nfac (number of facilities, so size of MIP problem)
    std::string initialplace2qhorizonopt = ql::options::get("initialplace2qhorizon");
    int  prefix = stoi(initialplace2qhorizonopt);

    // compute ipusecount[] to know which virtual qubits are actually used
    // use it to compute v2i, mapping (non-contiguous) virtual qubit indices to contiguous facility indices
    // (the MIP model is shorter when the indices are contiguous)
    // finally, nfac is set to the number of these facilities;
    // only consider virtual qubit uses until the specified max number of two qubit gates has been seen
    DOUT("... compute ipusecount by scanning circuit");
    std::vector<size_t>  ipusecount;// ipusecount[v] = count of use of virtual qubit v in current circuit
    ipusecount.resize(nvq,0);       // initially all 0
    std::vector<size_t> v2i;        // v2i[virtual qubit index v] -> index of facility i
    v2i.resize(nvq,UNDEFINED_QUBIT);// virtual qubit v not used by circuit as gate operand

    int  twoqubitcount = 0;
    for ( auto& gp : circ )
    {
        if (prefix == 0 || twoqubitcount < prefix)
        {
            for ( auto v : gp->operands)
            {
                ipusecount[v] += 1;
            }
        }
        if (gp->operands.size() == 2)
        {
            twoqubitcount++;
        }
    }
    nfac = 0;
    for (size_t v=0; v < nvq; v++)
    {
        if (ipusecount[v] != 0)
        {
            v2i[v] = nfac;
            nfac += 1;
        }
    }
    DOUT("... number of facilities: " << nfac << " while number of used virtual qubits is: " << nvq);

    // precompute refcount (used by the model as constants) by scanning circuit;
    // refcount[i][j] = count of two-qubit gates between facilities i and j in current circuit
    // at the same time, set anymap and currmap
    // anymap = there are no two-qubit gates so any map will do
    // currmap = in the current map, all two-qubit gates are NN so current map will do
    DOUT("... compute refcount by scanning circuit");
    std::vector<std::vector<size_t>>  refcount;
    refcount.resize(nfac); for (size_t i=0; i<nfac; i++) refcount[i].resize(nfac,0);
    bool anymap = true;    // true when all refcounts are 0
    bool currmap = true;   // true when in current map all two-qubit gates are NN
    
    twoqubitcount = 0;
    for ( auto& gp : circ )
    {
        auto&   q = gp->operands;
        if (q.size() == 2)
        {
            if (prefix == 0 || twoqubitcount < prefix)
            {
                anymap = false;
                refcount[v2i[q[0]]][v2i[q[1]]] += 1;
                
                if (v2r[q[0]] == UNDEFINED_QUBIT
                    || v2r[q[1]] == UNDEFINED_QUBIT
                    || gridp->Distance(v2r[q[0]], v2r[q[1]]) > 1
                    )
                {
                    currmap = false;
                }
            }
            twoqubitcount++;
        }
    }
    if (prefix != 0 && twoqubitcount >= prefix)
    {
        DOUT("InitialPlace: only considered " << prefix << " of " << twoqubitcount << " two-qubit gates, so resulting mapping is not exact");
    }
    if (anymap)
    {
        DOUT("InitialPlace: no two-qubit gates found, so no constraints, and any mapping is ok");
        DOUT("InitialPlace.PlaceBody [ANY MAPPING IS OK]");
        result = ipr_any;
        iptimetaken = 0.0;
        return;
    }
    if (currmap)
    {
        DOUT("InitialPlace: in current map, all two-qubit gates are nearest neighbor, so current map is ok");
        DOUT("InitialPlace.PlaceBody [CURRENT MAPPING IS OK]");
        result = ipr_current;
        iptimetaken = 0.0;
        return;
    }

    // compute iptimetaken, start interval timer here
    using namespace std::chrono;
    high_resolution_clock::time_point t1 = high_resolution_clock::now();

    // precompute costmax by applying formula
    // costmax[i][k] = sum j: sum l: refcount[i][j] * distance(k,l) for facility i in location k
    DOUT("... precompute costmax by combining refcount and distances");
    std::vector<std::vector<size_t>>  costmax;   
    costmax.resize(nfac); for (size_t i=0; i<nfac; i++) costmax[i].resize(nlocs,0);
    for ( size_t i=0; i<nfac; i++ )
    {
        for ( size_t k=0; k<nlocs; k++ )
        {
            for ( size_t j=0; j<nfac; j++ )
            {
                for ( size_t l=0; l<nlocs; l++ )
                {
                    costmax[i][k] += refcount[i][j] * (gridp->Distance(k,l) - 1);
                }
            }
        }
    }
    
    // the problem
    // mixed integer programming
    Mip  mip;
    
    // variables (columns)
    //  x[i][k] are integral, values 0 or 1
    //      x[i][k] represents whether facility i is in location k
    //  w[i][k] are real, values >= 0
    //      w[i][k] represents x[i][k] * sum j: sum l: refcount[i][j] * distance(k,l) * x[j][l]
    //       i.e. if facility i not in location k then 0
    //       else for all facilities j in its location l sum refcount[i][j] * distance(k,l)
    // DOUT("... allocate x column variable");
    std::vector<std::vector<Mip::Col>> x;
        x.resize(nfac); for (size_t i=0; i<nfac; i++) x[i].resize(nlocs);
    // DOUT("... allocate w column variable");
    std::vector<std::vector<Mip::Col>> w;
        w.resize(nfac); for (size_t i=0; i<nfac; i++) w[i].resize(nlocs);
    // DOUT("... add/initialize x and w column variables with trivial constraints and type");
    for ( size_t i=0; i<nfac; i++ )
    {
        for ( size_t k=0; k<nlocs; k++ )
        {
            x[i][k] = mip.addCol();
            mip.colLowerBound(x[i][k], 0);          // 0 <= x[i][k]
            mip.colUpperBound(x[i][k], 1);          //      x[i][k] <= 1
            mip.colType(x[i][k], Mip::INTEGER);     // int
            // DOUT("x[" << i << "][" << k << "] INTEGER >= 0 and <= 1");
    
            w[i][k] = mip.addCol();
            mip.colLowerBound(w[i][k], 0);          // 0 <= w[i][k]
            mip.colType(w[i][k], Mip::REAL);        // real
            // DOUT("w[" << i << "][" << k << "] REAL >= 0");
        }
    }
    
    // constraints (rows)
    //  forall i: ( sum k: x[i][k] == 1 )
    // DOUT("... add/initialize sum to 1 constraint rows");
    for ( size_t i=0; i<nfac; i++ )
    {
        Mip::Expr   sum;
        std::string s = "";
        bool started = false;
        for ( size_t k=0; k<nlocs; k++ )
        {
            sum += x[i][k];
            if (started) s += "+ "; else started = true;
            s += "x[";
            s += std::to_string(i);
            s += "][";
            s += std::to_string(k);
            s += "]";
        }
        mip.addRow(sum == 1);
        s += " == 1";
        // DOUT(s);
    }
    
    // constraints (rows)
    //  forall k: ( sum i: x[i][k] <= 1 )
    //  < 1 (i.e. == 0) may apply for a k when location k doesn't contain a qubit in this solution
    for ( size_t k=0; k<nlocs; k++ )
    {
        Mip::Expr   sum;
        std::string s = "";
        bool started = false;
        for ( size_t i=0; i<nfac; i++ )
        {
            sum += x[i][k];
            if (started) s += "+ "; else started = true;
            s += "x[";
            s += std::to_string(i);
            s += "]["; 
            s += std::to_string(k); 
            s += "]";
        }
        mip.addRow(sum <= 1);
        s += " <= 1";
        // DOUT(s);
    }
    
    // constraints (rows)
    //  forall i, k: costmax[i][k] * x[i][k]
    //          + sum j sum l refcount[i][j]*distance[k][l]*x[j][l] - w[i][k] <= costmax[i][k]
    // DOUT("... add/initialize nfac x nlocs constraint rows based on nfac x nlocs column combinations");
    for ( size_t i=0; i<nfac; i++ )
    {
        for ( size_t k=0; k<nlocs; k++ )
        {
            Mip::Expr   left = costmax[i][k] * x[i][k];
            std::string lefts = "";
            bool started = false;
            for ( size_t j=0; j<nfac; j++ )
            {
                for ( size_t l=0; l<nlocs; l++ )
                {
                    left += refcount[i][j] * gridp->Distance(k,l) * x[j][l];
                    if (refcount[i][j] * gridp->Distance(k,l) != 0)
                    {
                        if (started) lefts += " + "; else started = true;
                        lefts += std::to_string(refcount[i][j] * gridp->Distance(k,l));
                        lefts += " * x[";
                        lefts += std::to_string(j);
                        lefts += "][";
                        lefts += std::to_string(l);
                        lefts += "]";
                    }
                }
            }
            left -= w[i][k];
            lefts += "- w[";
            lefts += std::to_string(i);
            lefts += "][";
            lefts += std::to_string(k);
            lefts += "]";
            Mip::Expr   right = costmax[i][k];
            mip.addRow(left <= right);
            // DOUT(lefts << " <= " << costmax[i][k]);
        }
    }
    
    // objective
    Mip::Expr   objective;
    // DOUT("... add/initialize objective");
    std::string objs = "";
    bool started = false;
    mip.min();
    for ( size_t i=0; i<nfac; i++ )
    {
        for ( size_t k=0; k<nlocs; k++ )
        {
            objective += w[i][k];
            if (started) objs += "+ "; else started = true;
            objs += "w[";
            objs += std::to_string(i);
            objs += "][";
            objs += std::to_string(k);
            objs += "]";
        }
    }
    mip.obj(objective);
    // DOUT("MINIMIZE " << objs);

    DOUT("... v2r before solving, nvq=" << nvq);
    for (size_t v=0; v<nvq; v++)
    {
        DOUT("... about to print v2r[" << v << "]= ...");
        DOUT("....." << v2r[v]);
    }
    DOUT("..1 nvq=" << nvq);
    
    // solve the problem
    WOUT("... computing initial placement using MIP, this may take a while ...");
    DOUT("InitialPlace: solving the problem, this may take a while ...");
    DOUT("..2 nvq=" << nvq);
    Mip::SolveExitStatus s;
    DOUT("Just before solve: platformp=" << platformp << " nlocs=" << nlocs << " nvq=" << nvq << " gridp=" << gridp);
    DOUT("Just before solve: objs=" << objs << " x.size()=" << x.size() << " w.size()=" << w.size() << " refcount.size()=" << refcount.size() << " v2i.size()=" << v2i.size() << " ipusecount.size()=" << ipusecount.size());
    DOUT("..2b nvq=" << nvq);
    {
        s = mip.solve();
    }
    DOUT("..3 nvq=" << nvq);
    DOUT("Just after solve: platformp=" << platformp << " nlocs=" << nlocs << " nvq=" << nvq << " gridp=" << gridp);
    DOUT("Just after solve: objs=" << objs << " x.size()=" << x.size() << " w.size()=" << w.size() << " refcount.size()=" << refcount.size() << " v2i.size()=" << v2i.size() << " ipusecount.size()=" << ipusecount.size());
    MapperAssert(nvq == nlocs);         // consistency check, mainly to let it crash

    // computing iptimetaken, stop interval timer
    DOUT("..4 nvq=" << nvq);
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    duration<double> time_span = t2 - t1;
    iptimetaken = time_span.count();
    DOUT("..5 nvq=" << nvq);

    // DOUT("... determine result of solving");
    Mip::ProblemType pt = mip.type();
    DOUT("..6 nvq=" << nvq);
    if (s != Mip::SOLVED || pt != Mip::OPTIMAL)
    {
        DOUT("... InitialPlace: no (optimal) solution found; solve returned:"<< s << " type returned:" << pt);
        result = ipr_failed;
        DOUT("InitialPlace.PlaceBody [FAILED, DID NOT FIND MAPPING]");
        return;
    }
    DOUT("..7 nvq=" << nvq);

    // return new mapping as result in v2r

    // get the results: x[i][k] == 1 iff facility i is in location k (i.e. real qubit index k)
    // use v2i to translate facilities back to original virtual qubit indices
    // and fill v2r with the found locations for the used virtual qubits;
    // the unused mapped virtual qubits are mapped to an arbitrary permutation of the remaining locations;
    // the latter must be updated to generate swaps when mapping multiple kernels
    DOUT("..8 nvq=" << nvq);
    DOUT("... interpret result and copy to Virt2Real, nvq=" << nvq);
    for (size_t v=0; v<nvq; v++)
    {
        DOUT("... about to set v2r to undefined for v " << v);
        v2r[v] = UNDEFINED_QUBIT;      // i.e. undefined, i.e. v is not an index of a used virtual qubit
    }
    for ( size_t i=0; i<nfac; i++ )
    {
        size_t v;   // found virtual qubit index v represented by facility i
        // use v2i backward to find virtual qubit v represented by facility i
        DOUT("... about to inspect v2i to get solution and set it in v2r for facility " << i);
        for (v=0; v<nvq; v++)
        {
            if (v2i[v] == i)
            {
                break;
            }
        }
        MapperAssert(v < nvq);  // for each facility there must be a virtual qubit
        size_t k;   // location to which facility i being virtual qubit index v was allocated
        for (k=0; k<nlocs; k++ )
        {
            if (mip.sol(x[i][k]) == 1)
            {
                v2r[v] = k;
                // v2r.rs[] is not updated because no gates were really mapped yet
                break;
            }
        }
        MapperAssert(k < nlocs);  // each facility i by definition represents a used qubit so must have got a location
        DOUT("... end loop body over nfac");
    }

    auto mapinitone2oneopt = ql::options::get("mapinitone2one");
    if ("yes" == mapinitone2oneopt)
    {
        DOUT("... correct location of unused mapped virtual qubits to be an unused location");
        v2r.DPRINT("... result Virt2Real map of InitialPlace before mapping unused mapped virtual qubits ");
        // virtual qubits used by this kernel v have got their location k filled in in v2r[v] == k
        // unused mapped virtual qubits still have location UNDEFINED_QUBIT, fill with the remaining locs
        // this should be replaced by actually swapping them to there, when mapping multiple kernels
        for (size_t v=0; v<nvq; v++)
        {
            if (v2r[v] == UNDEFINED_QUBIT)
            {
                // v is unused by this kernel; find an unused location k
                size_t k;   // location k that is checked for having been allocated to some virtual qubit w
                for (k=0; k<nlocs; k++ )
                {
                    size_t w;
                    for (w=0; w<nvq; w++)
                    {
                        if (v2r[w] == k)
                        {
                            break;
                        }
                    }
                    if (w >= nvq)
                    {
                        // no w found for which v2r[w] == k
                        break;     // k is an unused location
                    }
                    // k is a used location, so continue with next k to check whether it is hopefully unused
                }
                MapperAssert(k < nlocs);  // when a virtual qubit is not used, there must be a location that is not used
                v2r[v] = k;
            }
            DOUT("... end loop body over nvq when mapinitone2oneopt");
        }
    }
    v2r.DPRINT("... final result Virt2Real map of InitialPlace");
    result = ipr_newmap;
    DOUT("InitialPlace.PlaceBody [SUCCESS, FOUND MAPPING]");
}

// the above PlaceBody is a regular function using circ, and updating v2r and result before it returns;
// it implements Initial Placement as if the call to Place in the mapper called PlaceBody directly;
// because it may take a while to return, a new Place and a PlaceWrapper are put in between;
// the idea is to run PlaceBody in a detached thread, that, when ready, signals the main thread;
// the main thread waits for this signal with a timeout value;
// all this is done in a try block where the catch is called on this timeout;
// why exceptions are used, is not clear, so it was replaced by PlaceWrapper returning "timedout" or not
// and this works as well ...
bool PlaceWrapper( ql::circuit& circ, Virt2Real& v2r, ipr_t& result, double& iptimetaken, std::string& initialplaceopt)
{
    DOUT("InitialPlace.PlaceWrapper called");
    std::mutex  m;
    std::condition_variable cv;

    // prepare timeout
    int      waitseconds;
    bool     andthrowexception = false;
    if ("1s" == initialplaceopt)        { waitseconds = 1; }
    else if ("1sx" == initialplaceopt)  { waitseconds = 1; andthrowexception = true; }
    else if ("10s" == initialplaceopt)  { waitseconds = 10; }
    else if ("10sx" == initialplaceopt) { waitseconds = 10; andthrowexception = true; }
    else if ("1m" == initialplaceopt)   { waitseconds = 60; }
    else if ("1mx" == initialplaceopt)  { waitseconds = 60; andthrowexception = true; }
    else if ("10m" == initialplaceopt)  { waitseconds = 600; }
    else if ("10mx" == initialplaceopt) { waitseconds = 600; andthrowexception = true; }
    else if ("1h" == initialplaceopt)   { waitseconds = 3600; }
    else if ("1hx" == initialplaceopt)  { waitseconds = 3600; andthrowexception = true; }
    else
    {
        FATAL("Unknown value of option 'initialplace'='" << initialplaceopt << "'.");
    }
    iptimetaken = waitseconds;    // pessimistic, in case of timeout, otherwise it is corrected

    // v2r and result are allocated on stack of main thread by some ancestor so be careful with threading
    std::thread t([&cv, this, &circ, &v2r, &result, &iptimetaken]()
        {
            DOUT("InitialPlace.PlaceWrapper subthread about to call PlaceBody");
            PlaceBody(circ, v2r, result, iptimetaken);
            DOUT("InitialPlace.PlaceBody returned in subthread; about to signal the main thread");
            cv.notify_one();        // by this, the main thread awakes from cv.wait_for without timeout
            DOUT("InitialPlace.PlaceWrapper subthread after signaling the main thread, and is about to die");
        }
    );
    DOUT("InitialPlace.PlaceWrapper main code created thread; about to call detach on it");
    t.detach();
    DOUT("InitialPlace.PlaceWrapper main code detached thread");
    {
        std::chrono::seconds maxwaittime(waitseconds);
        std::unique_lock<std::mutex> l(m);
        DOUT("InitialPlace.PlaceWrapper main code starts waiting with timeout of " << waitseconds << " seconds");
        if (cv.wait_for(l, maxwaittime) == std::cv_status::timeout)
        {
            DOUT("InitialPlace.PlaceWrapper main code awoke from waiting with timeout");
            if (andthrowexception)
            {
                DOUT("InitialPlace: timed out and stops compilation [TIMED OUT, STOP COMPILATION]");
                FATAL("Initial placement timed out and stops compilation [TIMED OUT, STOP COMPILATION]");
            }
            DOUT("InitialPlace.PlaceWrapper about to return timedout==true");
            return true;
        }
        DOUT("InitialPlace.PlaceWrapper main code awoke from waiting without timeout, from signal sent by InitialPlace.PlaceWrapper subthread just before its death");
    }

    DOUT("InitialPlace.PlaceWrapper about to return timedout==false");
    return false;
}

// find an initial placement of the virtual qubits for the given circuit as in Place
// put a timelimit on its execution specified by the initialplace option
// when it expires, result is set to ipr_timedout;
// details of how this is accomplished, can be found above;
// v2r is updated by PlaceBody/PlaceWrapper when it has found a mapping
void Place( ql::circuit& circ, Virt2Real& v2r, ipr_t& result, double& iptimetaken, std::string& initialplaceopt)
{
    Virt2Real   v2r_orig = v2r;

    DOUT("InitialPlace.Place ...");
    if ("yes" == initialplaceopt)
    {
        // do initial placement without time limit
        DOUT("InitialPlace.Place calling PlaceBody without time limit");
        PlaceBody(circ, v2r, result, iptimetaken);
        // v2r reflects new mapping, if any found, otherwise unchanged
        DOUT("InitialPlace.Place [done, no time limit], result=" << result << " iptimetaken=" << iptimetaken << " seconds");
    }
    else
    {
        bool timedout;
        timedout = PlaceWrapper(circ, v2r, result, iptimetaken, initialplaceopt);
    
        if (timedout)
        {
            result = ipr_timedout;
            DOUT("InitialPlace.Place [done, TIMED OUT, NO MAPPING FOUND], result=" << result << " iptimetaken=" << iptimetaken << " seconds");

            v2r = v2r_orig; // v2r may have got corrupted when timed out during v2r updating
        }
        else
        {
            // v2r reflects new mapping, if any found, otherwise unchanged
            DOUT("InitialPlace.Place [done, not timed out], result=" << result << " iptimetaken=" << iptimetaken << " seconds");
        }
    }
}
    
};  // end class InitialPlace
#endif // INITIALPLACE

// map kernel's circuit, main mapper entry once per kernel
void Mapper::Map(ql::quantum_kernel& kernel)
{
    COUT("Mapping kernel " << kernel.name << " [START]");
    DOUT("... kernel original virtual number of qubits=" << kernel.qubit_count);
    nc = kernel.creg_count;     // in absence of platform creg_count, take it from kernel, i.e. from OpenQL program
    kernelp = NULL;             // no new_gates until kernel.c has been copied

    Virt2Real   v2r;            // current mapping while mapping this kernel

    // unify all incoming v2rs into v2r to compute kernel input mapping;
    // but until inter-kernel mapping is implemented, take program initial mapping for it
    v2r.Init(nq);               // v2r now contains program initial mapping
    v2r.DPRINT("After initialization");

    v2r.Export(v2r_in);  // from v2r to caller for reporting
    v2r.Export(rs_in);   // from v2r to caller for reporting

    std::string initialplaceopt = ql::options::get("initialplace");
    if("no" != initialplaceopt)
    {
#ifdef INITIALPLACE
        std::string initialplace2qhorizonopt = ql::options::get("initialplace2qhorizon");
        DOUT("InitialPlace: kernel=" << kernel.name << " initialplace=" << initialplaceopt << " initialplace2qhorizon=" << initialplace2qhorizonopt << " [START]");
        InitialPlace    ip;             // initial placer facility
        ipr_t           ipok;           // one of several ip result possibilities
        double          iptimetaken;      // time solving the initial placement took, in seconds
        
        ip.Init(&grid, platformp);
        ip.Place(kernel.c, v2r, ipok, iptimetaken, initialplaceopt); // compute mapping (in v2r) using ip model, may fail
        DOUT("InitialPlace: kernel=" << kernel.name << " initialplace=" << initialplaceopt << " initialplace2qhorizon=" << initialplace2qhorizonopt << " result=" << ip.ipr2string(ipok) << " iptimetaken=" << iptimetaken << " seconds [DONE]");
#else // ifdef INITIALPLACE
        DOUT("InitialPlace support disabled during OpenQL build [DONE]");
        WOUT("InitialPlace support disabled during OpenQL build [DONE]");
#endif // ifdef INITIALPLACE
    }
    v2r.DPRINT("After InitialPlace");

    v2r.Export(v2r_ip);  // from v2r to caller for reporting
    v2r.Export(rs_ip);   // from v2r to caller for reporting

    MapCircuit(kernel, v2r);        // updates kernel.c with swaps, maps all gates, updates v2r map
    v2r.DPRINT("After heuristics");

    MakePrimitives(kernel);         // decompose to primitives as specified in the config file

    kernel.qubit_count = nq;        // bluntly copy nq (==#real qubits), so that all kernels get the same qubit_count
    v2r.Export(v2r_out);     // from v2r to caller for reporting
    v2r.Export(rs_out);      // from v2r to caller for reporting

    COUT("Mapping kernel " << kernel.name << " [DONE]");
}   // end Map
