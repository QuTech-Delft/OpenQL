/** \file
 * Future implementation.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/ptr.h"
#include "ql/utils/map.h"
#include "ql/utils/vec.h"
#include "ql/utils/list.h"
#include "ql/ir/compat/compat.h"
#include "options.h"
#include "past.h"
#include "alter.h"

namespace ql {
namespace com {
namespace sch {
    class TrivialHeuristic;

    template <typename Heuristic>
    class Scheduler;
}
}

namespace pass {
namespace map {
namespace qubits {
namespace map {
namespace detail {

using Scheduler = com::sch::Scheduler<com::sch::TrivialHeuristic>;

/**
 * Future: input window for mapper.
 *
 * The future window shows the gates that still must be mapped as the
 * availability list of a list scheduler that would work on a dependency graph
 * representation of each input circuit. This future window is initialized once
 * for the whole program, and gets a method call when it should switch to a new
 * circuit (corresponding to a new kernel). In each circuit and thus each
 * dependency graph the gates (including classical instruction) are found; the
 * dependency graph models their dependencies and also whether they act as
 * barriers, an example of the latter being a classical branch. The availability
 * list with gates (including classical instructions) is the main interface to
 * the mapper, i.e. the mapper selects one or more element(s) from it to map
 * next; it may even create alternatives for each combination of available
 * gates. The gates in the list have attributes like criticality, which can be
 * exploited by the mapper. The dependency graph and the availability list
 * operations are provided by com::ddg and com::sch.
 *
 * The future is a window because in principle it could be implemented
 * incrementally, i.e. that the dependence graph would be extended when an
 * attribute gets below a threshold, e.g. when successors of a gate are
 * interrogated for a particular attribute. A problem might be that criticality
 * requires having seen the end of the circuit, but the space overhead of this
 * attribute is much less than that of a full dependence graph. The
 * implementation below is not incremental: it creates the dep graph for a
 * circuit completely.
 *
 * The implementation below just selects the most critical gate from the
 * availability list as next candidate to map, the idea being that any
 * collateral damage of mapping this gate will have a lower probability of
 * increasing circuit depth than taking a non-critical gate as first one to map.
 * Later implementations may become more sophisticated.
 *
 * With the lookahead_mode option disabled, the future window's dependency
 * graphs (scheduled and avlist) are not used. Instead, a copy of the input
 * circuit (input_gatepv) is created and iterated over (input_gatepp).
 */
class GateIterator;
class Future {
public:
    Future(const ir::PlatformRef &p, const OptionsRef &opt, const ir::BlockBaseRef &block);

    /**
     * Get all gates from avlist into qlg. Return whether some gate was found.
     */
    utils::List<ir::CustomInstructionRef> get_schedulable_gates() const;

    /**
     * Indicates that a gate currently in avlist has been mapped, can be
     * taken out of the avlist, and that its successors can be made available.
     */
    void completed_gate(const ir::CustomInstructionRef &gate);

    /**
     * Return the most critical gate in lag (provided lookahead is enabled).
     * This is used in tiebreak, when every other option has failed to make a
     * distinction.
     */
    ir::CustomInstructionRef get_most_critical(const std::vector<ir::CustomInstructionRef> &lag) const;

    utils::Real get_progress() {
        utils::Real progress = 1.0;
        if (approx_gates_total) {
            progress -= ((utils::Real)approx_gates_remaining / (utils::Real)approx_gates_total);
        }

        return progress;
    }

private:
    /**
     * The platform being mapped to.
     */
    ir::PlatformRef platform;

    /**
     * The parsed option structure for the mapping pass.
     */
    OptionsRef options;

    std::shared_ptr<GateIterator> gateIterator;

    /**
     * Approximate total number of gates to begin with.
     */
    utils::UInt approx_gates_total = 0;

    /**
     * Approximate total number of gates remaining.
     */
    utils::UInt approx_gates_remaining = 0;
};

} // namespace detail
} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
