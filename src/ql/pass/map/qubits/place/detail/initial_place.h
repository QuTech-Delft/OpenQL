/** \file
 * Initial placement engine.
 *
 * InitialPlace: initial placement solved as an MIP (mixed integer linear
 * program). The initial placement is modelled as a Quadratic Assignment Problem
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
 * Since solving takes a while, two ways are offered to deal with this (and
 * these can be combined):
 *
 *  - the initial placement "horizon" may be used to limit the number of
 *    two-qubit gates considered by the solver to the first N for each kernel;
 *  - a timeout may be specified.
 */

#pragma once

#ifdef INITIALPLACE

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/ptr.h"
#include "ql/utils/vec.h"
#include "ql/plat/platform.h"
#include "ql/ir/ir.h"
#include "ql/com/qubit_mapping.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace place {
namespace detail {

/**
 * Options structure for configuring the initial placement algorithm.
 */
struct InitialPlaceOptions {

    /**
     * Timeout for the MIP algorithm in seconds, or 0 to disable timeout.
     */
    utils::Real timeout = 0.0;

    /**
     * The placement algorithm will only consider the connectivity required to
     * perform the first horizon two-qubit gates of a kernel. 0 means that all
     * gates should be considered.
     */
    utils::UInt horizon = 0;

    /**
     * When set, any virtual qubits not used in the original kernel will also
     * be mapped to real qubits.
     */
    utils::Bool map_all = false;

};

/**
 * Enumeration of the possible algorithm outcomes.
 */
enum class InitialPlaceResult {

    /**
     * Any mapping will do, because there are no two-qubit gates in the circuit.
     */
    ANY,

    /**
     * The current mapping will do, because all two-qubit gates are
     * nearest-neighbor.
     */
    CURRENT,

    /**
     * The placement algorithm found a mapping suitable for all two-qubit gates
     * before the configured placement horizon.
     */
    NEW_MAP,

    /**
     * No solution exists that satisfies the constraints for all two-qubit gates
     * before the configured placement horizon.
     */
    FAILED,

    /**
     * The algorithm timed out before a solution could be found.
     */
    TIMED_OUT

};

/**
 * String conversion for initial placement results.
 */
std::ostream &operator<<(std::ostream &os, InitialPlaceResult ipr);

/**
 * Initial placement algorithm.
 */
class InitialPlace {
private:

    /**
     * The options that we're being called with.
     */
    InitialPlaceOptions options;

    /**
     * Reference to the kernel we're operating on.
     */
    ir::KernelRef kernel;

    /**
     * Shorthand reference for the platform corresponding to the kernel.
     */
    plat::PlatformRef platform;

    /**
     * Number of locations, real qubits; index variables k and l.
     */
    utils::UInt nlocs = 0;

    /**
     * Same range as nlocs; when not, take set from config and create v2i
     * earlier.
     */
    utils::UInt nvq = 0;

    /**
     * Number of facilities, actually used virtual qubits; index variables i and
     * j. nfac <= nlocs: e.g. nlocs == 7, but only v2 and v5 are used; nfac then
     * is 2.
     */
    utils::UInt nfac = 0;

    /**
     * Initial placement result.
     */
    InitialPlaceResult result = InitialPlaceResult::FAILED;

    /**
     * Total time taken by body() in seconds.
     */
    utils::Real time_taken = 0.0;

    /**
     * The actual algorithm body. Finds an initial placement of the virtual
     * qubits for the configured kernel. The resulting placement is put in the
     * provided qubit map. time_taken is set to the time taken by the actual
     * algorithm.
     */
    InitialPlaceResult body(com::QubitMapping &v2r);

    /**
     * Wrapper around body() that runs it in a separate thread with a timeout.
     *
     * FIXME JvS: THIS IS *EXTREMELY* BROKEN. The thread doesn't actually stop
     *  on timeout; it doesn't even *try* to stop. It just keeps running in the
     *  background, and even continues poking around on the stack of the main
     *  thread!
     */
    utils::Bool wrapper(com::QubitMapping &v2r);

public:

    /**
     * Runs the algorithm to find an initial placement of the virtual qubits for
     * the given kernel with the given options. v2r is updated by
     * PlaceBody/PlaceWrapper when it has found a mapping.
     */
    InitialPlaceResult run(
        const ir::KernelRef &k,
        const InitialPlaceOptions &opt,
        com::QubitMapping &v2r
    );

    /**
     * Returns the amount of time taken by the mixed-integer-programming solver
     * for the call to run() in seconds.
     */
    utils::Real get_time_taken() const;

};

} // namespace detail
} // namespace place
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql

#endif // INITIALPLACE
