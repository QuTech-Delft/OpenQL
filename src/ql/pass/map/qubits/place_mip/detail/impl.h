#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/ptr.h"
#include "ql/utils/vec.h"
#include "ql/utils/pairhash.h"
#include "ql/ir/ir.h"
#include "ql/ir/compat/compat.h"
#include "ql/com/map/qubit_mapping.h"

class HighsModel;

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace place_mip {
namespace detail {

/**
 * Options structure for configuring the initial placement algorithm.
 */
struct Options {
    /**
     * Filename where to write the MPS model.
     */
    utils::Str model_filename = "";

    /**
     * Whether to write the MPS model to a file.
     */
    utils::Bool write_model_to_file = false;

    /**
     * Timeout for the MIP algorithm in seconds, or 0 to disable timeout.
     */
    utils::Real timeout = 0.0;

    /**
     * The placement algorithm will only consider the connectivity required to
     * perform the first horizon two-qubit gate types. 0 means that all
     * gate types should be considered.
     */
    utils::UInt horizon = 0;

    /**
     * Whether to exit compilation and error when the MIP solving exceeds the set timeout.
     * If true, compiler exits with an error.
     * If false, in case of timeout, the pass does not update qubit indices.
     */
    utils::Bool fail_on_timeout = true;
};

/**
 * Enumeration of the possible algorithm outcomes.
 */
enum class Result {

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
     * The placement algorithm found a mapping suitable for all two-qubit gate types
     * wrt horizon.
     */
    NEW_MAP,

    /**
     * No solution exists that satisfies the constraints for all two-qubit gate types
     * wrt horizon.
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
std::ostream &operator<<(std::ostream &os, Result ipr);

static constexpr utils::UInt UNDEFINED_QUBIT = utils::MAX;

/**
 * Initial placement algorithm.
 */
class Impl {
public:
    using TwoQGatesCount = std::unordered_map<std::pair<utils::UInt, utils::UInt>, utils::UInt, ql::utils::PairHash>;
    using DistanceProvider = std::function<utils::UInt(utils::UInt, utils::UInt)>;

private:
    /**
     * Checks whether all 2q gates have nearest-neighbor operands, with respect to the distance provider.
     */
    bool hasNonNN2QGates();

    /**
     * Compute costmax, which is a integer matrix used in the MIP problem.
     */
    utils::Vec<utils::Vec<utils::UInt>> computeCostMax(const utils::Vec<utils::Vec<utils::UInt>> &refcount);

    /**
     * Fills and returns the HiGHS model corresponding to the MIP problem to solve.
     */
    std::unique_ptr<HighsModel> createHiGHSModel(const utils::Vec<utils::Vec<utils::UInt>> &refcount);

    /**
     * Number of locations, real qubits; index variables k and l.
     */
    utils::UInt qubitsCount = 0;

    /**
     * Number of facilities, that is, virtual qubits that appear in the operands of a 2q gate in the circuit.
     */
    utils::UInt nfac = 0;

    /**
     * Total time taken by the MIP solving in seconds.
     */
    utils::Real time_taken = 0.0;

    /**
     * A map from pairs of two qubit indices to the number of times they occur in the circuit.
     * The horizon most occuring pairs of operands are kept, except when horizon = 0.
     */
    const TwoQGatesCount &twoQGatesCount;

    /**
     * A callback returning the distance between 2 qubit indices, in number of hops.
     * This allows for easy unit-testing.
     */
    DistanceProvider distanceProvider;

    const Options &opts;

public:
    Impl(utils::UInt aQubitsCount, const TwoQGatesCount &aTwoQGatesCount, DistanceProvider aDistanceProvider, const Options &aOpts);

    /**
     * Runs the algorithm to find an initial placement of the virtual qubits for
     * the given kernel with the given options. v2r is updated with
     * the new mapping if one is found.
     */
    Result run(utils::Vec<utils::UInt> &v2r);

    /**
     * Returns the amount of time taken by the mixed-integer-programming solver
     * for the call to run() in seconds.
     */
    utils::Real getTimeTaken() const;

};

/**
 * The MIP problem only cares about _how many_ 2 gates of each type exist in the circuit.
 * This function returns a map from 2q gate types to number of occurrences.
 * This allows for easy unit-testing of the MIP part, by providing easy to create input.
 */
Impl::TwoQGatesCount inventorize2QGates(ir::Ref ir);

void applyHorizon(utils::UInt horizon, Impl::TwoQGatesCount &twoQGatesCount);

/**
 * Entrypoint of the pass.
 */
Result performInitialPlacement(ir::Ref ir, const Options &opts, utils::Vec<utils::UInt> &mapping);

} // namespace detail
} // namespace place
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql