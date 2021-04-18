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
#include "ql/plat/platform.h"
#include "ql/ir/ir.h"
#include "ql/pass/sch/schedule/detail/scheduler.h"
#include "options.h"
#include "past.h"
#include "alter.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace map {
namespace detail {

// Shorthand.
using Scheduler = pass::sch::schedule::detail::Scheduler;

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
 * operations are provided by the Scheduler class.
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
class Future {
public:

    /**
     * The platform being mapped to.
     */
    plat::PlatformRef platform;

    /**
     * The parsed option structure for the mapping pass.
     */
    OptionsRef options;

    /**
     * A pointer, since dependency graph doesn't change (TODO: document better)
     */
    utils::Ptr<Scheduler> scheduler;

    /**
     * Input circuit when not using scheduler based avlist.
     */
    ir::Circuit input_gatepv;

    /**
     * State: has gate been scheduled, here: done from future?
     */
    utils::Map<ir::GateRef,utils::Bool> scheduled;

    /**
     * State: the nodes/gates which are available for mapping now.
     */
    utils::List<lemon::ListDigraph::Node> avlist;

    /**
     * State: alternative iterator in input_gatepv.
     */
    ir::Circuit::iterator input_gatepp;

    /**
     * Program-wide initialization function.
     */
    void initialize(const plat::PlatformRef &p, const OptionsRef &opt);

    /**
     * Set/switch input to the provided kernel.
     */
    void set_kernel(const ir::KernelRef &kernel, const utils::Ptr<Scheduler> &sched);

    /**
     * Get from avlist all gates that are non-quantum into nonqlg. Non-quantum
     * gates include classical and dummy (SOURCE/SINK). Return whether some
     * non-quantum gate was found.
     */
    utils::Bool get_non_quantum_gates(utils::List<ir::GateRef> &nonqlg) const;

    /**
     * Get all gates from avlist into qlg. Return whether some gate was found.
     */
    utils::Bool get_gates(utils::List<ir::GateRef> &qlg) const;

    /**
     * Indicates that a gate currently in avlist has been mapped, can be
     * taken out of the avlist, and that its successors can be made available.
     */
    void completed_gate(const ir::GateRef &gate);

    /**
     * Return the most critical gate in lag (provided lookahead is enabled).
     * This is used in tiebreak, when every other option has failed to make a
     * distinction.
     */
    ir::GateRef get_most_critical(const utils::List<ir::GateRef> &lag) const;

};

} // namespace detail
} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
