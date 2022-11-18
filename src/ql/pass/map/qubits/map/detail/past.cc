#include "past.h"

#include "ql/utils/filesystem.h"
#include "ql/ir/describe.h"
#include "ql/ir/ops.h"
#include "ql/ir/swap_parameters.h"
#include "ql/com/map/reference_updater.h"
#include "ql/com/dec/rules.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace map {
namespace detail {

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

void Past::add(const ir::CustomInstructionRef &gate, utils::Any<ir::Statement> *output_gates) {
    auto new_block_for_dec = utils::make<ir::Block>();
    new_block_for_dec->statements.add(gate);

    if (options->decomposition_rule_name_pattern != "") {
        auto predicate = [this](const ir::DecompositionRef &rule) {
            return utils::pattern_match(options->decomposition_rule_name_pattern, rule->name);
        };
        com::dec::apply_decomposition_rules(new_block_for_dec, true, predicate);
    }

    for (const auto &st: new_block_for_dec->statements) {
        auto custom = st.as<ir::CustomInstruction>();

        QL_ASSERT(!custom.empty() && "Decomposition rules for router can only contain gates");

        auto start_cycle = fc.get_start_cycle(custom);
        fc.add(custom, start_cycle);
    }

    if (output_gates) {
        output_gates->add(gate);
    }
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

bool Past::add_move(utils::UInt &r0, utils::UInt &r1, const ir::SwapParameters& swap_params, utils::Any<ir::Statement> *output_gates) {
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

    if (v2r.get_state(r1) == com::map::QubitState::NONE) {
        auto prepz = new_gate("prepz", {r1});
        prepz->set_annotation(swap_params);
        
        if (fc.cycle_extension(prepz) <= options->max_move_penalty) {
            add(prepz, output_gates);
        } else {
            return false;
        }
    }

    auto gname = platform->topology->is_inter_core_hop(r0, r1) ? "tmove" : "move";
    auto move_gate = new_gate(gname, {r0, r1});
    move_gate->set_annotation(swap_params);
    add(move_gate, output_gates);
    return true;
}

void Past::add_swap(utils::UInt r0, utils::UInt r1, utils::Any<ir::Statement> *output_gates) {
    if (v2r.get_state(r0) != com::map::QubitState::LIVE &&
        v2r.get_state(r1) != com::map::QubitState::LIVE) {
        // No state in both operand of intended swap/move; no gate needed.
        v2r.swap(r0, r1);
        return;
    }

    utils::UInt v0 = v2r.get_virtual(r0);
    utils::UInt v1 = v2r.get_virtual(r1);

    const ir::SwapParameters swap_params{true, (utils::Int) r0, (utils::Int) r1, (utils::Int) v1, (utils::Int) v0};

    if (options->use_move_gates &&
            (v2r.get_state(r0) != com::map::QubitState::LIVE ||
             v2r.get_state(r1) != com::map::QubitState::LIVE)) {
        if (add_move(r0, r1, swap_params, output_gates)) {
            num_moves_added++;
            v2r.swap(r0, r1);
            return;
        };
    }

    if (options->reverse_swap_if_better && fc.is_qubit_free_before(r0, r1)) {
        std::swap(r0, r1);
    }

    auto gname = platform->topology->is_inter_core_hop(r0, r1) ? "tswap" : "swap";
    auto newg = new_gate(gname, {r0, r1});
    newg->set_annotation(swap_params);
    add(newg, output_gates);

    num_swaps_added++;
    
    // Reflect in v2r that r0 and r1 interchanged state, i.e. update the map to
    // reflect the swap.
    v2r.swap(r0, r1);
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

    com::map::mapInstruction(platform, v2r.get_virt_to_real(), gate, cb);
}

utils::UInt Past::get_max_free_cycle() const {
    return fc.get_max();
}
} // namespace detail
} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
