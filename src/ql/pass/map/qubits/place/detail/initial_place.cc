/** \file
 * Initial placement engine.
 */

#ifdef INITIALPLACE

#include "initial_place.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <lemon/lp.h>
#include "ql/com/options.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace place {
namespace detail {

using namespace lemon;

Str InitialPlace::ipr2string(ipr_t ipr) {
    switch (ipr) {
        case ipr_any:       return "any";
        case ipr_current:   return "current";
        case ipr_newmap:    return "newmap";
        case ipr_failed:    return "failed";
        case ipr_timedout:  return "timedout";
    }
    return "unknown";
}

// kernel-once initialization
void InitialPlace::Init(const utils::Ptr<Grid> &g, const plat::PlatformRef &p) {
    // QL_DOUT("InitialPlace Init ...");
    platformp = p;
    nlocs = p->qubit_count;
    nvq = p->qubit_count;  // same range; when not, take set from config and create v2i earlier
    // QL_DOUT("... number of real qubits (locations): " << nlocs);
    gridp = g;
    QL_DOUT("Init: platformp=" << platformp.get_ptr() << " nlocs=" << nlocs << " nvq=" << nvq << " gridp=" << gridp.unwrap());
}

// find an initial placement of the virtual qubits for the given circuit
// the resulting placement is put in the provided virt2real map
// result indicates one of the result indicators (ipr_t, see above)
void InitialPlace::PlaceBody(const ir::Circuit &circ, Virt2Real &v2r, ipr_t &result, Real &iptimetaken) {
    QL_DOUT("InitialPlace.PlaceBody ...");

    // check validity of circuit
    for (auto &gp : circ) {
        auto &q = gp->operands;
        if (q.size() > 2) {
            QL_FATAL(" gate: " << gp->qasm() << " has more than 2 operand qubits; please decompose such gates first before mapping.");
        }
    }

    // only consider first number of two-qubit gates as specified by option initialplace2qhorizon
    // this influences refcount (so constraints) and nfac (number of facilities, so size of MIP problem)
    Str initialplace2qhorizonopt = com::options::get("initialplace2qhorizon");
    Int prefix = parse_int(initialplace2qhorizonopt);

    // compute ipusecount[] to know which virtual qubits are actually used
    // use it to compute v2i, mapping (non-contiguous) virtual qubit indices to contiguous facility indices
    // (the MIP model is shorter when the indices are contiguous)
    // finally, nfac is set to the number of these facilities;
    // only consider virtual qubit uses until the specified max number of two qubit gates has been seen
    QL_DOUT("... compute ipusecount by scanning circuit");
    Vec<UInt>  ipusecount;// ipusecount[v] = count of use of virtual qubit v in current circuit
    ipusecount.resize(nvq,0);       // initially all 0
    Vec<UInt> v2i;        // v2i[virtual qubit index v] -> index of facility i
    v2i.resize(nvq,UNDEFINED_QUBIT);// virtual qubit v not used by circuit as gate operand

    Int twoqubitcount = 0;
    for (auto &gp : circ) {
        if (prefix == 0 || twoqubitcount < prefix) {
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
    for (auto &gp : circ) {
        auto &q = gp->operands;
        if (q.size() == 2) {
            if (prefix == 0 || twoqubitcount < prefix) {
                anymap = false;
                refcount[v2i[q[0]]][v2i[q[1]]] += 1;

                if (
                    v2r[q[0]] == UNDEFINED_QUBIT
                    || v2r[q[1]] == UNDEFINED_QUBIT
                    || gridp->Distance(v2r[q[0]], v2r[q[1]]) > 1
                ) {
                    currmap = false;
                }
            }
            twoqubitcount++;
        }
    }
    if (prefix != 0 && twoqubitcount >= prefix) {
        QL_DOUT("InitialPlace: only considered " << prefix << " of " << twoqubitcount << " two-qubit gates, so resulting mapping is not exact");
    }
    if (anymap) {
        QL_DOUT("InitialPlace: no two-qubit gates found, so no constraints, and any mapping is ok");
        QL_DOUT("InitialPlace.PlaceBody [ANY MAPPING IS OK]");
        result = ipr_any;
        iptimetaken = 0.0;
        return;
    }
    if (currmap) {
        QL_DOUT("InitialPlace: in current map, all two-qubit gates are nearest neighbor, so current map is ok");
        QL_DOUT("InitialPlace.PlaceBody [CURRENT MAPPING IS OK]");
        result = ipr_current;
        iptimetaken = 0.0;
        return;
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
                    left += refcount[i][j] * gridp->Distance(k,l) * x[j][l];
                    if (refcount[i][j] * gridp->Distance(k,l) != 0) {
                        if (started) {
                            lefts += " + ";
                        } else {
                            started = true;
                        }
                        lefts += to_string(refcount[i][j] * gridp->Distance(k,l));
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
    QL_DOUT("Just before solve: platformp=" << platformp.get_ptr() << " nlocs=" << nlocs << " nvq=" << nvq << " gridp=" << gridp.unwrap());
    QL_DOUT("Just before solve: objs=" << objs << " x.size()=" << x.size() << " w.size()=" << w.size() << " refcount.size()=" << refcount.size() << " v2i.size()=" << v2i.size() << " ipusecount.size()=" << ipusecount.size());
    QL_DOUT("..2b nvq=" << nvq);
    {
        s = mip.solve();
    }
    QL_DOUT("..3 nvq=" << nvq);
    QL_DOUT("Just after solve: platformp=" << platformp.get_ptr() << " nlocs=" << nlocs << " nvq=" << nvq << " gridp=" << gridp.unwrap());
    QL_DOUT("Just after solve: objs=" << objs << " x.size()=" << x.size() << " w.size()=" << w.size() << " refcount.size()=" << refcount.size() << " v2i.size()=" << v2i.size() << " ipusecount.size()=" << ipusecount.size());
    QL_ASSERT(nvq == nlocs);         // consistency check, mainly to let it crash

    // computing iptimetaken, stop interval timer
    QL_DOUT("..4 nvq=" << nvq);
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    duration<Real> time_span = t2 - t1;
    iptimetaken = time_span.count();
    QL_DOUT("..5 nvq=" << nvq);

    // QL_DOUT("... determine result of solving");
    Mip::ProblemType pt = mip.type();
    QL_DOUT("..6 nvq=" << nvq);
    if (s != Mip::SOLVED || pt != Mip::OPTIMAL) {
        QL_DOUT("... InitialPlace: no (optimal) solution found; solve returned:" << s << " type returned:" << pt);
        result = ipr_failed;
        QL_DOUT("InitialPlace.PlaceBody [FAILED, DID NOT FIND MAPPING]");
        return;
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
        v2r[v] = UNDEFINED_QUBIT;      // i.e. undefined, i.e. v is not an index of a used virtual qubit
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

    auto mapinitone2oneopt = com::options::get("mapinitone2one");
    if (mapinitone2oneopt == "yes") {
        QL_DOUT("... correct location of unused mapped virtual qubits to be an unused location");
        v2r.DPRINT("... result Virt2Real map of InitialPlace before mapping unused mapped virtual qubits ");
        // virtual qubits used by this kernel v have got their location k filled in in v2r[v] == k
        // unused mapped virtual qubits still have location UNDEFINED_QUBIT, fill with the remaining locs
        // this should be replaced by actually swapping them to there, when mapping multiple kernels
        for (UInt v = 0; v < nvq; v++) {
            if (v2r[v] == UNDEFINED_QUBIT) {
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
    v2r.DPRINT("... final result Virt2Real map of InitialPlace");
    result = ipr_newmap;
    QL_DOUT("InitialPlace.PlaceBody [SUCCESS, FOUND MAPPING]");
}

// the above PlaceBody is a regular function using circ, and updating v2r and result before it returns;
// it implements Initial Placement as if the call to Place in the mapper called PlaceBody directly;
// because it may take a while to return, a new Place and a PlaceWrapper are put in between;
// the idea is to run PlaceBody in a detached thread, that, when ready, signals the main thread;
// the main thread waits for this signal with a timeout value;
// all this is done in a try block where the catch is called on this timeout;
// why exceptions are used, is not clear, so it was replaced by PlaceWrapper returning "timedout" or not
// and this works as well ...
Bool InitialPlace::PlaceWrapper(
    const ir::Circuit &circ,
    Virt2Real &v2r,
    ipr_t &result,
    Real &iptimetaken,
    const Str &initialplaceopt
) {
    QL_DOUT("InitialPlace.PlaceWrapper called");
    std::mutex  m;
    std::condition_variable cv;

    // prepare timeout
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
    iptimetaken = waitseconds;    // pessimistic, in case of timeout, otherwise it is corrected

    // v2r and result are allocated on stack of main thread by some ancestor so be careful with threading
    std::thread t([&cv, this, &circ, &v2r, &result, &iptimetaken]()
        {
            QL_DOUT("InitialPlace.PlaceWrapper subthread about to call PlaceBody");
            PlaceBody(circ, v2r, result, iptimetaken);
            QL_DOUT("InitialPlace.PlaceBody returned in subthread; about to signal the main thread");
            cv.notify_one();        // by this, the main thread awakes from cv.wait_for without timeout
            QL_DOUT("InitialPlace.PlaceWrapper subthread after signaling the main thread, and is about to die");
        }
    );
    QL_DOUT("InitialPlace.PlaceWrapper main code created thread; about to call detach on it");
    t.detach();
    QL_DOUT("InitialPlace.PlaceWrapper main code detached thread");
    {
        std::chrono::seconds maxwaittime(waitseconds);
        std::unique_lock<std::mutex> l(m);
        QL_DOUT("InitialPlace.PlaceWrapper main code starts waiting with timeout of " << waitseconds << " seconds");
        if (cv.wait_for(l, maxwaittime) == std::cv_status::timeout) {
            QL_DOUT("InitialPlace.PlaceWrapper main code awoke from waiting with timeout");
            if (throwexception) {
                QL_DOUT("InitialPlace: timed out and stops compilation [TIMED OUT, STOP COMPILATION]");
                QL_FATAL("Initial placement timed out and stops compilation [TIMED OUT, STOP COMPILATION]");
            }
            QL_DOUT("InitialPlace.PlaceWrapper about to return timedout==true");
            return true;
        }
        QL_DOUT("InitialPlace.PlaceWrapper main code awoke from waiting without timeout, from signal sent by InitialPlace.PlaceWrapper subthread just before its death");
    }

    QL_DOUT("InitialPlace.PlaceWrapper about to return timedout==false");
    return false;
}

// find an initial placement of the virtual qubits for the given circuit as in Place
// put a timelimit on its execution specified by the initialplace option
// when it expires, result is set to ipr_timedout;
// details of how this is accomplished, can be found above;
// v2r is updated by PlaceBody/PlaceWrapper when it has found a mapping
void InitialPlace::Place(
    const ir::Circuit &circ,
    Virt2Real &v2r,
    ipr_t &result,
    Real &iptimetaken,
    const Str &initialplaceopt
) {
    Virt2Real   v2r_orig = v2r;

    QL_DOUT("InitialPlace.Place ...");
    if (initialplaceopt == "yes") {
        // do initial placement without time limit
        QL_DOUT("InitialPlace.Place calling PlaceBody without time limit");
        PlaceBody(circ, v2r, result, iptimetaken);
        // v2r reflects new mapping, if any found, otherwise unchanged
        QL_DOUT("InitialPlace.Place [done, no time limit], result=" << result << " iptimetaken=" << iptimetaken << " seconds");
    } else {
        Bool timedout;
        timedout = PlaceWrapper(circ, v2r, result, iptimetaken, initialplaceopt);

        if (timedout) {
            result = ipr_timedout;
            QL_DOUT("InitialPlace.Place [done, TIMED OUT, NO MAPPING FOUND], result=" << result << " iptimetaken=" << iptimetaken << " seconds");

            v2r = v2r_orig; // v2r may have got corrupted when timed out during v2r updating
        } else {
            // v2r reflects new mapping, if any found, otherwise unchanged
            QL_DOUT("InitialPlace.Place [done, not timed out], result=" << result << " iptimetaken=" << iptimetaken << " seconds");
        }
    }
}

} // namespace detail
} // namespace place
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql

#endif // INITIALPLACE
