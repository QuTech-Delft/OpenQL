/** \file
 * Future implementation.
 */

#include "future.h"

#include "ql/utils/filesystem.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace map {
namespace detail {

/**
 * Program-wide initialization function.
 */
void Future::initialize(const plat::PlatformRef &p, const OptionsRef &opt) {
    // QL_DOUT("Future::Init ...");
    platform = p;
    options = opt;
    // QL_DOUT("Future::Init [DONE]");
}

/**
 * Set/switch input to the provided kernel.
 */
void Future::set_kernel(const ir::KernelRef &kernel, const utils::Ptr<Scheduler> &sched) {
    QL_DOUT("Future::set_kernel ...");
    approx_gates_total = kernel->gates.size();
    approx_gates_remaining = approx_gates_total;
    scheduler = sched;
    if (options->lookahead_mode == LookaheadMode::DISABLED) {
        input_gatepv = kernel->gates;                           // copy to free original circuit to allow outputing to
        input_gatepp = input_gatepv.begin();                    // iterator set to start of input circuit copy
    } else {
        scheduler->init(
            kernel,
            options->output_prefix,
            options->commute_multi_qubit,
            options->commute_single_qubit,
            options->enable_criticality
        );

        // and so also the original circuit can be output to after this
        for (auto &gp : kernel->gates) {
            scheduled.set(gp) = false;   // none were scheduled
        }
        scheduled.set(scheduler->instruction[scheduler->s]) = false;      // also the dummy nodes not
        scheduled.set(scheduler->instruction[scheduler->t]) = false;
        avlist.clear();
        avlist.push_back(scheduler->s);
        scheduler->set_remaining(rmgr::Direction::FORWARD);          // to know criticality

        if (options->write_dot_graphs) {
            utils::Str map_dot;
            utils::StrStrm fname;

            scheduler->get_dot(map_dot);

            fname << options->output_prefix << kernel->name << "_" << "mapper" << ".dot";
            QL_IOUT("writing " << "mapper" << " dependence graph dot file to '" << fname.str() << "' ...");
            utils::OutFile(fname.str()).write(map_dot);
        }
    }
    QL_DOUT("Future::set_kernel [DONE]");
}

/**
 * Get from avlist all gates that are non-quantum into nonqlg. Non-quantum
 * gates include classical and dummy (SOURCE/SINK). Return whether some
 * non-quantum gate was found.
 */
utils::Bool Future::get_non_quantum_gates(utils::List<ir::GateRef> &nonqlg) const {
    nonqlg.clear();
    if (options->lookahead_mode == LookaheadMode::DISABLED) {
        ir::GateRef gate = *input_gatepp;
        if (ir::GateRefs::const_iterator(input_gatepp) != input_gatepv.end()) {
            if (
                gate->type() == ir::GateType::CLASSICAL
                || gate->type() == ir::GateType::DUMMY
            ) {
                nonqlg.push_back(gate);
            }
        }
    } else {
        for (auto n : avlist) {
            ir::GateRef gate = scheduler->instruction[n];
            if (
                gate->type() == ir::GateType::CLASSICAL
                || gate->type() == ir::GateType::DUMMY
            ) {
                nonqlg.push_back(gate);
            }
        }
    }
    return !nonqlg.empty();
}

/**
 * Get all gates from avlist into qlg. Return whether some gate was found.
 */
utils::Bool Future::get_gates(utils::List<ir::GateRef> &qlg) const {
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
            ir::GateRef gp = scheduler->instruction[n];
            if (gp->operands.size() > 2) {
                QL_FATAL(" gate: " << gp->qasm() << " has more than 2 operand qubits; please decompose such gates first before mapping.");
            }
            qlg.push_back(gp);
        }
    }
    return !qlg.empty();
}

/**
 * Indicates that a gate currently in avlist has been mapped, can be
 * taken out of the avlist, and that its successors can be made available.
 */
void Future::completed_gate(const ir::GateRef &gate) {
    if (approx_gates_remaining) {
        approx_gates_remaining--;
    }
    if (options->lookahead_mode == LookaheadMode::DISABLED) {
        input_gatepp = std::next(input_gatepp);
    } else {
        scheduler->take_available(scheduler->node.at(gate), avlist, scheduled, rmgr::Direction::FORWARD);
    }
}

/**
 * Return the most critical gate in lag (provided lookahead is enabled).
 * This is used in tiebreak, when every other option has failed to make a
 * distinction.
 */
ir::GateRef Future::get_most_critical(const utils::List<ir::GateRef> &lag) const {
    if (options->lookahead_mode == LookaheadMode::DISABLED) {
        return lag.front();
    } else {
        return scheduler->find_mostcritical(lag);
    }
}

} // namespace detail
} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
