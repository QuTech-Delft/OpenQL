/** \file
 * Past implementation.
 */

#include "past.h"

#include "ql/utils/filesystem.h"
#include "ql/ir/describe.h"
#include "ql/ir/ops.h"
#include "ql/com/map/reference_updater.h"

// uncomment next line to enable multi-line dumping
// #define MULTI_LINE_LOG_DEBUG

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace map {
namespace detail {

/**
 * Past initializer.
 */
Past::Past(ir::PlatformRef p, const OptionsRef &opt) {
    platform = p;
    options = opt;

    v2r.resize(
        platform->qubits->shape[0],
        true,
        options->assume_initialized ? com::map::QubitState::INITIALIZED : com::map::QubitState::NONE
    );
    fc.initialize(platform, options);
}

void Past::import_mapping(const com::map::QubitMapping &v2r_value) {
    v2r = v2r_value;
}

void Past::export_mapping(com::map::QubitMapping &v2r_destination) const {
    v2r_destination = v2r;
}

void Past::schedule() {
    // Assumes that gates in waiting_gates are in topological order.

    for (const auto& gate: waiting_gates) {
        auto start_cycle = fc.get_start_cycle(gate);
        fc.add(gate, start_cycle);
        cycle.set(gate) = start_cycle;
        
        auto comp = [this](const ir::CustomInstructionRef &g1, const ir::CustomInstructionRef &g2) {
            return cycle.get(g1) < cycle.get(g2);
        };
        auto insertion_location = std::upper_bound(output_gates.begin(), output_gates.end(), gate, comp);
        output_gates.insert(insertion_location, gate);
    }

    waiting_gates.clear();
}

void Past::add(const ir::CustomInstructionRef &gate) {
    waiting_gates.push_back(gate);
}

ir::Maybe<ir::CustomInstruction> Past::new_gate(
    const utils::Str &gname,
    const utils::Vec<utils::UInt> &qubits
) const {
    utils::Any<ir::Expression> operands;
    for (auto q: qubits) {
        operands.add(make_qubit_ref(platform, q));
    }

    auto insn = make_instruction(platform, gname, operands); // FIXME: return {} if instr does not exist? Currently throws
    return insn.as<ir::CustomInstruction>();
}

utils::UInt Past::get_num_swaps_added() const {
    return num_swaps_added;
}

utils::UInt Past::get_num_moves_added() const {
    return num_moves_added;
}

utils::Bool Past::is_first_swap_earliest(
    utils::UInt fr0,
    utils::UInt fr1,
    utils::UInt sr0,
    utils::UInt sr1
) const {
    return fc.is_first_swap_earliest(fr0, fr1, sr0, sr1);
}

void Past::add_move(utils::UInt &r0, utils::UInt &r1) {
    if (v2r.get_state(r0) != com::map::QubitState::LIVE) {
        QL_ASSERT(
            v2r.get_state(r0) == com::map::QubitState::NONE ||
            v2r.get_state(r0) == com::map::QubitState::INITIALIZED
        );

        // Interchange r0 and r1, so that r1 (right-hand operand of move) will
        // be the state-less one.
        std::swap(r0, r1);
    }

    QL_ASSERT(v2r.get_state(r0) == com::map::QubitState::LIVE);    // and r0 will be the one with state
    QL_ASSERT(v2r.get_state(r1) != com::map::QubitState::LIVE);    // and r1 will be the one without state (QubitState::NONE || com::QubitState::INITIALIZED)

    // First (optimistically) create the move circuit and add it to circuit.
    auto gname = platform->topology->is_inter_core_hop(r0, r1) ? "tmove" : "move";
    auto newg = new_gate(gname, {r0, r1});
    add(newg);

    // TODO: move + prep?
}

/**
 * Generates a single swap/move with real operands and adds it to the
 * current past's waiting list. Note that the swap/move may be implemented
 * by a series of gates (circuit circ below), and that a swap/move
 * essentially is a commutative operation, interchanging the states of the
 * two qubits.
 *
 * A move is implemented by 2 CNOTs, while a swap is 3 CNOTs, provided the
 * target qubit is in |0> (inited) state. So, when one of the operands is
 * the current location of an unused virtual qubit, use a move with that
 * location as 2nd operand, after first having initialized the target qubit
 * in |0> (inited) state when that has not been done already. However, this
 * initialization must not extend the depth (beyond the configured limit),
 * so this can only be done when cycles for it are for free.
 */
void Past::add_swap(utils::UInt r0, utils::UInt r1) {
    if (v2r.get_state(r0) != com::map::QubitState::LIVE &&
        v2r.get_state(r1) != com::map::QubitState::LIVE) {
        // No state in both operand of intended swap/move; no gate needed.
        v2r.swap(r0, r1);
        return;
    }

    utils::UInt v0 = v2r.get_virtual(r0);
    utils::UInt v1 = v2r.get_virtual(r1);

    const ir::compat::SwapParamaters swap_params {true, (utils::Int) r0, (utils::Int) r1, (utils::Int) v1, (utils::Int) v0};

    if (options->use_move_gates &&
            (v2r.get_state(r0) != com::map::QubitState::LIVE ||
             v2r.get_state(r1) != com::map::QubitState::LIVE)) {
        add_move(r0, r1);
        num_moves_added++;
        v2r.swap(r0, r1);
        // TODO: set_annotation(swap_params);

        return;
    }

    if (options->reverse_swap_if_better && fc.is_first_operand_earlier(r0, r1)) {
        std::swap(r0, r1);
    }

    auto gname = platform->topology->is_inter_core_hop(r0, r1) ? "tswap" : "swap";
    auto newg = new_gate(gname, {r0, r1});
    newg->set_annotation(swap_params);
    add(newg);

    num_swaps_added++;
    
    // Reflect in v2r that r0 and r1 interchanged state, i.e. update the map to
    // reflect the swap.
    v2r.swap(r0, r1);
}

void Past::add_and_schedule(const ir::CustomInstructionRef &gate) {
    add(gate);
    schedule();
}

utils::UInt Past::get_real_qubit(utils::UInt virt) {
    utils::UInt r = v2r[virt];
    QL_ASSERT(r != com::map::UNDEFINED_QUBIT);
    return r;
}

void Past::make_real(const ir::CustomInstructionRef &gate) {
    const auto &gname = gate->instruction_type->name;
    com::map::ReferenceUpdater::Callback cb = [this, gname](utils::UInt virtual_qubit) {
        if (options->assume_prep_only_initializes && (gname == "prepz" || gname == "Prepz")) {
            v2r.set_state(virtual_qubit, com::map::QubitState::INITIALIZED);
        } else {
            v2r.set_state(virtual_qubit, com::map::QubitState::LIVE);
        }
    };

    com::map::ReferenceUpdater referenceUpdater(platform, v2r.get_virt_to_real(), cb);
    gate->visit(referenceUpdater);
}

utils::UInt Past::get_max_free_cycle() const {
    return fc.get_max();
}

utils::Any<ir::Statement> Past::flush_to_circuit() {
    utils::UInt cycle = 0;
    utils::Any<ir::Statement> output_circuit;
    for (const auto &gate : output_gates) {
        gate->cycle = cycle++;
        output_circuit.add(gate);
    }
    return output_circuit;
}

} // namespace detail
} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
