/** \file
 * FreeCycle implementation.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/vec.h"
#include "ql/utils/opt.h"
#include "ql/ir/compat/compat.h"
#include "ql/rmgr/manager.h"
#include "ql/com/map/qubit_mapping.h"
#include "options.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace map {
namespace detail {

/**
 * FreeCycle: map each real qubit to the first cycle that it is free for use.
 *
 * In scheduling gates, qubit dependencies cause latencies. For each real qubit,
 * the first cycle that it is free to use is the cycle that the last gate that
 * was scheduled in the qubit, has just finished (i.e. in the previous cycle);
 * the map serves as a summary to ease scheduling next gates.
 *
 * Likewise, while mapping, swaps are scheduled just before a non-NN two-qubit
 * gate. Moreover, such swaps may involve real qubits on the path between the
 * real operand qubits of the gate, which may be different from the real operand
 * qubits. The evaluation of which path of swaps is best is, among other data,
 * based on which path causes the latency of the whole circuit to be extended
 * the least. This latency extension is measured from the data in the FreeCycle
 * map; so a FreeCycle map is part of each path of swaps that is evaluated for a
 * particular non-NN 2-qubit gate next to a FreeCycle map that is part of the
 * output stream (the main past).
 *
 * Since gate durations are in nanoseconds, and one cycle is some fixed number
 * of nanoseconds, the duration is converted to a rounded-up number of cycles
 * when computing the added latency.
 */
class FreeCycle {
private:

    /**
     * Platform description.
     */
    ir::compat::PlatformRef platform;

    /**
     * Parsed mapper pass options.
     */
    OptionsRef options;

    /**
     * Map is (nq+nb) long; after initialization, will always be the same.
     */
    utils::UInt nq;

    /**
     * Bregs are in map (behind qubits) to track dependences around conditions.
     *
     * FIXME JvS: why qubits and bregs, but not cregs?
     */
    utils::UInt nb;

    /**
     * Multiplication factor from cycles to nano-seconds (unit of duration).
     */
    utils::UInt ct;

    /**
     * fcv[real qubit index i]: qubit i is free from this cycle on.
     */
    utils::Vec<utils::UInt> fcv;

    /**
     * Actual resources occupied by scheduled gates, if resource-aware.
     */
    utils::Opt<rmgr::State> rs;

public:

    /**
     * Initializes this FreeCycle object.
     */
    void initialize(const ir::compat::PlatformRef &p, const OptionsRef &opt);

    /**
     * Returns the depth of the FreeCycle map. Equals the max of all entries
     * minus the min of all entries not used yet; would be used to compute the
     * max size of a top window on the past.
     */
    utils::UInt get_depth() const;

    /**
     * Returns the minimum cycle of the FreeCycle map; equals the min of all
     * entries.
     */
    utils::UInt get_min() const;

    /**
     * Returns the maximum cycle of the FreeCycle map; equals the max of all
     * entries.
     */
    utils::UInt get_max() const;

    /**
     * Prints the state of this object along with the given string.
     */
    void print(const utils::Str &s) const;

    /**
     * Calls print only if the loglevel is debug or more verbose.
     */
    void debug_print(const utils::Str &s) const;

    /**
     * Return whether gate with first operand qubit r0 can be scheduled earlier
     * than with operand qubit r1.
     */
    utils::Bool is_first_operand_earlier(utils::UInt r0, utils::UInt r1) const;

    /**
     * Returns whether swap(fr0,fr1) start earlier than a swap(sr0,sr1). Is
     * really a short-cut ignoring config file and perhaps several other
     * details.
     */
    utils::Bool is_first_swap_earliest(
        utils::UInt fr0,
        utils::UInt fr1,
        utils::UInt sr0,
        utils::UInt sr1
    ) const;

    /**
     * Returns what the start cycle would be when we would schedule the given
     * gate, ignoring resource constraints. gate operands are real qubit indices
     * and breg indices. Purely functional, doesn't affect state.
     */
    utils::UInt get_start_cycle_no_rc(const ir::compat::GateRef &g) const;

    /**
     * Returns what the start cycle would be when we would schedule the given
     * gate. gate operands are real qubit indices and breg indices. Purely
     * functional, doesn't affect state.
     */
    utils::UInt get_start_cycle(const ir::compat::GateRef &g) const;

    /**
     * Schedules the given gate in the FreeCycle map. The gate operands are real
     * qubit indices and breg indices. The FreeCycle map is updated, but not the
     * resource map. This is done because add_no_rc is used to represent just gate
     * dependencies, avoiding a build of a dep graph.
     */
    void add_no_rc(const ir::compat::GateRef &g, utils::UInt startCycle);

    /**
     * Schedules the given gate in the FreeCycle and resource maps. The gate
     * operands are real qubit indices and breg indices. Both the FreeCycle map
     * and the resource map are updated. startcycle must be the result of an
     * earlier StartCycle call (with rc!)
     */
    void add(const ir::compat::GateRef &g, utils::UInt start_cycle);

};

} // namespace detail
} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
