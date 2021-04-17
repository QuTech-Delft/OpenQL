/** \file
 * Future implementation.
 */

#include "future.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace route {
namespace detail {

using namespace utils;
using namespace com;

// just program wide initialization
void Future::Init(const plat::PlatformRef &p, const OptionsRef &opt) {
    // QL_DOUT("Future::Init ...");
    platformp = p;
    options = opt;
    // QL_DOUT("Future::Init [DONE]");
}

// Set/switch input to the provided circuit
// nq, nc and nb are parameters because nc/nb may not be provided by platform but by kernel
// the latter should be updated when mapping multiple kernels
void Future::SetCircuit(const ir::KernelRef &kernel, const utils::Ptr<Scheduler> &sched, UInt nq, UInt nc, UInt nb) {
    QL_DOUT("Future::SetCircuit ...");
    schedp = sched;
    if (options->lookahead_mode == LookaheadMode::DISABLED) {
        input_gatepv = kernel->c;                               // copy to free original circuit to allow outputing to
        input_gatepp = input_gatepv.begin();                    // iterator set to start of input circuit copy
    } else {
        schedp->init(
            kernel,
            options->output_prefix,
            options->commute_multi_qubit,
            options->commute_single_qubit
        );

        // and so also the original circuit can be output to after this
        for (auto &gp : kernel->c) {
            scheduled.set(gp) = false;   // none were scheduled
        }
        scheduled.set(schedp->instruction[schedp->s]) = false;      // also the dummy nodes not
        scheduled.set(schedp->instruction[schedp->t]) = false;
        avlist.clear();
        avlist.push_back(schedp->s);
        schedp->set_remaining(plat::resource::Direction::FORWARD);          // to know criticality

        if (options->print_dot_graphs) {
            Str map_dot;
            StrStrm fname;

            schedp->get_dot(map_dot);

            fname << options->output_prefix << kernel->name << "_" << "mapper" << ".dot";
            QL_IOUT("writing " << "mapper" << " dependence graph dot file to '" << fname.str() << "' ...");
            OutFile(fname.str()).write(map_dot);
        }
    }
    QL_DOUT("Future::SetCircuit [DONE]");
}

// Get from avlist all gates that are non-quantum into nonqlg
// Non-quantum gates include: classical, and dummy (SOURCE/SINK)
// Return whether some non-quantum gate was found
Bool Future::GetNonQuantumGates(List<ir::GateRef> &nonqlg) const {
    nonqlg.clear();
    if (options->lookahead_mode == LookaheadMode::DISABLED) {
        ir::GateRef gp = *input_gatepp;
        if (ir::Circuit::const_iterator(input_gatepp) != input_gatepv.end()) {
            if (
                gp->type() == ir::GateType::CLASSICAL
                || gp->type() == ir::GateType::DUMMY
            ) {
                nonqlg.push_back(gp);
            }
        }
    } else {
        for (auto n : avlist) {
            ir::GateRef gp = schedp->instruction[n];
            if (
                gp->type() == ir::GateType::CLASSICAL
                || gp->type() == ir::GateType::DUMMY
            ) {
                nonqlg.push_back(gp);
            }
        }
    }
    return !nonqlg.empty();
}

// Get all gates from avlist into qlg
// Return whether some gate was found
Bool Future::GetGates(List<ir::GateRef> &qlg) const {
    qlg.clear();
    if (options->lookahead_mode == LookaheadMode::DISABLED) {
        if (input_gatepp != input_gatepv.end()) {
            ir::GateRef gp = *input_gatepp;
            if (gp->operands.size() > 2) {
                QL_FATAL(" gate: " << gp->qasm() << " has more than 2 operand qubits; please decompose such gates first before mapping.");
            }
            qlg.push_back(gp);
        }
    } else {
        for (auto n : avlist) {
            ir::GateRef gp = schedp->instruction[n];
            if (gp->operands.size() > 2) {
                QL_FATAL(" gate: " << gp->qasm() << " has more than 2 operand qubits; please decompose such gates first before mapping.");
            }
            qlg.push_back(gp);
        }
    }
    return !qlg.empty();
}

// Indicate that a gate currently in avlist has been mapped, can be taken out of the avlist
// and its successors can be made available
void Future::DoneGate(const ir::GateRef &gp) {
    if (options->lookahead_mode == LookaheadMode::DISABLED) {
        input_gatepp = std::next(input_gatepp);
    } else {
        schedp->take_available(schedp->node.at(gp), avlist, scheduled,
                               plat::resource::Direction::FORWARD);
    }
}

// Return gp in lag that is most critical (provided lookahead is enabled)
// This is used in tiebreak, when every other option has failed to make a distinction.
ir::GateRef Future::MostCriticalIn(const List<ir::GateRef> &lag) const {
    if (options->lookahead_mode == LookaheadMode::DISABLED) {
        return lag.front();
    } else {
        return schedp->find_mostcritical(lag);
    }
}

} // namespace detail
} // namespace route
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
