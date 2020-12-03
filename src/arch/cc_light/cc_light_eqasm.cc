/** \name
 * CC-light eQASM code emitter.
 */

#include "arch/cc_light/cc_light_eqasm.h"

namespace ql {
namespace arch {

using namespace utils;

single_qubit_mask::single_qubit_mask(qubit_set_t &&qs) : qs(std::move(qs)) {
}

single_qubit_mask::single_qubit_mask(UInt qubit) {
    qs.push_back(qubit);
}

mask_t single_qubit_mask::get_mask(UInt reg) {
    UInt n = qs.size();
    StrStrm m;
    // to do : make sure there is no duplicate qubits
    m << "smis s" << reg << ", { ";
    if (n == 1) {
        m << qs[0] << " }";
    } else {
        UInt i = 0;
        while (i++ < n) {
            m << qs[i] << ",";
        }
        m << qs[i] << "}";
    }
    return m.str();
}

two_qubit_mask::two_qubit_mask(qubit_pair_set_t &&qs) : qs(std::move(qs)) {
}

two_qubit_mask::two_qubit_mask(qubit_pair_t p) {
    qs.push_back(p);
}

mask_t two_qubit_mask::get_mask(UInt reg) {
    UInt n = qs.size();
    StrStrm m;
    // to do : make sure reg is not reserved
    // to do : make sure there is no duplicate qubits
    m << "smit t" << reg << ", { ";
    if (n == 1) {
        m << "(" << qs[0].first << "," << qs[0].second <<  ") }";
    } else {
        UInt i=0;
        while (i++ < n) {
            m << "(" << qs[i].first << "," << qs[i].second << "),";
        }
        m << "(" << qs[i].first << "," << qs[i].second << ") }";
    }
    return m.str();
}

/**
 *  compensate for latency
 */
void cc_light_eqasm_instruction::compensate_latency() {
    if (!latency_compensated) {
        // println("compensate latency : " << start << " -> " << (start-latency) << " : latency = " << latency);
        start -= latency;
        latency_compensated = true;
    } else {
        QL_WOUT("latency of instruction '" << this << "' is already compensated !");
    }
}

/**
 * set start
 */
void cc_light_eqasm_instruction::set_start(UInt t) {
    start = t;
}

/**
 * decompose meta-instructions
 */
cc_light_eqasm_program_t cc_light_eqasm_instruction::decompose() {
    cc_light_eqasm_program_t p;
    p.push_back(this);
    return p;
}

/**
 * return cc_light_eqasm instruction type
 */
cc_light_eqasm_instr_type_t cc_light_eqasm_instruction::get_instruction_type() const {
    return instr_type;
}

/**
 * return operation type
 */
operation_type_t cc_light_eqasm_instruction::get_operation_type() const {
    return operation_type;
}

cc_light_single_qubit_gate::cc_light_single_qubit_gate(
    const Str &name,
    single_qubit_mask &&mask
) : mask(std::move(mask)) {
    this->name = name;
}

/**
 * emit cc_light_eqasm code
 */
cc_light_eqasm_instr_t cc_light_single_qubit_gate::code() {
    StrStrm c;
    c << mask.get_mask(7) << "\n";
    c << "bs 1 " << name << " s7";
    return c.str();
}

cc_light_two_qubit_gate::cc_light_two_qubit_gate(
    const Str &name,
    two_qubit_mask &&mask
) : mask(std::move(mask)) {
    this->name = name;
}

/**
 * emit cc_light_eqasm code
 */
cc_light_eqasm_instr_t cc_light_two_qubit_gate::code() {
    StrStrm c;
    c << mask.get_mask(7) << "\n";
    c << "bs 1 " << name << " t7";
    return c.str();
}

} // namespace arch
} // namespace ql
