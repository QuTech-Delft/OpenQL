/** \file
 * Initial placement engine.
 */

#include "ql/pass/map/qubits/initial_placement_algo.h"

#ifdef INITIALPLACE

#include <thread>
#include <mutex>
#include <condition_variable>
#include <lemon/lp.h>

// uncomment next line to enable multi-line dumping
// #define MULTI_LINE_LOG_DEBUG

namespace ql {
namespace pass {
namespace map {
namespace qubits {

using namespace lemon;
using namespace utils;

/**
 * String conversion for initial placement results.
 */
std::ostream &operator<<(std::ostream &os, Result ipr) {
    switch (ipr) {
        case Result::ANY:       os << "any";        break;
        case Result::CURRENT:   os << "current";    break;
        case Result::NEW_MAP:   os << "newmap";     break;
        case Result::FAILED:    os << "failed";     break;
        case Result::TIMED_OUT: os << "timedout";   break;
    }
    return os;
}

// find an initial placement of the virtual qubits for the given circuit
// the resulting placement is put in the provided virt2real map
// result indicates one of the result indicators (InitialPlaceResult, see above)
Result InitialPlacementAlgo::body(com::map::QubitMapping &v2r) {
    QL_DOUT("InitialPlace.body ...");

    // check validity of circuit
    for (auto &gp : kernel->gates) {
        auto &q = gp->operands;
        if (q.size() > 2) {
            QL_FATAL(" gate: " << gp->qasm() << " has more than 2 operand qubits; please decompose such gates first before mapping.");
        }
    }

    // only consider first number of two-qubit gates as specified by option initialplace2qhorizon
    // this influences refcount (so constraints) and nfac (number of facilities, so size of MIP problem)

    // compute ipusecount[] to know which virtual qubits are actually used
    // use it to compute v2i, mapping (non-contiguous) virtual qubit indices to contiguous facility indices
    // (the MIP model is shorter when the indices are contiguous)
    // finally, nfac is set to the number of these facilities;
    // only consider virtual qubit uses until the specified max number of two qubit gates has been seen
    QL_DOUT("... compute ipusecount by scanning circuit");
    Vec<UInt>  ipusecount;// ipusecount[v] = count of use of virtual qubit v in current circuit
    ipusecount.resize(nvq,0);       // initially all 0
    Vec<UInt> v2i;        // v2i[virtual qubit index v] -> index of facility i
    v2i.resize(nvq, com::map::UNDEFINED_QUBIT);// virtual qubit v not used by circuit as gate operand

    UInt twoqubitcount = 0;
    for (auto &gp : kernel->gates) {
        if (options.horizon == 0 || twoqubitcount < options.horizon) {
            for (auto v : gp->operands) {
                ipusecount[v] += 1;
            }
        }
        if (gp->operands.size() == 2) {
            twoqubitcount++;
        }
    }
    nfac = 0;
    for (UInt v=0; v < nvq; v++) {
        if (ipusecount[v] != 0) {
            v2i[v] = nfac;
            nfac += 1;
        }
    }
    QL_DOUT("... number of facilities: " << nfac << " while number of used virtual qubits is: " << nvq);

    // precompute refcount (used by the model as constants) by scanning circuit;
    // refcount[i][j] = count of two-qubit gates between facilities i and j in current circuit
    // at the same time, set anymap and currmap
    // anymap = there are no two-qubit gates so any map will do
    // currmap = in the current map, all two-qubit gates are NN so current map will do
    QL_DOUT("... compute refcount by scanning circuit");
    Vec<Vec<UInt>>  refcount;
    refcount.resize(nfac); for (UInt i=0; i<nfac; i++) refcount[i].resize(nfac,0);
    Bool anymap = true;    // true when all refcounts are 0
    Bool currmap = true;   // true when in current map all two-qubit gates are NN

    twoqubitcount = 0;
    for (auto &gp : kernel->gates) {
        auto &q = gp->operands;
        if (q.size() == 2) {
            if (options.horizon == 0 || twoqubitcount < options.horizon) {
                anymap = false;
                refcount[v2i[q[0]]][v2i[q[1]]] += 1;

                if (
                    v2r[q[0]] == com::map::UNDEFINED_QUBIT
                    || v2r[q[1]] == com::map::UNDEFINED_QUBIT
                    || platform->topology->get_distance(v2r[q[0]], v2r[q[1]]) > 1
                ) {
                    currmap = false;
                }
            }
            twoqubitcount++;
        }
    }
    if (options.horizon != 0 && twoqubitcount >= options.horizon) {
        QL_DOUT("InitialPlace: only considered " << options.horizon << " of " << twoqubitcount << " two-qubit gates, so resulting mapping is not exact");
    }
    if (anymap) {
        QL_DOUT("InitialPlace: no two-qubit gates found, so no constraints, and any mapping is ok");
        QL_DOUT("InitialPlace.body [ANY MAPPING IS OK]");
        time_taken = 0.0;
        return Result::ANY;
    }
    if (currmap) {
        QL_DOUT("InitialPlace: in current map, all two-qubit gates are nearest neighbor, so current map is ok");
        QL_DOUT("InitialPlace.body [CURRENT MAPPING IS OK]");
        time_taken = 0.0;
        return Result::CURRENT;
    }

    // compute iptimetaken, start interval timer here
    using namespace std::chrono;
    high_resolution_clock::time_point t1 = high_resolution_clock::now();

    // precompute costmax by applying formula
    // costmax[i][k] = sum j: sum l: refcount[i][j] * distance(k,l) for facility i in location k
    QL_DOUT("... precompute costmax by combining refcount and distances");
    Vec<Vec<UInt>>  costmax;
    costmax.resize(nfac); for (UInt i=0; i<nfac; i++) costmax[i].resize(nlocs,0);
    for (UInt i = 0; i < nfac; i++) {
        for (UInt k = 0; k < nlocs; k++) {
            for (UInt j = 0; j < nfac; j++) {
                for (UInt l = 0; l < nlocs; l++) {
                    costmax[i][k] += refcount[i][j] * (platform->topology->get_distance(k, l) - 1);
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
    // QL_DOUT("... allocate x column variable");
    Vec<Vec<Mip::Col>> x;
    x.resize(nfac); for (UInt i=0; i<nfac; i++) x[i].resize(nlocs);
    // QL_DOUT("... allocate w column variable");
    Vec<Vec<Mip::Col>> w;
    w.resize(nfac); for (UInt i=0; i<nfac; i++) w[i].resize(nlocs);
    // QL_DOUT("... add/initialize x and w column variables with trivial constraints and type");
    for (UInt i = 0; i < nfac; i++) {
        for (UInt k = 0; k < nlocs; k++) {
            x[i][k] = mip.addCol();
            mip.colLowerBound(x[i][k], 0);          // 0 <= x[i][k]
            mip.colUpperBound(x[i][k], 1);          //      x[i][k] <= 1
            mip.colType(x[i][k], Mip::INTEGER);     // Int
            // QL_DOUT("x[" << i << "][" << k << "] INTEGER >= 0 and <= 1");

            w[i][k] = mip.addCol();
            mip.colLowerBound(w[i][k], 0);          // 0 <= w[i][k]
            mip.colType(w[i][k], Mip::REAL);        // real
            // QL_DOUT("w[" << i << "][" << k << "] REAL >= 0");
        }
    }

    // constraints (rows)
    //  forall i: ( sum k: x[i][k] == 1 )
    // QL_DOUT("... add/initialize sum to 1 constraint rows");
    for (UInt i = 0; i < nfac; i++) {
        Mip::Expr   sum;
        Str s{};
        Bool started = false;
        for (UInt k = 0; k < nlocs; k++) {
            sum += x[i][k];
            if (started) {
                s += "+ ";
            } else {
                started = true;
            }
            s += "x[";
            s += to_string(i);
            s += "][";
            s += to_string(k);
            s += "]";
        }
        mip.addRow(sum == 1);
        s += " == 1";
        // QL_DOUT(s);
    }

    // constraints (rows)
    //  forall k: ( sum i: x[i][k] <= 1 )
    //  < 1 (i.e. == 0) may apply for a k when location k doesn't contain a qubit in this solution
    for (UInt k = 0; k < nlocs; k++) {
        Mip::Expr   sum;
        Str s{};
        Bool started = false;
        for (UInt i = 0; i < nfac; i++) {
            sum += x[i][k];
            if (started) s += "+ "; else started = true;
            s += "x[";
            s += to_string(i);
            s += "][";
            s += to_string(k);
            s += "]";
        }
        mip.addRow(sum <= 1);
        s += " <= 1";
        // QL_DOUT(s);
    }

    // constraints (rows)
    //  forall i, k: costmax[i][k] * x[i][k]
    //          + sum j sum l refcount[i][j]*distance[k][l]*x[j][l] - w[i][k] <= costmax[i][k]
    // QL_DOUT("... add/initialize nfac x nlocs constraint rows based on nfac x nlocs column combinations");
    for (UInt i = 0; i < nfac; i++) {
        for (UInt k = 0; k < nlocs; k++) {
            Mip::Expr   left = costmax[i][k] * x[i][k];
            Str lefts{};
            Bool started = false;
            for (UInt j = 0; j < nfac; j++) {
                for (UInt l = 0; l < nlocs; l++) {
                    left += refcount[i][j] * platform->topology->get_distance(k, l) * x[j][l];
                    if (refcount[i][j] * platform->topology->get_distance(k, l) != 0) {
                        if (started) {
                            lefts += " + ";
                        } else {
                            started = true;
                        }
                        lefts += to_string(refcount[i][j] * platform->topology->get_distance(k, l));
                        lefts += " * x[";
                        lefts += to_string(j);
                        lefts += "][";
                        lefts += to_string(l);
                        lefts += "]";
                    }
                }
            }
            left -= w[i][k];
            lefts += "- w[";
            lefts += to_string(i);
            lefts += "][";
            lefts += to_string(k);
            lefts += "]";
            Mip::Expr   right = costmax[i][k];
            mip.addRow(left <= right);
            // QL_DOUT(lefts << " <= " << costmax[i][k]);
        }
    }

    // objective
    Mip::Expr   objective;
    // QL_DOUT("... add/initialize objective");
    Str objs{};
    Bool started = false;
    mip.min();
    for (UInt i = 0; i < nfac; i++) {
        for (UInt k = 0; k < nlocs; k++) {
            objective += w[i][k];
            if (started) {
                objs += "+ ";
            } else {
                started = true;
            }
            objs += "w[";
            objs += to_string(i);
            objs += "][";
            objs += to_string(k);
            objs += "]";
        }
    }
    mip.obj(objective);
    // QL_DOUT("MINIMIZE " << objs);

    QL_DOUT("... v2r before solving, nvq=" << nvq);
    for (UInt v = 0; v < nvq; v++) {
        QL_DOUT("... about to print v2r[" << v << "]= ...");
        QL_DOUT("....." << v2r[v]);
    }
    QL_DOUT("..1 nvq=" << nvq);

    // solve the problem
    QL_WOUT("... computing initial placement using MIP, this may take a while ...");
    QL_DOUT("InitialPlace: solving the problem, this may take a while ...");
    QL_DOUT("..2 nvq=" << nvq);
    Mip::SolveExitStatus s;
    QL_DOUT("Just before solve: platformp=" << platform.get_ptr() << " nlocs=" << nlocs << " nvq=" << nvq);
    QL_DOUT("Just before solve: objs=" << objs << " x.size()=" << x.size() << " w.size()=" << w.size() << " refcount.size()=" << refcount.size() << " v2i.size()=" << v2i.size() << " ipusecount.size()=" << ipusecount.size());
    QL_DOUT("..2b nvq=" << nvq);
    {
        s = mip.solve();
    }
    QL_DOUT("..3 nvq=" << nvq);
    QL_DOUT("Just after solve: platformp=" << platform.get_ptr() << " nlocs=" << nlocs << " nvq=" << nvq);
    QL_DOUT("Just after solve: objs=" << objs << " x.size()=" << x.size() << " w.size()=" << w.size() << " refcount.size()=" << refcount.size() << " v2i.size()=" << v2i.size() << " ipusecount.size()=" << ipusecount.size());
    QL_ASSERT(nvq == nlocs);         // consistency check, mainly to let it crash

    // computing iptimetaken, stop interval timer
    QL_DOUT("..4 nvq=" << nvq);
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    duration<Real> time_span = t2 - t1;
    time_taken = time_span.count();
    QL_DOUT("..5 nvq=" << nvq);

    // QL_DOUT("... determine result of solving");
    Mip::ProblemType pt = mip.type();
    QL_DOUT("..6 nvq=" << nvq);
    if (s != Mip::SOLVED || pt != Mip::OPTIMAL) {
        QL_DOUT("... InitialPlace: no (optimal) solution found; solve returned:" << s << " type returned:" << pt);
        QL_DOUT("InitialPlace.body [FAILED, DID NOT FIND MAPPING]");
        return Result::FAILED;
    }
    QL_DOUT("..7 nvq=" << nvq);

    // return new mapping as result in v2r

    // get the results: x[i][k] == 1 iff facility i is in location k (i.e. real qubit index k)
    // use v2i to translate facilities back to original virtual qubit indices
    // and fill v2r with the found locations for the used virtual qubits;
    // the unused mapped virtual qubits are mapped to an arbitrary permutation of the remaining locations;
    // the latter must be updated to generate swaps when mapping multiple kernels
    QL_DOUT("..8 nvq=" << nvq);
    QL_DOUT("... interpret result and copy to Virt2Real, nvq=" << nvq);
    for (UInt v = 0; v < nvq; v++) {
        QL_DOUT("... about to set v2r to undefined for v " << v);
        v2r[v] = com::map::UNDEFINED_QUBIT;      // i.e. undefined, i.e. v is not an index of a used virtual qubit
    }
    for (UInt i = 0; i < nfac; i++) {
        UInt v;   // found virtual qubit index v represented by facility i
        // use v2i backward to find virtual qubit v represented by facility i
        QL_DOUT("... about to inspect v2i to get solution and set it in v2r for facility " << i);
        for (v = 0; v < nvq; v++) {
            if (v2i[v] == i) {
                break;
            }
        }
        QL_ASSERT(v < nvq);  // for each facility there must be a virtual qubit
        UInt k;   // location to which facility i being virtual qubit index v was allocated
        for (k = 0; k < nlocs; k++) {
            if (mip.sol(x[i][k]) == 1) {
                v2r[v] = k;
                // v2r.rs[] is not updated because no gates were really mapped yet
                break;
            }
        }
        QL_ASSERT(k < nlocs);  // each facility i by definition represents a used qubit so must have got a location
        QL_DOUT("... end loop body over nfac");
    }

    if (options.map_all) {
        QL_DOUT("... correct location of unused mapped virtual qubits to be an unused location");
#ifdef MULTI_LINE_LOG_DEBUG
        QL_IF_LOG_DEBUG {
            QL_DOUT("dump v2r of InitialPlace before mapping unused mapped virtual qubits:");
            v2r.dump_state();
        }
#else
        QL_DOUT("dump v2r of InitialPlace before mapping unused mapped virtual qubits (disabled)");
#endif
        // virtual qubits used by this kernel v have got their location k filled in in v2r[v] == k
        // unused mapped virtual qubits still have location UNDEFINED_QUBIT, fill with the remaining locs
        // this should be replaced by actually swapping them to there, when mapping multiple kernels
        for (UInt v = 0; v < nvq; v++) {
            if (v2r[v] == com::map::UNDEFINED_QUBIT) {
                // v is unused by this kernel; find an unused location k
                UInt k;   // location k that is checked for having been allocated to some virtual qubit w
                for (k = 0; k < nlocs; k++) {
                    UInt w;
                    for (w = 0; w < nvq; w++) {
                        if (v2r[w] == k) {
                            break;
                        }
                    }
                    if (w >= nvq) {
                        // no w found for which v2r[w] == k
                        break;     // k is an unused location
                    }
                    // k is a used location, so continue with next k to check whether it is hopefully unused
                }
                QL_ASSERT(k < nlocs);  // when a virtual qubit is not used, there must be a location that is not used
                v2r[v] = k;
            }
            QL_DOUT("... end loop body over nvq when mapinitone2oneopt");
        }
    }
#ifdef MULTI_LINE_LOG_DEBUG
    QL_IF_LOG_DEBUG {
        QL_DOUT("... final result Virt2Real map of InitialPlace");
        v2r.dump_state();
    }
#else
    QL_DOUT("... final result Virt2Real map of InitialPlace (disabled)");
#endif
    QL_DOUT("InitialPlace.body [SUCCESS, FOUND MAPPING]");
    return Result::NEW_MAP;
}

/**
 * Wrapper around body() that runs it in a separate thread with a timeout.
 *
 * FIXME JvS: THIS IS *EXTREMELY* BROKEN. The thread doesn't actually stop
 *  on timeout; it doesn't even *try* to stop. It just keeps running in the
 *  background, and even continues poking around on the stack of the main
 *  thread!
 */
Bool InitialPlacementAlgo::wrapper(com::map::QubitMapping &v2r) {
    throw Exception(
        "Initial placement with timeout is disabled, because its current "
        "implementation is completely broken. On timeout, the entire process "
        "OpenQL is running in would go into an undefined state that can do "
        "anything from continuing with no problem at all to deleting system32, "
        "because the worker thread will keep running in the background, poking "
        "around in memory it doesn't own as it does."
    );

    QL_DOUT("InitialPlace.wrapper called");
    std::mutex  m;
    std::condition_variable cv;

    /*// prepare timeout
    JvS: this is the responsibility of the initial placement caller now. If/when
     timeout logic is reintroduced, it should be moved. Same for the exception
     throw.
    Bool throwexception = initialplaceopt.at(initialplaceopt.size() - 1) == 'x';
    Str waittime = throwexception
                         ? initialplaceopt.substr(0, initialplaceopt.size() - 1)
                         : initialplaceopt;
    Int waitseconds = parse_int(waittime.substr(0, waittime.size() - 1));
    switch (initialplaceopt.at(initialplaceopt.size() - 1)) {
        case 's': break;
        case 'm': waitseconds *= 60; break;
        case 'h': waitseconds *= 3600; break;
        default:
            QL_FATAL("Unknown value of option 'initialplace'='" << initialplaceopt << "'.");
    }
    time_taken = waitseconds;    // pessimistic, in case of timeout, otherwise it is corrected*/

    // v2r and result are allocated on stack of main thread by some ancestor so be careful with threading
    std::thread t([&cv, this, &v2r]()
        {
            QL_DOUT("InitialPlace.wrapper subthread about to call body");
            result = body(v2r);
            QL_DOUT("InitialPlace.body returned in subthread; about to signal the main thread");
            cv.notify_one();        // by this, the main thread awakes from cv.wait_for without timeout
            QL_DOUT("InitialPlace.wrapper subthread after signaling the main thread, and is about to die");
        }
    );
    QL_DOUT("InitialPlace.wrapper main code created thread; about to call detach on it");
    t.detach();
    QL_DOUT("InitialPlace.wrapper main code detached thread");
    {
        std::chrono::milliseconds maxwaittime(static_cast<utils::UInt>(options.timeout * 1000));
        std::unique_lock<std::mutex> l(m);
        QL_DOUT("InitialPlace.wrapper main code starts waiting with timeout of " << options.timeout << " seconds");
        if (cv.wait_for(l, maxwaittime) == std::cv_status::timeout) {
            QL_DOUT("InitialPlace.wrapper main code awoke from waiting with timeout");
            /*if (throwexception) {
                QL_DOUT("InitialPlace: timed out and stops compilation [TIMED OUT, STOP COMPILATION]");
                QL_FATAL("Initial placement timed out and stops compilation [TIMED OUT, STOP COMPILATION]");
            }
            QL_DOUT("InitialPlace.wrapper about to return timedout==true");*/
            return true;
        }
        QL_DOUT("InitialPlace.PlaceWrapper main code awoke from waiting without timeout, from signal sent by InitialPlace.wrapper subthread just before its death");
    }

    QL_DOUT("InitialPlace.wrapper about to return timedout==false");
    return false;
}

// find an initial placement of the virtual qubits for the given circuit as in Place
// put a timelimit on its execution specified by the initialplace option
// when it expires, result is set to ipr_timedout;
// details of how this is accomplished, can be found above;
// v2r is updated by PlaceBody/PlaceWrapper when it has found a mapping
Result InitialPlacementAlgo::run(
    const ir::compat::KernelRef &k,
    const Options &opt,
    com::map::QubitMapping &v2r
) {

    // Initialize ourselves for the given kernel.
    options = opt;
    kernel = k;
    platform = kernel->platform;
    nlocs = platform->qubit_count;
    nvq = platform->qubit_count;  // same range; when not, take set from config and create v2i earlier
    nfac = 0;
    result = Result::FAILED;
    time_taken = 0.0;

    QL_DOUT("Init: platformp=" << platform.get_ptr() << " nlocs=" << nlocs << " nvq=" << nvq);

    QL_DOUT("InitialPlace.Place ...");
    if (options.timeout <= 0.0) {

        // Do initial placement without a time limit.
        QL_DOUT("InitialPlace.Place calling body without time limit");
        body(v2r);
        QL_DOUT("InitialPlace.Place [done, no time limit], result=" << result << " iptimetaken=" << time_taken << " seconds");

    } else {

        // Save original virtual to real qubit map in case we get a timeout, so
        // we can restore the original.
        auto v2r_orig = v2r;

        // Run the wrapper with timeout.
        auto timed_out = wrapper(v2r);

        // Replace garbage results with real values if there was a timeout.
        if (timed_out) {
            v2r = v2r_orig;
            result = Result::TIMED_OUT;
        }

        // Print debug output.
        if (timed_out) {
            QL_DOUT("InitialPlace.Place [done, TIMED OUT, NO MAPPING FOUND], result=" << result << " iptimetaken=" << time_taken << " seconds");
        } else {
            QL_DOUT("InitialPlace.Place [done, not timed out], result=" << result << " iptimetaken=" << time_taken << " seconds");
        }

    }

    return result;
}

/**
 * Returns the amount of time taken by the mixed-integer-programming solver
 * for the call to run() in seconds.
 */
utils::Real InitialPlacementAlgo::get_time_taken() const {
    return time_taken;
}

} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql

#endif // INITIALPLACE
