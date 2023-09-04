#pragma once

#include "options.h"
#include "past.h"
#include "alter.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace map {
namespace detail {

class GateIterator;

/**
 * Future: input window for mapper.
 *
 * The future window shows the gates that remain to be be mapped in that block.
 * The order in which the gates are routed is either linear, following circuit order,
 * or by topological order. The dependency graph is provided by com::ddg.
 *
 */

class Future {
public:
    Future(const ir::PlatformRef &p, const OptionsRef &opt, const ir::BlockBaseRef &block);

    Future(const Future& rhs);

    ~Future();

    /**
     * Get all gates whose routing should be attempted next.
     */
    utils::List<ir::CustomInstructionRef> get_schedulable_gates() const;

    /**
     * Indicates that a gate obtained by get_schedulable_gates() has been mapped, can be
     * taken out of the remaining gates, and that its successor(s) can be made available.
     */
    void completed_gate(const ir::CustomInstructionRef &gate);

    /**
     * Return the most critical gate in lag (provided lookahead is enabled).
     * This is used in tiebreak, when every other option has failed to make a
     * distinction.
     */
    ir::CustomInstructionRef get_most_critical(const std::vector<ir::CustomInstructionRef> &gates) const;

    utils::Real get_progress();

private:
    ir::PlatformRef platform;
    OptionsRef options;

    /**
     * The gate iterator (circuit or topological order) used to obtain the next gate to route/map.
     */
    std::unique_ptr<GateIterator> gateIterator;

    /**
     * Initial number of gates to process (to know progress).
     */
    utils::UInt approx_gates_total = 0;

    /**
     * Approximate total number of gates remaining (to know progress).
     */
    utils::UInt approx_gates_remaining = 0;
};

} // namespace detail
} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
