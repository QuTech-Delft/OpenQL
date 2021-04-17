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
namespace route {
namespace detail {

using Scheduler = pass::sch::schedule::detail::Scheduler;

// =========================================================================================
// Future: input window for mapper
//
// The future window shows the gates that still must be mapped as the availability list
// of a list scheduler that would work on a dependence graph representation of each input circuit.
// This future window is initialized once for the whole program, and gets a method call
// when it should switch to a new circuit (corresponding to a new kernel).
// In each circuit and thus each dependence graph the gates (including classical instruction) are found;
// the dependence graph models their dependences and also whether they act as barriers,
// an example of the latter being a classical branch.
// The availability list with gates (including classical instructions) is the main interface
// to the mapper, i.e. the mapper selects one or more element(s) from it to map next;
// it may even create alternatives for each combination of available gates.
// The gates in the list have attributes like criticality, which can be exploited by the mapper.
// The dependence graph and the availability list operations are provided by the Scheduler class.
//
// The future is a window because in principle it could be implemented incrementally,
// i.e. that the dependence graph would be extended when an attribute gets below a threshold,
// e.g. when successors of a gate are interrogated for a particular attribute.
// A problem might be that criticality requires having seen the end of the circuit,
// but the space overhead of this attribute is much less than that of a full dependence graph.
// The implementation below is not incremental: it creates the dep graph for a circuit completely.
//
// The implementation below just selects the most critical gate from the availability list
// as next candidate to map, the idea being that any collateral damage of mapping this gate
// will have a lower probability of increasing circuit depth
// than taking a non-critical gate as first one to map.
// Later implementations may become more sophisticated.
//
// With option maplookaheadopt=="no", the future window's dependence graph (scheduled and avlist) are not used.
// Instead a copy of the input circuit (input_gatepv) is created and iterated over (input_gatepp).

class Future {
public:
    plat::PlatformRef                       platformp;
    OptionsRef                              options;        // parsed mapper pass options
    utils::Ptr<Scheduler>                   schedp;         // a pointer, since dependence graph doesn't change
    ir::Circuit                             input_gatepv;   // input circuit when not using scheduler based avlist

    utils::Map<ir::GateRef,utils::Bool>     scheduled;      // state: has gate been scheduled, here: done from future?
    utils::List<lemon::ListDigraph::Node>   avlist;         // state: which nodes/gates are available for mapping now?
    ir::Circuit::iterator                   input_gatepp;   // state: alternative iterator in input_gatepv

    // just program wide initialization
    void Init(const plat::PlatformRef &p, const OptionsRef &opt);

    // Set/switch input to the provided circuit
    // nq, nc and nb are parameters because nc/nb may not be provided by platform but by kernel
    // the latter should be updated when mapping multiple kernels
    void SetCircuit(const ir::KernelRef &kernel, const utils::Ptr<Scheduler> &sched, utils::UInt nq, utils::UInt nc, utils::UInt nb);

    // Get from avlist all gates that are non-quantum into nonqlg
    // Non-quantum gates include: classical, and dummy (SOURCE/SINK)
    // Return whether some non-quantum gate was found
    utils::Bool GetNonQuantumGates(utils::List<ir::GateRef> &nonqlg) const;

    // Get all gates from avlist into qlg
    // Return whether some gate was found
    utils::Bool GetGates(utils::List<ir::GateRef> &qlg) const;

    // Indicate that a gate currently in avlist has been mapped, can be taken out of the avlist
    // and its successors can be made available
    void DoneGate(const ir::GateRef &gp);

    // Return gp in lag that is most critical (provided lookahead is enabled)
    // This is used in tiebreak, when every other option has failed to make a distinction.
    ir::GateRef MostCriticalIn(const utils::List<ir::GateRef> &lag) const;

};

} // namespace detail
} // namespace route
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
