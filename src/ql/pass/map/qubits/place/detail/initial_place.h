/** \file
 * Initial placement engine.
 *
 * TODO JvS: clean up docs
 *
 * InitialPlace: initial placement solved as an MIP, mixed integer linear program
 * the initial placement is modelled as a Quadratic Assignment Problem
 * by Lingling Lao in her mapping paper:
 *
 * variables:
 *     forall i: forall k: x[i][k], x[i][k] is integral and 0 or 1, meaning qubit i is in location k
 * objective:
 *     min z = sum i: sum j: sum k: sum l: refcount[i][j] * distance(k,l) * x[i][k] * x[j][l]
 * subject to:
 *     forall k: ( sum i: x[i][k] <= 1 )        allow more locations than qubits
 *     forall i: ( sum k: x[i][k] == 1 )        but each qubit must have one locations
 *
 * the article "An algorithm for the quadratic assignment problem using Benders' decomposition"
 * by L. Kaufman and F. Broeckx, transforms this problem by introducing w[i][k] as follows:
 *
 * forall i: forall k: w[i][k] =  x[i][k] * ( sum j: sum l: refcount[i][j] * distance(k,l) * x[j][l] )
 *
 * to the following mixed integer linear problem:
 *
 *  precompute:
 *      forall i: forall k: costmax[i][k] = sum j: sum l: refcount[i][j] * distance(k,l)
 *      (note: each of these costmax[][] is >= 0, so the "max(this,0)" around this is not needed)
 *  variables:
 *      forall i: forall k: x[i][k], x[i][k] is integral and 0 or 1
 *      forall i: forall k: w[i][k], w[i][k] is real and >= 0
 *  objective:
 *      min z = sum i: sum k: w[i][k]
 *  subject to:
 *      forall k: ( sum i: x[i][k] <= 1 )
 *      forall i: ( sum k: x[i][k] == 1 )
 *      forall i: forall k: costmax[i][k] * x[i][k]
 *          + ( sum j: sum l: refcount[i][j]*distance(k,l)*x[j][l] ) - w[i][k] <= costmax[i][k]
 *
 * This model is coded in lemon/mip below.
 * The latter is mapped onto glpk.
 *
 * Since solving takes a while, two ways are offered to deal with this; these can be combined:
 * 1. option initialplace2qhorizon: one of: 0,10,20,30,40,50,60,70,80,90,100
 * The initialplace algorithm considers only this number of initial two-qubit gates to determine a mapping.
 * When 0 is specified as option value, there is no limit.
 * 2. option initialplace: an option steerable timeout mechanism around it is implemented, using threads:
 * The solver runs in a subthread which can succeed or be timed out by the main thread waiting for it.
 * When timed out, it can stop the compiler by raising an exception or continue mapping as if it were not called.
 * When INITIALPLACE is not defined, the compiler doesn't contain initial placement support and ignores calls to it;
 * then all this: lemon/mip, glpk, thread support is avoided making OpenQL much easier build and run.
 * Otherwise, depending on the initialplace option value, initial placement is attempted before the heuristic.
 * Options values of initialplace:
 *  no      don't run initial placement ('ip')
 *  yes     run ip until the solver is ready
 *  1hx     run ip max for 1 hour; when timed out, stop the compiler
 *  1h      run ip max for 1 hour; when timed out, just use heuristics
 *  10mx    run ip max for 10 minutes; when timed out, stop the compiler
 *  10m     run ip max for 10 minutes; when timed out, just use heuristics
 *  1mx     run ip max for 1 minute; when timed out, stop the compiler
 *  1m      run ip max for 1 minute; when timed out, just use heuristics
 *  10sx    run ip max for 10 seconds; when timed out, stop the compiler
 *  10s     run ip max for 10 seconds; when timed out, just use heuristics
 *  1sx     run ip max for 1 second; when timed out, stop the compiler
 *  1s      run ip max for 1 second; when timed out, just use heuristics
 */

#pragma once

#ifdef INITIALPLACE

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/ptr.h"
#include "ql/utils/vec.h"
#include "ql/plat/platform.h"
#include "ql/plat/topology.h"
#include "ql/ir/ir.h"
#include "ql/com/qubit_mapping.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace place {
namespace detail {

using namespace utils;
using namespace plat::topology;

typedef enum InitialPlaceResults {
    ipr_any,            // any mapping will do because there are no two-qubit gates in the circuit
    ipr_current,        // current mapping will do because all two-qubit gates are NN
    ipr_newmap,         // initial placement solution found a mapping
    ipr_failed,         // initial placement solution failed
    ipr_timedout        // initial placement solution timed out and thus failed
} ipr_t;

class InitialPlace {
private:
                                          // parameters, constant for a kernel
    plat::PlatformRef         platformp;  // platform
    UInt                      nlocs;      // number of locations, real qubits; index variables k and l
    UInt                      nvq;        // same range as nlocs; when not, take set from config and create v2i earlier
    utils::Ptr<Grid>          gridp;      // current grid with Distance function

                                          // remaining attributes are computed per circuit
    UInt                      nfac;       // number of facilities, actually used virtual qubits; index variables i and j
                                          // nfac <= nlocs: e.g. nlocs == 7, but only v2 and v5 are used; nfac then is 2

public:

    Str ipr2string(ipr_t ipr);

    // kernel-once initialization
    void Init(const utils::Ptr<Grid> &g, const plat::PlatformRef &p);

    // find an initial placement of the virtual qubits for the given circuit
    // the resulting placement is put in the provided virt2real map
    // result indicates one of the result indicators (ipr_t, see above)
    void PlaceBody(const ir::Circuit &circ, com::QubitMapping &v2r, ipr_t &result, Real &iptimetaken);

    // the above PlaceBody is a regular function using circ, and updating v2r and result before it returns;
    // it implements Initial Placement as if the call to Place in the mapper called PlaceBody directly;
    // because it may take a while to return, a new Place and a PlaceWrapper are put in between;
    // the idea is to run PlaceBody in a detached thread, that, when ready, signals the main thread;
    // the main thread waits for this signal with a timeout value;
    // all this is done in a try block where the catch is called on this timeout;
    // why exceptions are used, is not clear, so it was replaced by PlaceWrapper returning "timedout" or not
    // and this works as well ...
    Bool PlaceWrapper(
        const ir::Circuit &circ,
        com::QubitMapping &v2r,
        ipr_t &result,
        Real &iptimetaken,
        const Str &initialplaceopt
    );

    // find an initial placement of the virtual qubits for the given circuit as in Place
    // put a timelimit on its execution specified by the initialplace option
    // when it expires, result is set to ipr_timedout;
    // details of how this is accomplished, can be found above;
    // v2r is updated by PlaceBody/PlaceWrapper when it has found a mapping
    void Place(
        const ir::Circuit &circ,
        com::QubitMapping &v2r,
        ipr_t &result,
        Real &iptimetaken,
        const Str &initialplaceopt
    );

};  // end class InitialPlace

} // namespace detail
} // namespace place
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql

#endif // INITIALPLACE
