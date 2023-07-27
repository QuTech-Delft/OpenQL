/** \file
 * Quantum kernel abstraction implementation.
 */

#include "ql/ir/compat/kernel.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <string>

#include "ql/config.h"
#include "ql/utils/json.h"
#include "ql/utils/str.h"
#include "ql/utils/vec.h"
#include "ql/ir/compat/platform.h"
#include "ql/ir/compat/gate.h"
#include "ql/ir/compat/classical.h"
#include "ql/ir/compat/bundle.h"
#include "ql/com/options.h"
#include "ql/com/dec/unitary.h"

// uncomment next line to enable multi-line dumping
// #define MULTI_LINE_LOG_DEBUG

namespace ql {
namespace ir {
namespace compat {

using namespace utils;

/**
 * Generates cQASM for a given circuit.
 */
Str qasm(const GateRefs &c) {
    StrStrm ss;
    for (auto gate : c) {
        ss << gate->qasm() << "\n";
    }
    return ss.str();
}

Kernel::Kernel(
    const Str &name,
    const ir::compat::PlatformRef &platform,
    UInt qubit_count,
    UInt creg_count,
    UInt breg_count
) :
    name(name),
    platform(platform),
    qubit_count(qubit_count),
    creg_count(creg_count),
    breg_count(breg_count),
    type(KernelType::STATIC),
    iteration_count(1),
    cycles_valid(true),
    condition(ConditionType::ALWAYS)
{
    if (qubit_count > platform->qubit_count) {
        QL_USER_ERROR(
            "cannot create kernel (" << name << ") " <<
            "that uses more qubits (" << qubit_count << ") " <<
            "than the platform has (" << platform->qubit_count << ")"
        );
    }
    if (creg_count > platform->creg_count) {
        if (platform->compat_implicit_creg_count) {
            platform->creg_count = creg_count;
        } else {
            QL_USER_ERROR(
                "cannot create kernel (" << name << ") " <<
                "that uses more cregs (" << creg_count << ") " <<
                "than the platform has (" << platform->creg_count << ")"
            );
        }
    }
    if (breg_count > platform->breg_count) {
        if (platform->compat_implicit_breg_count) {
            platform->breg_count = breg_count;
        } else {
            QL_USER_ERROR(
                "cannot create kernel (" << name << ") " <<
                "that uses more bregs (" << breg_count << ") "
                "than the platform has (" << platform->breg_count << ")"
            );
        }
    }
}

void Kernel::set_condition(const ClassicalOperation &oper) {
    if ((oper.operands[0])->as_register().id >= creg_count || (oper.operands[1]->as_register().id >= creg_count)) {
        QL_USER_ERROR("operand(s) out of range for '" << oper.operation_name << "'");
    }

    if (oper.operation_type != ClassicalOperationType::RELATIONAL) {
        QL_USER_ERROR(
            "condition requires relational operator, '" <<
            oper.operation_name << "' is not supported"
        );
    }

    br_condition = oper;
}

void Kernel::set_kernel_type(KernelType typ) {
    type = typ;
}

Str Kernel::get_gates_definition() const {
    StrStrm ss;

    for (auto i = platform->instruction_map.begin(); i != platform->instruction_map.end(); i++) {
        ss << i->first << std::endl;
    }
    return ss.str();
}

Str Kernel::get_name() const {
    return name;
}

void Kernel::identity(UInt qubit) {
    gate("identity", qubit);
}

void Kernel::i(UInt qubit) {
    gate("identity", qubit);
}

void Kernel::hadamard(UInt qubit) {
    gate("hadamard", qubit);
}

void Kernel::h(UInt qubit) {
    hadamard(qubit);
}

void Kernel::rx(UInt qubit, Real angle) {
    gates.emplace<gate_types::RX>(qubit, angle);
    gates.back()->condition = condition;
    gates.back()->cond_operands = cond_operands;;
    cycles_valid = false;
}

void Kernel::ry(UInt qubit, Real angle) {
    gates.emplace<gate_types::RY>(qubit, angle);
    gates.back()->condition = condition;
    gates.back()->cond_operands = cond_operands;;
    cycles_valid = false;
}

void Kernel::rz(UInt qubit, Real angle) {
    gates.emplace<gate_types::RZ>(qubit, angle);
    gates.back()->condition = condition;
    gates.back()->cond_operands = cond_operands;;
    cycles_valid = false;
}

void Kernel::s(UInt qubit) {
    gate("s", qubit);
}

void Kernel::sdag(UInt qubit) {
    gate("sdag", qubit);
}

void Kernel::t(UInt qubit) {
    gate("t", qubit);
}

void Kernel::tdag(UInt qubit) {
    gate("tdag", qubit);
}

void Kernel::x(UInt qubit) {
    gate("x", qubit);
}

void Kernel::y(UInt qubit) {
    gate("y", qubit);
}

void Kernel::z(UInt qubit) {
    gate("z", qubit);
}

void Kernel::rx90(UInt qubit) {
    gate("rx90", qubit);
}

void Kernel::mrx90(UInt qubit) {
    gate("mrx90", qubit);
}

void Kernel::rx180(UInt qubit) {
    gate("rx180", qubit);
}

void Kernel::ry90(UInt qubit) {
    gate("ry90", qubit);
}

void Kernel::mry90(UInt qubit) {
    gate("mry90", qubit);
}

void Kernel::ry180(UInt qubit) {
    gate("ry180", qubit);
}

void Kernel::measure(UInt qubit) {
    gate("measure", {qubit}, {}, 0, 0.0, {});
}

void Kernel::measure(UInt qubit, UInt bit) {
    gate("measure", {qubit}, {}, 0, 0.0, {bit});
}

void Kernel::prepz(UInt qubit) {
    gate("prepz", qubit);
}

void Kernel::cnot(UInt qubit1, UInt qubit2) {
    gate("cnot", {qubit1, qubit2});
}

void Kernel::cz(UInt qubit1, UInt qubit2) {
    gate("cz", {qubit1, qubit2});
}

void Kernel::cphase(UInt qubit1, UInt qubit2) {
    gate("cphase", {qubit1, qubit2});
}

void Kernel::toffoli(UInt qubit1, UInt qubit2, UInt qubit3) {
    // TODO add custom gate check if needed
    gates.emplace<gate_types::Toffoli>(qubit1, qubit2, qubit3);
    gates.back()->condition = condition;
    gates.back()->cond_operands = cond_operands;;
    cycles_valid = false;
}

void Kernel::swap(UInt qubit1, UInt qubit2) {
    gate("swap", {qubit1, qubit2});
}

void Kernel::wait(const Vec<UInt> &qubits, UInt duration) {
    gate("wait", qubits, {}, duration);
}

void Kernel::display() {
    gates.emplace<gate_types::Display>();
    cycles_valid = false;
}

void Kernel::clifford(Int id, UInt qubit) {
    switch (id) {
        case 0 :
            break;              //  ['I']
        case 1 :
            ry90(qubit);
            rx90(qubit);
            break;              //  ['Y90', 'X90']
        case 2 :
            mrx90(qubit);
            mry90(qubit);
            break;              //  ['mX90', 'mY90']
        case 3 :
            rx180(qubit);
            break;              //  ['X180']
        case 4 :
            mry90(qubit);
            mrx90(qubit);
            break;              //  ['mY90', 'mX90']
        case 5 :
            rx90(qubit);
            mry90(qubit);
            break;              //  ['X90', 'mY90']
        case 6 :
            ry180(qubit);
            break;              //  ['Y180']
        case 7 :
            mry90(qubit);
            rx90(qubit);
            break;              //  ['mY90', 'X90']
        case 8 :
            rx90(qubit);
            ry90(qubit);
            break;              //  ['X90', 'Y90']
        case 9 :
            rx180(qubit);
            ry180(qubit);
            break;              //  ['X180', 'Y180']
        case 10:
            ry90(qubit);
            mrx90(qubit);
            break;              //  ['Y90', 'mX90']
        case 11:
            mrx90(qubit);
            ry90(qubit);
            break;              //  ['mX90', 'Y90']
        case 12:
            ry90(qubit);
            rx180(qubit);
            break;              //  ['Y90', 'X180']
        case 13:
            mrx90(qubit);
            break;              //  ['mX90']
        case 14:
            rx90(qubit);
            mry90(qubit);
            mrx90(qubit);
            break;              //  ['X90', 'mY90', 'mX90']
        case 15:
            mry90(qubit);
            break;              //  ['mY90']
        case 16:
            rx90(qubit);
            break;              //  ['X90']
        case 17:
            rx90(qubit);
            ry90(qubit);
            rx90(qubit);
            break;              //  ['X90', 'Y90', 'X90']
        case 18:
            mry90(qubit);
            rx180(qubit);
            break;              //  ['mY90', 'X180']
        case 19:
            rx90(qubit);
            ry180(qubit);
            break;              //  ['X90', 'Y180']
        case 20:
            rx90(qubit);
            mry90(qubit);
            rx90(qubit);
            break;              //  ['X90', 'mY90', 'X90']
        case 21:
            ry90(qubit);
            break;              //  ['Y90']
        case 22:
            mrx90(qubit);
            ry180(qubit);
            break;              //  ['mX90', 'Y180']
        case 23:
            rx90(qubit);
            ry90(qubit);
            mrx90(qubit);
            break;              //  ['X90', 'Y90', 'mX90']
        default:
            break;
    }
}

Bool Kernel::add_default_gate_if_available(
    const Str &gname,
    const Vec<UInt> &qubits,
    const Vec<UInt> &cregs,
    UInt duration,
    Real angle,
    const Vec<UInt> &bregs,
    ConditionType gcond,
    const Vec<UInt> &gcondregs
) {
    Bool result = false;

    Bool is_one_qubit_gate = (gname == "identity") || (gname == "i")
                             || (gname == "hadamard") || (gname == "h")
                             || (gname == "pauli_x") || (gname == "pauli_y") || (gname == "pauli_z")
                             || (gname == "x") || (gname == "y") || (gname == "z")
                             || (gname == "s") || (gname == "sdag")
                             || (gname == "t") || (gname == "tdag")
                             || (gname == "rx") || (gname == "ry") || (gname == "rz")
                             || (gname == "rx90") || (gname == "mrx90") || (gname == "rx180")
                             || (gname == "ry90") || (gname == "mry90") || (gname == "ry180")
                             || (gname == "measure") || (gname == "prepz");

    Bool is_two_qubit_gate = (gname == "cnot")
                             || (gname == "cz") || (gname == "cphase")
                             || (gname == "swap");

    Bool is_multi_qubit_gate = (gname == "toffoli")
                               || (gname == "wait") || (gname == "barrier");
    Bool is_non_conditional_gate = (gname == "wait") || (gname == "barrier");

    if (is_one_qubit_gate) {
        if (qubits.size() != 1) {
            return false;
        }
    } else if (is_two_qubit_gate) {
        if (qubits.size() != 2) {
            return false;
        }
        if (qubits[0] == qubits[1]) {
            return false;
        }
    } else if (!is_multi_qubit_gate) {
        return false;
    }

    if (gname == "identity" || gname == "i") {
        gates.emplace<gate_types::Identity>(qubits[0]);
        result = true;
    } else if (gname == "hadamard" || gname == "h") {
        gates.emplace<gate_types::Hadamard>(qubits[0]);
        result = true;
    } else if (gname == "pauli_x" || gname == "x") {
        gates.emplace<gate_types::PauliX>(qubits[0]);
        result = true;
    } else if( gname == "pauli_y" || gname == "y") {
        gates.emplace<gate_types::PauliY>(qubits[0]);
        result = true;
    } else if (gname == "pauli_z" || gname == "z") {
        gates.emplace<gate_types::PauliZ>(qubits[0]);
        result = true;
    } else if (gname == "s" || gname == "phase") {
        gates.emplace<gate_types::Phase>(qubits[0]);
        result = true;
    } else if (gname == "sdag" || gname == "phasedag") {
        gates.emplace<gate_types::PhaseDag>(qubits[0]);
        result = true;
    } else if (gname == "t") {
        gates.emplace<gate_types::T>(qubits[0]);
        result = true;
    } else if (gname == "tdag") {
        gates.emplace<gate_types::TDag>(qubits[0]);
        result = true;
    } else if (gname == "rx") {
        gates.emplace<gate_types::RX>(qubits[0], angle);
        result = true;
    } else if (gname == "ry") {
        gates.emplace<gate_types::RY>(qubits[0], angle);
        result = true;
    } else if( gname == "rz") {
        gates.emplace<gate_types::RZ>(qubits[0], angle);
        result = true;
    } else if (gname == "rx90") {
        gates.emplace<gate_types::RX90>(qubits[0]);
        result = true;
    } else if (gname == "mrx90") {
        gates.emplace<gate_types::MRX90>(qubits[0]);
        result = true;
    } else if (gname == "rx180") {
        gates.emplace<gate_types::RX180>(qubits[0]);
        result = true;
    } else if (gname == "ry90") {
        gates.emplace<gate_types::RY90>(qubits[0]);
        result = true;
    } else if (gname == "mry90") {
        gates.emplace<gate_types::MRY90>(qubits[0]);
        result = true;
    } else if (gname == "ry180") {
        gates.emplace<gate_types::RY180>(qubits[0]);
        result = true;
    } else if (gname == "measure") {
        if (cregs.empty()) {
            gates.emplace<gate_types::Measure>(qubits[0]);
        } else {
            gates.emplace<gate_types::Measure>(qubits[0], cregs[0]);
        }
        result = true;
    } else if (gname == "prepz") {
        gates.emplace<gate_types::PrepZ>(qubits[0]);
        result = true;
    } else if (gname == "cnot") {
        gates.emplace<gate_types::CNot>(qubits[0], qubits[1]);
        result = true;
    } else if (gname == "cz" || gname == "cphase") {
        gates.emplace<gate_types::CPhase>(qubits[0], qubits[1]);
        result = true;
    } else if (gname == "toffoli") {
        gates.emplace<gate_types::Toffoli>(qubits[0], qubits[1], qubits[2]);
        result = true;
    } else if (gname == "swap") {
        gates.emplace<gate_types::Swap>(qubits[0], qubits[1]);
        result = true;
    } else if (gname == "barrier") {
        /*
        wait/barrier is applied on the qubits specified as arguments.
        if no qubits are specified, then wait/barrier is applied on all qubits
        */
        if (qubits.empty()) {
            Vec<UInt> all_qubits;
            for (UInt q = 0; q < qubit_count; q++) {
                all_qubits.push_back(q);
            }
            gates.emplace<gate_types::Wait>(all_qubits, 0, 0);
        } else {
            gates.emplace<gate_types::Wait>(qubits, 0, 0);
        }
        result = true;
    } else if (gname == "wait") {
        /*
        wait/barrier is applied on the qubits specified as arguments.
        if no qubits are specified, then wait/barrier is applied on all qubits
        */
        UInt duration_in_cycles = ceil(static_cast<float>(duration) / platform->cycle_time);
        if (qubits.empty()) {
            Vec<UInt> all_qubits;
            for (UInt q = 0; q < qubit_count; q++) {
                all_qubits.push_back(q);
            }
            gates.emplace<gate_types::Wait>(all_qubits, duration, duration_in_cycles);
        } else {
            gates.emplace<gate_types::Wait>(qubits, duration, duration_in_cycles);
        }
        result = true;
    } else {
        result = false;
    }

    if (result) {
        gates.back()->breg_operands = bregs;
        if (gcond != ConditionType::ALWAYS && is_non_conditional_gate ) {
            QL_WOUT("Condition " << gcond << " on default gate '" << gname << "' specified while gate cannot be executed conditionally; condition will be ignored");
            gates.back()->condition = ConditionType::ALWAYS;
            gates.back()->cond_operands = {};
        } else {
            gates.back()->condition = gcond;
            gates.back()->cond_operands = gcondregs;
        }
        cycles_valid = false;
    }

    return result;
}

// if a specialized custom gate ("e.g. cz q0,q4") is available, add it to circuit and return true
// if a parameterized custom gate ("e.g. cz") is available, add it to circuit and return true
//
// note that there is no check for the found gate being a composite gate
Bool Kernel::add_custom_gate_if_available(
    const Str &gname,
    const Vec<UInt> &qubits,
    const Vec<UInt> &cregs,
    UInt duration,
    Real angle,
    const Vec<UInt> &bregs,
    ConditionType gcond,
    const Vec<UInt> &gcondregs
) {
#ifdef OPT_DECOMPOSE_WAIT_BARRIER  // hack to skip wait/barrier
    if (gname=="wait" || gname=="barrier") {
        return false;   // return, so a default gate will be attempted
    }
#endif
    // construct canonical name
    Str instr = "";
    for (auto qubit : qubits) {
        if (!instr.empty()) {
            instr += ",";
        }
        instr += std::string{"q"} + to_string(qubit);
    }
    instr = gname + " " + instr;

    // first check if a specialized custom gate is available
    // a specialized custom gate is of the form: "cz q0 q3"
    // QL_DOUT("is specialized custom gate available for for instr " << instr);
    auto it = platform->instruction_map.find(instr);
    if (it == platform->instruction_map.end()) {
        // QL_DOUT("is parameterized custom gate available for gate " << gname);
        it = platform->instruction_map.find(gname);
    }
    if (it == platform->instruction_map.end()) {
        QL_DOUT("custom gate not added for " << gname);
        return false;
    }

    auto g = GateRef::make<gate_types::Custom>(*(it->second));
    g->operands.clear();
    for (auto qubit : qubits) {
        g->operands.push_back(qubit);
    }
    g->creg_operands.clear();
    for (auto &cop : cregs) {
        g->creg_operands.push_back(cop);
    }
    g->breg_operands.clear();
    for (auto &bop : bregs) {
        g->breg_operands.push_back(bop);
    }
    if (duration > 0) {
        g->duration = duration;
    }
    g->angle = angle;
    g->condition = gcond;
    g->cond_operands = gcondregs;
    gates.add(g);

    QL_DOUT("custom gate added for " << gname);
    cycles_valid = false;
    return true;
}

// FIXME: move to class composite_gate?
// return the subinstructions of a composite gate
// while doing, test whether the subinstructions have a definition (so they cannot be specialized or default ones!)
void Kernel::get_decomposed_ins(
    const gate_types::Composite &gate,
    Vec<Str> &sub_instructions
) const {
    auto &sub_gates = gate.gs;
    QL_DOUT("composite ins: " << gate.name);
    for (auto &agate : sub_gates) {
        const Str &sub_ins = agate->name;
        QL_DOUT("  sub ins: " << sub_ins);
        auto it = platform->instruction_map.find(sub_ins);
        if (it != platform->instruction_map.end()) {
            // QL_DOUT("  sub ins found in instruction_map: " << sub_ins);
            sub_instructions.push_back(sub_ins);
        } else {
            QL_ICE("gate decomposition not available for '" << sub_ins << "' in the target platform");
        }
    }
}

// if specialized composed gate: "e.g. cz q0,q3" available, with composition of subinstructions, return true
//      also check each subinstruction for presence of a custom_gate (or a default gate)
// otherwise, return false
// don't add anything to circuit
//
// add specialized decomposed gate, example JSON definition: "cl_14 q1": ["rx90 %0", "rym90 %0", "rxm90 %0"]
Bool Kernel::add_spec_decomposed_gate_if_available(
    const Str &gate_name,
    const Vec<UInt> &all_qubits,
    const Vec<UInt> &cregs,
    const Vec<UInt> &bregs,
    ConditionType gcond,
    const Vec<UInt> &gcondregs
) {
    Bool added = false;
    QL_DOUT("Checking if specialized decomposition is available for " << gate_name);

    // construct canonical name
    Str instr_parameterized = "";
    for (auto qubit : all_qubits) {
        if (!instr_parameterized.empty()) {
            instr_parameterized += ",";
        }
        instr_parameterized += std::string{"q"} + to_string(qubit);
    }
    instr_parameterized = gate_name + " " + instr_parameterized;
    QL_DOUT("specialized instruction name: " << instr_parameterized);

    // find the name
    auto it = platform->instruction_map.find(instr_parameterized);
    if (it != platform->instruction_map.end()) {
        // check gate type
        QL_DOUT("specialized composite gate found for " << instr_parameterized);
        if (it->second->type() == GateType::COMPOSITE) {
            QL_DOUT("gate type is composite gate type " << instr_parameterized);
        } else {
            QL_DOUT("not a composite gate type " << instr_parameterized);
            return false;
        }
        auto gptr = it->second.as<gate_types::Composite>();
        if (gptr.empty()) {
            QL_DOUT("but its gate pointer is empty, not a composite gate type");
            return false;
        }

        // perform decomposition
        Vec<Str> sub_instructions;
        get_decomposed_ins(*gptr, sub_instructions);
        for (auto &sub_ins : sub_instructions) {
            // extract name and qubits
            QL_DOUT("Adding sub ins: " << sub_ins << " of composite " << instr_parameterized);
            std::replace(sub_ins.begin(), sub_ins.end(), ',', ' ');    // FIXME: perform all conversions in sanitize_instruction_name()
            QL_DOUT(" after comma removal, sub ins: " << sub_ins);
            std::istringstream iss(sub_ins);

            Vec<Str> tokens{
                std::istream_iterator<Str>{iss},
                std::istream_iterator<Str>{}
            };

            Vec<UInt> this_gate_qubits;
            Str &sub_ins_name = tokens[0];

            for (UInt i = 1; i < tokens.size(); i++) {
                QL_DOUT("tokens[i] : " << tokens[i]);
                auto sub_str_token = tokens[i].substr(1);
                QL_DOUT("sub_str_token[i] : " << sub_str_token);
                this_gate_qubits.push_back(stoi(tokens[i].substr(1)));
            }

            QL_DOUT("actual qubits of this gate:" << this_gate_qubits);

            // custom gate check
            // when found, custom_added is true, and the expanded subinstruction was added to the circuit
            Bool custom_added = add_custom_gate_if_available(sub_ins_name, this_gate_qubits, cregs, 0, 0.0, bregs, gcond, gcondregs);
            if (!custom_added) {
                if (com::options::get("use_default_gates") == "yes") {
                    // default gate check
                    QL_DOUT("adding default gate for " << sub_ins_name);
                    Bool default_available = add_default_gate_if_available(sub_ins_name, this_gate_qubits, cregs, 0, 0.0, bregs, gcond, gcondregs);
                    if (default_available) {
                        QL_DOUT("added default gate '" << sub_ins_name << "' with qubits " << this_gate_qubits); // // NB: changed WOUT to DOUT, since this is common for 'barrier', spamming log
                    } else {
                        QL_USER_ERROR("unknown gate '" << sub_ins_name << "' with qubits " << this_gate_qubits);
                    }
                } else {
                    QL_USER_ERROR("unknown gate '" << sub_ins_name << "' with qubits " << this_gate_qubits);
                }
            }
        }
        added = true;
    } else {
        QL_DOUT("composite gate not found for " << instr_parameterized);
    }

    return added;
}

// if composite gate: "e.g. cz %0 %1" available, return true;
//      also check each subinstruction for availability as a custom gate (or default gate)
// if not, return false
// don't add anything to circuit
//
// add parameterized decomposed gate, example JSON definition: "cl_14 %0": ["rx90 %0", "rym90 %0", "rxm90 %0"]
Bool Kernel::add_param_decomposed_gate_if_available(
    const Str &gate_name,
    const Vec<UInt> &all_qubits,
    const Vec<UInt> &cregs,
    const Vec<UInt> &bregs,
    ConditionType gcond,
    const Vec<UInt> &gcondregs
) {
    Bool added = false;
    QL_DOUT("Checking if parameterized composite gate is available for " << gate_name);

    // construct instruction name from gate_name and actual qubit parameters
    Str instr_parameterized = "";
    for (UInt i = 0; i < all_qubits.size(); i++) {
        if (!instr_parameterized.empty()) {
            instr_parameterized += ",";
        }
        instr_parameterized += std::string{"%"} + to_string(i);
    }
    instr_parameterized = gate_name + " " + instr_parameterized;
    QL_DOUT("parameterized instruction name: " << instr_parameterized);

    // check for composite ins
    auto it = platform->instruction_map.find(instr_parameterized);
    if (it != platform->instruction_map.end()) {
        QL_DOUT("parameterized gate found for " << instr_parameterized);
        if (it->second->type() == GateType::COMPOSITE) {
            QL_DOUT("gate type is COMPOSITE for " << instr_parameterized);
        } else {
            QL_DOUT("not a composite gate type " << instr_parameterized);
            return false;
        }
        auto gptr = it->second.as<gate_types::Composite>();
        if (gptr.empty()) {
            QL_DOUT("is composite but gate pointer is empty, so not a composite gate type");
            return false;
        }

        Vec<Str> sub_instructions;
        get_decomposed_ins(*gptr, sub_instructions);
        for (auto &sub_ins : sub_instructions) {
            QL_DOUT("Adding sub ins: " << sub_ins);
            std::replace( sub_ins.begin(), sub_ins.end(), ',', ' ');
            QL_DOUT(" after comma removal, sub ins: " << sub_ins);

            // tokenize sub_ins into sub_ins_name and this_gate_qubits
            // FIXME: similar code in add_spec_decomposed_gate_if_available()
            std::istringstream iss(sub_ins);
            Vec<Str> tokens{ std::istream_iterator<Str>{iss},
                                             std::istream_iterator<Str>{} };

            Vec<UInt> this_gate_qubits;
            Str & sub_ins_name = tokens[0];

            for (UInt i = 1; i < tokens.size(); i++) {
                auto sub_str_token = tokens[i].substr(1);   // example: tokens[i] equals "%1" -> sub_str_token equals "1"
                UInt qubit_idx = stoi(sub_str_token);
                if (qubit_idx >= all_qubits.size()) {
                    QL_FATAL("Illegal qubit parameter index " << sub_str_token
                                                              << " exceeds actual number of parameters given (" << all_qubits.size()
                                                              << ") while adding sub ins '" << sub_ins
                                                              << "' in parameterized instruction '" << instr_parameterized << "'");
                }
                this_gate_qubits.push_back(all_qubits[qubit_idx]);
            }
            QL_DOUT("actual qubits of this gate: " << this_gate_qubits);

            // FIXME: following code block exists several times in this file
            // custom gate check
            // when found, custom_added is true, and the expanded subinstruction was added to the circuit
            Bool custom_added = add_custom_gate_if_available(sub_ins_name, this_gate_qubits, cregs, 0, 0.0, bregs, gcond, gcondregs);
            if (!custom_added) {
                if (com::options::get("use_default_gates") == "yes") {
                    // default gate check
                    QL_DOUT("adding default gate for " << sub_ins_name);
                    Bool default_available = add_default_gate_if_available(sub_ins_name, this_gate_qubits, cregs, 0, 0.0, bregs, gcond, gcondregs);
                    if (default_available) {
                        QL_WOUT("added default gate '" << sub_ins_name << "' with qubits " << this_gate_qubits);
                    } else {
                        QL_USER_ERROR("unknown gate '" << sub_ins_name << "' with qubits " << this_gate_qubits);
                    }
                } else {
                    QL_USER_ERROR("unknown gate '" << sub_ins_name << "' with qubits " << this_gate_qubits);
                }
            }
        }
        added = true;
        QL_DOUT("added composite gate and subinstrs for " << instr_parameterized);
    } else {
#ifdef MULTI_LINE_LOG_DEBUG
        QL_IF_LOG_DEBUG {
            QL_DOUT("composite gate not found for " << instr_parameterized << " in instruction_map:");
            for (const auto &i : platform->instruction_map) {
                QL_DOUT("add_param_decomposed_gate_if_available: platform->instruction_map[]" << i.first);
            }
        }
#else
        QL_DOUT("composite gate not found for " << instr_parameterized << " in instruction_map (disabled)");
#endif
    }
    return added;
}

void Kernel::gate(const Str &gname, UInt q0) {
    gate(gname, Vec<UInt> {q0});
}

void Kernel::gate(const Str &gname, UInt q0, UInt q1) {
    gate(gname, Vec<UInt> {q0, q1});
}

/**
 * general user-level gate creation with any combination of operands
 *
 * check argument register indices against platform parameters; fail fatally if an index is out of range
 * add implicit arguments if absent (used when no register argument means all registers)
 * find matching gate in kernel's and platform's gate_definition (custom or default); when no match, fail
 * return the gate (or its decomposition) by appending it to kernel.c, the current kernel's circuit
 */
void Kernel::gate(
    const Str &gname,
    const Vec<UInt> &qubits,
    const Vec<UInt> &cregs,
    UInt duration,
    Real angle,
    const Vec<UInt> &bregs,
    ConditionType gcond,
    const Vec<UInt> &gcondregs
) {
    QL_DOUT("gate:" <<" gname=" << gname <<" qubits=" << qubits <<" cregs=" << cregs <<" duration=" << duration <<" angle=" << angle <<" bregs=" << bregs <<" gcond=" << gcond <<" gcondregs=" << gcondregs);

    for (auto &qno : qubits) {
        if (qno >= qubit_count) {
            QL_FATAL("Number of qubits in platform: " << to_string(qubit_count) << ", specified qubit numbers out of range for gate: '" << gname << "' with qubits " << qubits);
        }
    }
    for (auto &cno : cregs) {
        if (cno >= creg_count) {
            QL_FATAL("Out of range operand(s) for '" << gname << "' with cregs " << cregs);
        }
    }
    for (auto &bno : bregs) {
        if (bno >= breg_count) {
            QL_FATAL("Out of range operand(s) for '" << gname << "' with bregs " << bregs);
        }
    }
    if (!Gate::is_valid_cond(gcond, gcondregs)) {
        QL_FATAL("Condition " << gcond << " of '" << gname << "' incompatible with gcondregs " << gcondregs);
    }
    for (auto &cbno : gcondregs) {
        if (cbno >= breg_count) {
            QL_FATAL("Out of range condition operand(s) for '" << gname << "' with gcondregs " << gcondregs);
        }
    }
    auto lqubits = qubits;
    auto lcregs = cregs;
    auto lbregs = bregs;
    gate_add_implicits(gname, lqubits, lcregs, duration, angle, lbregs, gcond, gcondregs);
    if (!gate_nonfatal(gname, lqubits, lcregs, duration, angle, lbregs, gcond, gcondregs)) {
        QL_FATAL("Unknown gate '" << gname << "' with qubits " << lqubits);
    }
}

/**
 * preset condition to make all future created gates conditional gates with this condition
 * preset ends when cleared: back to {cond_always, {}};
 * useful in combination with higher-level gate creation interfaces
 * that don't support adding a condition for conditional execution
 */
void Kernel::gate_preset_condition(
    ConditionType gcond,
    const utils::Vec<utils::UInt> &gcondregs
) {
    if (!Gate::is_valid_cond(gcond, gcondregs)) {
        QL_FATAL("Condition " << gcond << " of gate_preset_condition incompatible with gcondregs " << gcondregs);
    }
    QL_DOUT("Gate_preset_condition: setting condition=" << condition << " cond_operands=" << cond_operands);
    condition = gcond;
    cond_operands = gcondregs;
}

/**
 * clear preset condition again
 */
void Kernel::gate_clear_condition() {
    gate_preset_condition(ConditionType::ALWAYS, {});
}

/**
 * short-cut creation of conditional gate with only qubits as operands
 */
void Kernel::condgate(
    const utils::Str &gname,
    const utils::Vec<utils::UInt> &qubits,
    ConditionType gcond,
    const utils::Vec<utils::UInt> &gcondregs
) {
    gate(gname, qubits, {}, 0, 0.0, {}, gcond, gcondregs);
}

/**
 * conversion used by Python conditional execution interface
 */
ConditionType Kernel::condstr2condvalue(const std::string &condstring) {
    ConditionType condvalue;
    if      (condstring == "COND_ALWAYS" || condstring == "1") condvalue = ConditionType::ALWAYS;
    else if (condstring == "COND_NEVER" || condstring == "0") condvalue = ConditionType::NEVER;
    else if (condstring == "COND_UNARY" || condstring.empty()) condvalue = ConditionType::UNARY;
    else if (condstring == "COND_NOT" || condstring == "!") condvalue = ConditionType::NOT;
    else if (condstring == "COND_AND" || condstring == "&") condvalue = ConditionType::AND;
    else if (condstring == "COND_NAND" || condstring == "!&") condvalue = ConditionType::NAND;
    else if (condstring == "COND_OR" || condstring == "|") condvalue = ConditionType::OR;
    else if (condstring == "COND_NOR" || condstring == "!|") condvalue = ConditionType::NOR;
    else if (condstring == "COND_XOR" || condstring == "^") condvalue = ConditionType::XOR;
    else if (condstring == "COND_NXOR" || condstring == "!^") condvalue = ConditionType::NXOR;
    else {
        throw std::runtime_error("Error: Unknown condition " + condstring);
    }
    return condvalue;
}

/**
 * add implicit parameters to gate to match IR requirements
 */
void Kernel::gate_add_implicits(
    const Str &gname,
    Vec<UInt> &qubits,
    Vec<UInt> &cregs,
    UInt &duration,
    Real &angle,
    Vec<UInt> &bregs,
    ConditionType &gcond,
    const Vec<UInt> &gcondregs
) {
    if (gname == "measure" || gname == "measx" || gname == "measz") {
        QL_DOUT("gate_add_implicits:" <<" gname=" << gname <<" qubits=" << qubits <<" cregs=" << cregs <<" duration=" << duration <<" angle=" << angle <<" bregs=" << bregs <<" gcond=" << gcond <<" gcondregs=" << gcondregs);
        if (bregs.size() == 0 && qubits[0] < breg_count) {
            bregs.push_back(qubits[0]);
        }
        QL_DOUT("gate_add_implicits (after):" <<" gname=" << gname <<" qubits=" << qubits <<" cregs=" << cregs <<" duration=" << duration <<" angle=" << angle <<" bregs=" << bregs <<" gcond=" << gcond <<" gcondregs=" << gcondregs);
    }
}

/**
 * custom gate with arbitrary number of operands
 * as gate above but return whether gate was successfully matched in gate_definition, next to gate in kernel.c
 */
Bool Kernel::gate_nonfatal(
    const Str &gname,
    const Vec<UInt> &qubits,
    const Vec<UInt> &cregs,
    UInt duration,
    Real angle,
    const Vec<UInt> &bregs,
    ConditionType gcond,
    const Vec<UInt> &gcondregs
) {
    Vec<UInt> lcondregs = gcondregs;

    // check and impose kernel's preset condition if any
    if (condition != ConditionType::ALWAYS && ( condition != gcond || cond_operands != gcondregs)) {
        // a non-trivial condition, different from the current condition argument (gcond/gcondregs),
        // was preset in the kernel to be imposed on all subsequently created gates
        // if the condition argument is also non-trivial, there is a clash (but we could also take the intersection)
        if (gcond != ConditionType::ALWAYS) {
            QL_FATAL("Condition " << gcond << " for '" << gname << "' specified while a different non-trivial condition was already preset");
        }
        // impose kernel's preset condition
        gcond = condition;
        lcondregs = cond_operands;
    }

    Bool added = false;
    // check if specialized composite gate is available
    // if not, check if parameterized composite gate is available
    // if not, check if a specialized custom gate is available
    // if not, check if a parameterized custom gate is available
    // if not, check if a default gate is available
    // if not, then error

    QL_DOUT("Gate_nonfatal:" <<" gname=" << gname <<" qubits=" << qubits <<" cregs=" << cregs <<" duration=" << duration <<" angle=" << angle <<" bregs=" << bregs <<" gcond=" << gcond <<" gcondregs=" << gcondregs);

    auto gname_lower = to_lower(gname);
    QL_DOUT("Adding gate : " << gname_lower << " with qubits " << qubits);

    // specialized composite gate check
    QL_DOUT("trying to add specialized composite gate for: " << gname_lower);
    Bool spec_decom_added = add_spec_decomposed_gate_if_available(gname_lower, qubits, cregs, bregs, gcond, lcondregs);
    if (spec_decom_added) {
        added = true;
        QL_DOUT("specialized decomposed gates added for " << gname_lower);
    } else {
        // parameterized composite gate check
        QL_DOUT("trying to add parameterized composite gate for: " << gname_lower);
        Bool param_decom_added = add_param_decomposed_gate_if_available(gname_lower, qubits, cregs, bregs, gcond, lcondregs);
        if (param_decom_added) {
            added = true;
            QL_DOUT("decomposed gates added for " << gname_lower);
        } else {
            // specialized/parameterized custom gate check
            QL_DOUT("adding custom gate for " << gname_lower);
            // when found, custom_added is true, and the gate was added to the circuit
            Bool custom_added = add_custom_gate_if_available(gname_lower, qubits, cregs, duration, angle, bregs, gcond, lcondregs);
            if (custom_added) {
                added = true;
                QL_DOUT("custom gate added for " << gname_lower);
            } else {
                if (com::options::get("use_default_gates") == "yes") {
                    // default gate check (which is always parameterized)
                    QL_DOUT("adding default gate for " << gname_lower);

                    Bool default_available = add_default_gate_if_available(gname_lower, qubits, cregs, duration, angle, bregs, gcond, lcondregs);
                    if (default_available) {
                        added = true;
                        QL_DOUT("default gate added for " << gname_lower);   // FIXME: used to be WOUT, but that gives a warning for every "wait" and spams the log
                    }
                }
            }
        }
    }
    if (added) {
        cycles_valid = false;
    }
    return added;
}

// to add unitary to kernel
void Kernel::gate(
    com::dec::Unitary &u,
    const Vec<UInt> &qubits
) {
    QL_DOUT("Adding decomposed unitary to kernel ...");
    cycles_valid = false;
    gates.extend(u.get_decomposition(qubits));
}

// adding state prepration / (arbitrary) qubit initialisation to kernel
void Kernel::state_prep(
    const Vec<Complex> &array,
    const Vec<UInt> &qubits
) {
    QL_DOUT("Preparing state with array " << array);
    cycles_valid = false;
    com::dec::Unitary u("state prep", {array.begin(), array.end()});
    gates.extend(u.prepare_state(qubits));
}

/**
 * qasm output
 */
// FIXME: create a separate QASM backend?
Str Kernel::get_prologue() const  {
    StrStrm ss;
    ss << "\n";
    ss << "." << name << "\n";
    // ss << name << ":\n";

    if (type == KernelType::IF_START) {
        ss << "    b" << br_condition->inv_operation_name <<" r" << (br_condition->operands[0])->as_register().id
           <<", r" << (br_condition->operands[1])->as_register().id << ", " << name << "_end\n";
    }

    if (type == KernelType::ELSE_START) {
        ss << "    b" << br_condition->operation_name <<" r" << (br_condition->operands[0])->as_register().id
           <<", r" << (br_condition->operands[1])->as_register().id << ", " << name << "_end\n";
    }

    if (type == KernelType::FOR_START) {
        // TODO for now r29, r30, r31 are used, fix it
        ss << "    ldi r29" <<", " << iteration_count << "\n";
        ss << "    ldi r30" <<", " << 1 << "\n";
        ss << "    ldi r31" <<", " << 0 << "\n";
    }

    return ss.str();
}

// FIXME: generates duplicate labels for names that match up to the first "_", see Backend::loopLabel()
Str Kernel::get_epilogue() const {
    StrStrm ss;

    if (type == KernelType::DO_WHILE_END) {
        ss << "    b" << br_condition->operation_name <<" r" << (br_condition->operands[0])->as_register().id
           <<", r" << (br_condition->operands[1])->as_register().id << ", " << name << "_start\n";
    }

    if (type == KernelType::FOR_END) {
        Str kname(name);
        std::replace( kname.begin(), kname.end(), '_', ' ');
        std::istringstream iss(kname);
        Vec<Str> tokens{ std::istream_iterator<Str>{iss},
                                         std::istream_iterator<Str>{} };

        // TODO for now r29, r30, r31 are used, fix it
        ss << "    add r31, r31, r30\n";
        ss << "    blt r31, r29, " << tokens[0] << "\n";
    }

    return ss.str();
}

Str Kernel::qasm() const {
    StrStrm ss;

    ss << get_prologue();

    for (UInt i = 0; i < gates.size(); ++i) {
        ss << "    " << gates[i]->qasm() << "\n";
    }

    ss << get_epilogue();

    return  ss.str();
}

/**
 * classical gate
 */
void Kernel::classical(const ClassicalRegister &destination, const ClassicalOperation &oper) {
    // check sanity of destination
    if (destination.id >= creg_count) {
        QL_USER_ERROR("operand(s) out of range for '" << oper.operation_name << "'");
    }

    // check sanity of other operands
    for (auto &op : oper.operands) {
        if (op->type() == ClassicalOperandType::REGISTER) {
            if (op->as_register().id >= creg_count) {
                QL_USER_ERROR("operand(s) out of range for '" << oper.operation_name << "'");
            }
        }
    }

    gates.emplace<gate_types::Classical>(destination, oper);
    cycles_valid = false;
}

void Kernel::classical(const Str &operation) {
    gates.emplace<gate_types::Classical>(operation);
    cycles_valid = false;
}

void Kernel::controlled_x(UInt tq, UInt cq) {
    // from: https://arxiv.org/pdf/1206.0758v3.pdf
    // A meet-in-the-middle algorithm for fast synthesis
    // of depth-optimal quantum circuits
    cnot(cq, tq);
}

void Kernel::controlled_y(UInt tq, UInt cq) {
    // from: https://arxiv.org/pdf/1206.0758v3.pdf
    // A meet-in-the-middle algorithm for fast synthesis
    // of depth-optimal quantum circuits
    sdag(tq);
    cnot(cq, tq);
    s(tq);
}

void Kernel::controlled_z(UInt tq, UInt cq) {
    // from: https://arxiv.org/pdf/1206.0758v3.pdf
    // A meet-in-the-middle algorithm for fast synthesis
    // of depth-optimal quantum circuits
    hadamard(tq);
    cnot(cq, tq);
    hadamard(tq);
}

void Kernel::controlled_h(UInt tq, UInt cq) {
    // from: https://arxiv.org/pdf/1206.0758v3.pdf
    // A meet-in-the-middle algorithm for fast synthesis
    // of depth-optimal quantum circuits
    s(tq);
    hadamard(tq);
    t(tq);
    cnot(cq, tq);
    tdag(tq);
    hadamard(tq);
    sdag(tq);
}

void Kernel::controlled_i(UInt, UInt) {
    // well, basically you dont need to do anything for it :‑)
}

void Kernel::controlled_s(UInt tq, UInt cq) {
    // cphase(cq, tq);

    // from: https://arxiv.org/pdf/1206.0758v3.pdf
    // A meet-in-the-middle algorithm for fast synthesis
    // of depth-optimal quantum circuits

    cnot(tq, cq);
    tdag(cq);
    cnot(tq, cq);
    t(cq);
    t(tq);
}

void Kernel::controlled_sdag(UInt tq, UInt cq) {
    // based on: https://arxiv.org/pdf/1206.0758v3.pdf
    // A meet-in-the-middle algorithm for fast synthesis
    // of depth-optimal quantum circuits

    tdag(cq);
    tdag(tq);
    cnot(tq, cq);
    t(cq);
    cnot(tq, cq);
}

void Kernel::controlled_t(UInt tq, UInt cq, UInt aq) {
    QL_WOUT("Controlled-T implementation requires an ancilla");
    QL_WOUT("At the moment, Qubit 0 is used as ancilla");
    QL_WOUT("This will change when Qubit allocater is implemented");
    // from: https://arxiv.org/pdf/1206.0758v3.pdf
    // A meet-in-the-middle algorithm for fast synthesis
    // of depth-optimal quantum circuits
    cnot(cq, tq);
    hadamard(aq);
    sdag(cq);
    cnot(tq, aq);
    cnot(aq, cq);
    t(cq);
    tdag(aq);
    cnot(tq, cq);
    cnot(tq, aq);
    t(cq);
    tdag(aq);
    cnot(aq, cq);
    h(cq);
    t(cq);
    h(cq);
    cnot(aq, cq);
    tdag(cq);
    t(aq);
    cnot(tq, aq);
    cnot(tq, cq);
    t(aq);
    tdag(cq);
    cnot(aq, cq);
    s(cq);
    cnot(tq, aq);
    cnot(cq, tq);
    h(aq);
}

void Kernel::controlled_tdag(UInt tq, UInt cq, UInt aq) {
    QL_WOUT("Controlled-Tdag implementation requires an ancilla");
    QL_WOUT("At the moment, Qubit 0 is used as ancilla");
    QL_WOUT("This will change when Qubit allocater is implemented");
    // from: https://arxiv.org/pdf/1206.0758v3.pdf
    // A meet-in-the-middle algorithm for fast synthesis
    // of depth-optimal quantum circuits
    h(aq);
    cnot(cq, tq);
    sdag(cq);
    cnot(tq, aq);
    cnot(aq, cq);
    t(cq);
    cnot(tq, cq);
    tdag(aq);
    cnot(tq, aq);
    t(cq);
    tdag(aq);
    cnot(aq, cq);
    h(cq);
    tdag(cq);
    h(cq);
    cnot(aq, cq);
    tdag(cq);
    t(aq);
    cnot(tq, aq);
    cnot(tq, cq);
    tdag(cq);
    t(aq);
    cnot(aq, cq);
    s(cq);
    cnot(tq, aq);
    cnot(cq, tq);
    hadamard(aq);
}

void Kernel::controlled_ix(UInt tq, UInt cq) {
    // from: https://arxiv.org/pdf/1210.0974.pdf
    // Quantum circuits of T-depth one
    cnot(cq, tq);
    s(cq);
}

// toffoli decomposition
// from: https://arxiv.org/pdf/1210.0974.pdf
// Quantum circuits of T-depth one
void Kernel::controlled_cnot_AM(UInt tq, UInt cq1, UInt cq2) {
    h(tq);
    t(cq1);
    t(cq2);
    t(tq);
    cnot(cq2, cq1);
    cnot(tq, cq2);
    cnot(cq1, tq);
    tdag(cq2);
    cnot(cq1, cq2);
    tdag(cq1);
    tdag(cq2);
    tdag(tq);
    cnot(tq, cq2);
    cnot(cq1, tq);
    cnot(cq2, cq1);
    h(tq);
}

// toffoli decomposition
// Neilsen and Chuang
void Kernel::controlled_cnot_NC(UInt tq, UInt cq1, UInt cq2) {
    h(tq);
    cnot(cq2,tq);
    tdag(tq);
    cnot(cq1,tq);
    t(tq);
    cnot(cq2,tq);
    tdag(tq);
    cnot(cq1,tq);
    tdag(cq2);
    t(tq);
    cnot(cq1,cq2);
    h(tq);
    tdag(cq2);
    cnot(cq1,cq2);
    t(cq1);
    s(cq2);
}

void Kernel::controlled_swap(UInt tq1, UInt tq2, UInt cq) {
    // from: https://arxiv.org/pdf/1210.0974.pdf
    // Quantum circuits of T-depth one
    cnot(tq2, tq1);
    cnot(cq, tq1);
    h(tq2);
    t(cq);
    tdag(tq1);
    t(tq2);
    cnot(tq2, tq1);
    cnot(cq, tq2);
    t(tq1);
    cnot(cq, tq1);
    tdag(tq2);
    tdag(tq1);
    cnot(cq, tq2);
    cnot(tq2, tq1);
    t(tq1);
    h(tq2);
    cnot(tq2, tq1);
}

void Kernel::controlled_rx(UInt tq, UInt cq, Real theta) {
    rx(tq, theta/2);
    cz(cq, tq);
    rx(tq, -theta/2);
    cz(cq, tq);
}

void Kernel::controlled_ry(UInt tq, UInt cq, Real theta) {
    ry(tq, theta/2);
    cnot(cq, tq);
    ry(tq, -theta/2);
    cnot(cq, tq);
}

void Kernel::controlled_rz(UInt tq, UInt cq, Real theta) {
    rz(tq, theta/2);
    cnot(cq, tq);
    rz(tq, -theta/2);
    cnot(cq, tq);
}

void Kernel::controlled_single(
    const Kernel &k,
    UInt control_qubit,
    UInt ancilla_qubit
) {
    for (auto &g : k.gates) {
        Str gname = g->name;
        GateType gtype = g->type();
        Vec<UInt> goperands = g->operands;
        QL_DOUT("Generating controlled gate for " << gname);
        QL_DOUT("Type : " << gtype);
        if (gtype == GateType::PAULI_X || gtype == GateType::RX180) {
            UInt tq = goperands[0];
            UInt cq = control_qubit;
            controlled_x(tq, cq);
        } else if (gtype == GateType::PAULI_Y || gtype == GateType::RY180) {
            UInt tq = goperands[0];
            UInt cq = control_qubit;
            controlled_y(tq, cq);
        } else if (gtype == GateType::PAULI_Z) {
            UInt tq = goperands[0];
            UInt cq = control_qubit;
            controlled_z(tq, cq);
        } else if (gtype == GateType::HADAMARD) {
            UInt tq = goperands[0];
            UInt cq = control_qubit;
            controlled_h(tq, cq);
        } else if (gtype == GateType::IDENTITY) {
            UInt tq = goperands[0];
            UInt cq = control_qubit;
            controlled_i(tq, cq);
        } else if (gtype == GateType::T) {
            UInt tq = goperands[0];
            UInt cq = control_qubit;
            UInt aq = ancilla_qubit;
            controlled_t(tq, cq, aq);
        } else if (gtype == GateType::T_DAG) {
            UInt tq = goperands[0];
            UInt cq = control_qubit;
            UInt aq = ancilla_qubit;
            controlled_tdag(tq, cq, aq);
        } else if (gtype == GateType::PHASE) {
            UInt tq = goperands[0];
            UInt cq = control_qubit;
            controlled_s(tq, cq);
        } else if (gtype == GateType::PHASE_DAG) {
            UInt tq = goperands[0];
            UInt cq = control_qubit;
            controlled_sdag(tq, cq);
        } else if (gtype == GateType::CNOT) {
            UInt cq1 = goperands[0];
            UInt cq2 = control_qubit;
            UInt tq = goperands[1];

            auto opt = com::options::get("decompose_toffoli");
            if (opt == "AM") {
                controlled_cnot_AM(tq, cq1, cq2);
            } else if (opt == "NC") {
                controlled_cnot_NC(tq, cq1, cq2);
            } else {
                toffoli(cq1, cq2, tq);
            }
        } else if (gtype == GateType::SWAP) {
            UInt tq1 = goperands[0];
            UInt tq2 = goperands[1];
            UInt cq = control_qubit;
            controlled_swap(tq1, tq2, cq);
        } else if (gtype == GateType::RX) {
            UInt tq = goperands[0];
            UInt cq = control_qubit;
            controlled_rx(tq, cq, g->angle);
        } else if (gtype == GateType::RY) {
            UInt tq = goperands[0];
            UInt cq = control_qubit;
            controlled_ry(tq, cq, g->angle);
        } else if (gtype == GateType::RZ) {
            UInt tq = goperands[0];
            UInt cq = control_qubit;
            controlled_rz(tq, cq, g->angle);
        } else if (gtype == GateType::RX90) {
            UInt tq = goperands[0];
            UInt cq = control_qubit;
            controlled_rx(tq, cq, M_PI/2);
        } else if (gtype == GateType::MRX90) {
            UInt tq = goperands[0];
            UInt cq = control_qubit;
            controlled_rx(tq, cq, -1*M_PI/2);
        } else if (gtype == GateType::RX180) {
            UInt tq = goperands[0];
            UInt cq = control_qubit;
            controlled_rx(tq, cq, M_PI);
            // controlled_x(tq, cq);
        } else if (gtype == GateType::RY90) {
            UInt tq = goperands[0];
            UInt cq = control_qubit;
            controlled_ry(tq, cq, M_PI/4);
        } else if (gtype == GateType::MRY90) {
            UInt tq = goperands[0];
            UInt cq = control_qubit;
            controlled_ry(tq, cq, -1*M_PI/4);
        } else if (gtype == GateType::RY180) {
            UInt tq = goperands[0];
            UInt cq = control_qubit;
            controlled_ry(tq, cq, M_PI);
            // controlled_y(tq, cq);
        } else {
            QL_USER_ERROR("circuit too complex; controlled version of gate '" << gname << "' is unknown");
        }
    }
}

void Kernel::controlled(
    const Kernel &k,
    const Vec<UInt> &control_qubits,
    const Vec<UInt> &ancilla_qubits
) {
    QL_DOUT("Generating controlled kernel ... ");
    Int ncq = control_qubits.size();
    Int naq = ancilla_qubits.size();

    if (ncq == 0) {
        QL_USER_ERROR("at least one control_qubits must be specified");
    } else if (ncq == 1) {
        //                      control               ancilla
        controlled_single(k, control_qubits[0], ancilla_qubits[0]);
    } else if (ncq > 1) {
        // Network implementing C^n(U) operation
        // - based on Fig. 4.10, p.p 185, Nielson & Chuang
        // - requires as many ancilla/work qubits as control qubits
        if (naq == ncq) {
            toffoli(control_qubits[0], control_qubits[1], ancilla_qubits[0]);

            for (Int n = 0; n <= naq - 3; n++) {
                toffoli(control_qubits[n+2], ancilla_qubits[n], ancilla_qubits[n+1]);
            }

            //                      control               ancilla
            controlled_single(k, ancilla_qubits[naq-2], ancilla_qubits[naq-1]);

            for (Int n = naq - 3; n >= 0; n--) {
                toffoli(control_qubits[n+2], ancilla_qubits[n], ancilla_qubits[n+1]);
            }

            toffoli(control_qubits[0], control_qubits[1], ancilla_qubits[0]);
        } else {
            QL_USER_ERROR("number of control qubits must equal number of ancilla qubits");
        }
    }

    QL_DOUT("Generating controlled kernel [Done]");
}

void Kernel::conjugate(const Kernel &k) {
    QL_COUT("Generating conjugate kernel");
    for (auto rgit = k.gates.get_vec().rbegin(); rgit != k.gates.get_vec().rend(); ++rgit) {
        auto g = *rgit;
        Str gname = g->name;
        GateType gtype = g->type();
        QL_DOUT("Generating conjugate gate for " << gname);
        QL_DOUT("Type : " << gtype);
        if (gtype == GateType::PAULI_X || gtype == GateType::RX180) {
            gate("x", g->operands, {}, g->duration, g->angle, g->breg_operands);
        } else if (gtype == GateType::PAULI_Y || gtype == GateType::RY180) {
            gate("y", g->operands, {}, g->duration, g->angle, g->breg_operands);
        } else if (gtype == GateType::PAULI_Z) {
            gate("z", g->operands, {}, g->duration, g->angle, g->breg_operands);
        } else if (gtype == GateType::HADAMARD) {
            gate("hadamard", g->operands, {}, g->duration, g->angle, g->breg_operands);
        } else if (gtype == GateType::IDENTITY) {
            gate("identity", g->operands, {}, g->duration, g->angle, g->breg_operands);
        } else if (gtype == GateType::T) {
            gate("tdag", g->operands, {}, g->duration, g->angle, g->breg_operands);
        } else if (gtype == GateType::T_DAG) {
            gate("t", g->operands, {}, g->duration, g->angle, g->breg_operands);
        } else if (gtype == GateType::PHASE) {
            gate("sdag", g->operands, {}, g->duration, g->angle, g->breg_operands);
        } else if (gtype == GateType::PHASE_DAG) {
            gate("s", g->operands, {}, g->duration, g->angle, g->breg_operands);
        } else if (gtype == GateType::CNOT) {
            gate("cnot", g->operands, {}, g->duration, g->angle, g->breg_operands);
        } else if (gtype == GateType::SWAP) {
            gate("swap", g->operands, {}, g->duration, g->angle, g->breg_operands);
        } else if (gtype == GateType::RX) {
            gate("rx", g->operands, {}, g->duration, -(g->angle), g->breg_operands);
        } else if (gtype == GateType::RY) {
            gate("ry", g->operands, {}, g->duration, -(g->angle), g->breg_operands);
        } else if (gtype == GateType::RZ) {
            gate("rz", g->operands, {}, g->duration, -(g->angle), g->breg_operands);
        } else if (gtype == GateType::RX90) {
            gate("mrx90", g->operands, {}, g->duration, g->angle, g->breg_operands);
        } else if (gtype == GateType::MRX90) {
            gate("rx90", g->operands, {}, g->duration, g->angle, g->breg_operands);
        } else if (gtype == GateType::RX180) {
            gate("x", g->operands, {}, g->duration, g->angle, g->breg_operands);
        } else if (gtype == GateType::RY90) {
            gate("mry90", g->operands, {}, g->duration, g->angle, g->breg_operands);
        } else if (gtype == GateType::MRY90) {
            gate("ry90", g->operands, {}, g->duration, g->angle, g->breg_operands);
        } else if (gtype == GateType::RY180) {
            gate("y", g->operands, {}, g->duration, g->angle, g->breg_operands);
        } else if (gtype == GateType::CPHASE) {
            gate("cphase", g->operands, {}, g->duration, g->angle, g->breg_operands);
        } else if (gtype == GateType::TOFFOLI) {
            gate("toffoli", g->operands, {}, g->duration, g->angle, g->breg_operands);
        } else {
            QL_USER_ERROR("circuit too complex; conjugate version of gate '" << gname << "' is not defined");
        }
    }
    QL_COUT("Generating conjugate kernel [Done]");
}

} // namespace compat
} // namespace ir
} // namespace ql
