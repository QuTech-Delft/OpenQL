/** \file
 * FreeCycle implementation.
 */

#include "free_cycle.h"

#include "ql/ir/ops.h"

// uncomment next line to enable multi-line dumping
// #define MULTI_LINE_LOG_DEBUG

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace map {
namespace detail {

/**
 * Initializes this FreeCycle object.
 */
void FreeCycle::initialize(const ir::PlatformRef &p, const OptionsRef &opt) {
    auto rm = *p->resources;
    options = opt;
    platform = p;

    fcv.clear();

    rs = rm.build(rmgr::Direction::FORWARD);
}

/**
 * Returns the depth of the FreeCycle map. Equals the max of all entries
 * minus the min of all entries not used yet; would be used to compute the
 * max size of a top window on the past.
 */
utils::UInt FreeCycle::get_depth() const {
    return get_max() - get_min();
}

/**
 * Returns the minimum cycle of the FreeCycle map; equals the min of all
 * entries.
 */
utils::UInt FreeCycle::get_min() const {
    utils::UInt min_free_cycle = ir::compat::MAX_CYCLE;
    for (const auto &p : fcv) {
        if (p.second < min_free_cycle) {
            min_free_cycle = p.second;
        }
    }
    return min_free_cycle;
}

/**
 * Returns the maximum cycle of the FreeCycle map; equals the max of all
 * entries.
 */
utils::UInt FreeCycle::get_max() const {
    utils::UInt max_free_cycle = 0;
    for (const auto &p : fcv) {
        if (max_free_cycle < p.second) {
            max_free_cycle = p.second;
        }
    }
    return max_free_cycle;
}

/**
 * Return whether gate with first operand qubit r0 can be scheduled earlier
 * than with operand qubit r1.
 */
utils::Bool FreeCycle::is_first_operand_earlier(utils::UInt r0, utils::UInt r1) const {
    return get_for_qubit(r0) < get_for_qubit(r1);
}

/**
 * Returns whether swap(fr0,fr1) start earlier than a swap(sr0,sr1). Is
 * really a short-cut ignoring config file and perhaps several other
 * details.
 */
utils::Bool FreeCycle::is_first_swap_earliest(
    utils::UInt fr0,
    utils::UInt fr1,
    utils::UInt sr0,
    utils::UInt sr1) const {
    if (options->reverse_swap_if_better) {
        if (get_for_qubit(fr0) < get_for_qubit(fr1)) {
            std::swap(fr0, fr1);
        }
        
        if (get_for_qubit(sr0) < get_for_qubit(sr1)) {
            std::swap(sr0, sr1);
        }
    }

    auto start_cycle_first_swap = utils::max(get_for_qubit(fr0) - 1, get_for_qubit(fr1));
    auto start_cycle_second_swap = utils::max(get_for_qubit(sr0) - 1, get_for_qubit(sr1));

    return start_cycle_first_swap < start_cycle_second_swap;
}

/**
 * Returns what the start cycle would be when we would schedule the given
 * gate, ignoring resource constraints. Purely functional, doesn't affect state.
 */
utils::UInt FreeCycle::get_start_cycle(const ir::CustomInstructionRef &g) const {
    utils::UInt start_cycle = 1;
    for (auto op : g->operands) {
        const auto &ref = op.as<ir::Reference>();
        if (!ref.empty()) {
            start_cycle = utils::max(start_cycle, get_for_reference(*ref));
        }
    }

    // FIXME: this is limited...
    const auto& cond = g->condition;
    const auto &ref = cond.as<ir::Reference>();
    if (!ref.empty()) {
        start_cycle = utils::max(start_cycle, get_for_reference(*ref));
    }

    return start_cycle;
}

/**
 * Schedules the given gate in the FreeCycle map. The FreeCycle map is updated.
 * This is done because add is used to represent just gate
 * dependencies, avoiding a build of a dep graph.
 */
void FreeCycle::add(const ir::CustomInstructionRef &g, utils::UInt startCycle) {
    utils::UInt duration = g->instruction_type->duration;
    utils::UInt freeCycle = startCycle + duration;
    for (auto op : g->operands) {
        const auto &ref = op.as<ir::Reference>();
        if (!ref.empty()) { 
            get_for_reference(*ref) = freeCycle;
        }
    }
}

} // namespace detail
} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
