/** \file
 * Quantum kernel abstraction implementation.
 */

#pragma once

#include "utils/num.h"
#include "utils/str.h"
#include "utils/vec.h"
#include "utils/opt.h"
#include "gate.h"
#include "circuit.h"
#include "classical.h"
#include "hardware_configuration.h"
#include "unitary.h"
#include "platform.h"

namespace ql {

enum class kernel_type_t {
    STATIC,
    FOR_START, FOR_END,
    DO_WHILE_START, DO_WHILE_END,
    IF_START, IF_END,
    ELSE_START, ELSE_END
};

class quantum_kernel {
public: // FIXME: should be private
    utils::Str              name;
    utils::UInt             iterations;
    utils::UInt             qubit_count;
    utils::UInt             creg_count;
    utils::UInt             breg_count;
    kernel_type_t           type;
    circuit                 c;
    utils::Bool             cycles_valid; // used in bundler to check if kernel has been scheduled
    utils::Opt<operation>   br_condition;
    utils::UInt             cycle_time;   // FIXME HvS just a copy of platform.cycle_time
    instruction_map_t       instruction_map;
    utils::Vec<utils::UInt> cond_operands;    // see gate interface: condition mode to make new gates conditional
    cond_type_t             condition;        // kernel condition mode is set by gate_preset_condition()

public:
    quantum_kernel(const utils::Str &name);
    quantum_kernel(
        const utils::Str &name,
        const quantum_platform &platform,
        utils::UInt qcount,
        utils::UInt ccount=0,
        utils::UInt bcount=0
    );

    // FIXME: add constructor which allows setting iterations and type, and use that in program.h::add_for(), etc

    void set_condition(const operation &oper);
    void set_kernel_type(kernel_type_t typ);

    utils::Str get_gates_definition() const;
    utils::Str get_name() const;
    circuit &get_circuit();
    const circuit &get_circuit() const;

    void identity(utils::UInt qubit);
    void i(utils::UInt qubit);
    void hadamard(utils::UInt qubit);
    void h(utils::UInt qubit);
    void rx(utils::UInt qubit, utils::Real angle);
    void ry(utils::UInt qubit, utils::Real angle);
    void rz(utils::UInt qubit, utils::Real angle);
    void s(utils::UInt qubit);
    void sdag(utils::UInt qubit);
    void t(utils::UInt qubit);
    void tdag(utils::UInt qubit);
    void x(utils::UInt qubit);
    void y(utils::UInt qubit);
    void z(utils::UInt qubit);
    void rx90(utils::UInt qubit);
    void mrx90(utils::UInt qubit);
    void rx180(utils::UInt qubit);
    void ry90(utils::UInt qubit);
    void mry90(utils::UInt qubit);
    void ry180(utils::UInt qubit);
    void measure(utils::UInt qubit);
    void measure(utils::UInt qubit, utils::UInt bit);
    void prepz(utils::UInt qubit);
    void cnot(utils::UInt qubit1, utils::UInt qubit2);
    void cz(utils::UInt qubit1, utils::UInt qubit2);
    void cphase(utils::UInt qubit1, utils::UInt qubit2);
    void toffoli(utils::UInt qubit1, utils::UInt qubit2, utils::UInt qubit3);
    void swap(utils::UInt qubit1, utils::UInt qubit2);
    void wait(const utils::Vec<utils::UInt> &qubits, utils::UInt duration);
    void display();
    void clifford(utils::Int id, utils::UInt qubit=0);

private:
    // a default gate is the last resort of user gate resolution and is of a build-in form, as below in the code;
    // the "using_default_gates" option can be used to enable ("yes") or disable ("no") default gates;
    // the use of default gates is deprecated; use the .json configuration file instead to define custom gates;
    //
    // if a default gate definition is available for the given gate name and qubits, add it to circuit and return true
    utils::Bool add_default_gate_if_available(
        const utils::Str &gname,
        const utils::Vec<utils::UInt> &qubits,
        const utils::Vec<utils::UInt> &cregs = {},
        utils::UInt duration = 0,
        utils::Real angle = 0.0,
        const utils::Vec<utils::UInt> &bregs = {},
        cond_type_t gcond = cond_always,
        const utils::Vec<utils::UInt> &gcondregs = {}
    );

    // if a specialized custom gate ("e.g. cz q0,q4") is available, add it to circuit and return true
    // if a parameterized custom gate ("e.g. cz") is available, add it to circuit and return true
    //
    // note that there is no check for the found gate being a composite gate
    utils::Bool add_custom_gate_if_available(
        const utils::Str &gname,
        const utils::Vec<utils::UInt> &qubits,
        const utils::Vec<utils::UInt> &cregs = {},
        utils::UInt duration = 0,
        utils::Real angle = 0.0,
        const utils::Vec<utils::UInt> &bregs = {},
        cond_type_t gcond = cond_always,
        const utils::Vec<utils::UInt> &gcondregs = {}
    );

    // FIXME: move to class composite_gate?
    // return the subinstructions of a composite gate
    // while doing, test whether the subinstructions have a definition (so they cannot be specialized or default ones!)
    void get_decomposed_ins(
        const composite_gate *gptr,
        utils::Vec<utils::Str> &sub_instructons
    ) const;

    // if specialized composed gate: "e.g. cz q0,q3" available, with composition of subinstructions, return true
    //      also check each subinstruction for presence as a custom_gate (or a default gate)
    // otherwise, return false
    // rely on add_custom_gate_if_available or add_default_gate_if_available to add each subinstruction to the circuit
    //
    // add specialized decomposed gate, example JSON definition: "cl_14 q1": ["rx90 %0", "rym90 %0", "rxm90 %0"]
    utils::Bool add_spec_decomposed_gate_if_available(
        const utils::Str &gate_name,
        const utils::Vec<utils::UInt> &all_qubits,
        const utils::Vec<utils::UInt> &cregs = {},
        const utils::Vec<utils::UInt> &bregs = {},
        cond_type_t gcond = cond_always,
        const utils::Vec<utils::UInt> &gcondregs = {}
    );

    // if composite gate: "e.g. cz %0 %1" available, return true;
    //      also check each subinstruction for availability as a custom gate (or default gate)
    // if not, return false
    // rely on add_custom_gate_if_available or add_default_gate_if_available to add each subinstruction to the circuit
    //
    // add parameterized decomposed gate, example JSON definition: "cl_14 %0": ["rx90 %0", "rym90 %0", "rxm90 %0"]
    utils::Bool add_param_decomposed_gate_if_available(
        const utils::Str &gate_name,
        const utils::Vec<utils::UInt> &all_qubits,
        const utils::Vec<utils::UInt> &cregs = {},
        const utils::Vec<utils::UInt> &bregs = {},
        cond_type_t gcond = cond_always,
        const utils::Vec<utils::UInt> &gcondregs = {}
    );

public:

    void gate(const utils::Str &gname, utils::UInt q0);
    void gate(const utils::Str &gname, utils::UInt q0, utils::UInt q1);
    void gate(
        const utils::Str &gname,
        const utils::Vec<utils::UInt> &qubits = {},
        const utils::Vec<utils::UInt> &cregs = {},
        utils::UInt duration = 0,
        utils::Real angle = 0.0,
        const utils::Vec<utils::UInt> &bregs = {},
        cond_type_t gcond = cond_always,
        const utils::Vec<utils::UInt> &gcondregs = {}
    );
    void gate_preset_condition(
        cond_type_t gcond,
        const utils::Vec<utils::UInt> &gcondregs
    );
    void gate_clear_condition();
    void condgate(
        const utils::Str &gname,
        const utils::Vec<utils::UInt> &qubits,
        cond_type_t gcond,
        const utils::Vec<utils::UInt> &gcondregs
    );
    // to add unitary to kernel
    void gate(const unitary &u, const utils::Vec<utils::UInt> &qubits);

    // terminology:
    // - composite/custom/default (in decreasing order of priority during lookup in the gate definition):
    //      - composite gate: a gate definition with subinstructions; when matched, decompose and add the subinstructions
    //      - custom gate: a fully configurable gate definition, with all kinds of attributes; there is no decomposition
    //      - default gate: a gate definition build-in in this compiler; see above for the definition
    //          deprecated; setting option "use_default_gates" from "yes" to "no" turns it off
    // - specialized/parameterized (in decreasing order of priority during lookup in the gate definition)
    //      - specialized: a gate definition that is special for its operands, i.e. the operand qubits must match
    //      - parameterized: a gate definition that can be used for all possible qubit operands
    //
    // the following order of checks is used below:
    // check if specialized composite gate is available
    //      e.g. whether "cz q0,q3" is available as composite gate, where subinstructions are available as custom gates
    // if not, check if parameterized composite gate is available
    //      e.g. whether "cz %0,%1" is in gate_definition, where subinstructions are available as custom gates
    // if not, check if a specialized custom gate is available
    //      e.g. whether "cz q0,q3" is available as non-composite gate
    // if not, check if a parameterized custom gate is available
    //      e.g. whether "cz" is in gate_definition as non-composite gate
    // if not, check if a default gate is available
    //      e.g. whether "cz" is available as default gate
    // if not, then FATAL (for gate()) or return false (for gate_nonfatal())

    /**
     * custom gate with arbitrary number of operands
     * as gate above but return whether gate was successfully matched in gate_definition, next to gate in kernel.c
     */
    utils::Bool gate_nonfatal(
        const utils::Str &gname,
        const utils::Vec<utils::UInt> &qubits = {},
        const utils::Vec<utils::UInt> &cregs = {},
        utils::UInt duration = 0,
        utils::Real angle = 0.0,
        const utils::Vec<utils::UInt> &bregs = {},
        cond_type_t gcond = cond_always,
        const utils::Vec<utils::UInt> &gcondregs = {}
    );

    /**
     * support function for Python conditional execution interfaces to pass condition
     */
    ql::cond_type_t condstr2condvalue(const std::string &condstring);

private:
    void gate_add_implicits(
        const utils::Str &gname,
        utils::Vec<utils::UInt> &qubits,
        utils::Vec<utils::UInt> &cregs,
        utils::UInt &duration,
        utils::Real &angle,
        utils::Vec<utils::UInt> &bregs,
        cond_type_t &gcond,
        const utils::Vec<utils::UInt> &gcondregs
    );

    //recursive gate count function
    //n is number of qubits
    //i is the start point for the instructionlist
    utils::Int recursiveRelationsForUnitaryDecomposition(
        const unitary &u,
        const utils::Vec<utils::UInt> &qubits,
        utils::UInt n,
        utils::UInt i
    );

    //controlled qubit is the first in the list.
    void multicontrolled_rz(
        const utils::Vec<utils::Real> &instruction_list,
        utils::UInt start_index,
        utils::UInt end_index,
        const utils::Vec<utils::UInt> &qubits
    );

    //controlled qubit is the first in the list.
    void multicontrolled_ry(
        const utils::Vec<utils::Real> &instruction_list,
        utils::UInt start_index,
        utils::UInt end_index,
        const utils::Vec<utils::UInt> &qubits
    );

public:

    /**
     * qasm output
     */
    // FIXME: create a separate QASM backend?
    utils::Str get_prologue() const;
    utils::Str get_epilogue() const;
    utils::Str qasm() const;

    void classical(const creg &destination, const operation &oper);
    void classical(const utils::Str &operation);

    // Controlled gates
    void controlled_x(utils::UInt tq, utils::UInt cq);
    void controlled_y(utils::UInt tq, utils::UInt cq);
    void controlled_z(utils::UInt tq, utils::UInt cq);
    void controlled_h(utils::UInt tq, utils::UInt cq);
    void controlled_i(utils::UInt tq, utils::UInt cq);
    void controlled_s(utils::UInt tq, utils::UInt cq);
    void controlled_sdag(utils::UInt tq, utils::UInt cq);
    void controlled_t(utils::UInt tq, utils::UInt cq, utils::UInt aq);
    void controlled_tdag(utils::UInt tq, utils::UInt cq, utils::UInt aq);
    void controlled_ix(utils::UInt tq, utils::UInt cq);
    void controlled_cnot_AM(utils::UInt tq, utils::UInt cq1, utils::UInt cq2);
    void controlled_cnot_NC(utils::UInt tq, utils::UInt cq1, utils::UInt cq2);
    void controlled_swap(utils::UInt tq1, utils::UInt tq2, utils::UInt cq);
    void controlled_rx(utils::UInt tq, utils::UInt cq, utils::Real theta);
    void controlled_ry(utils::UInt tq, utils::UInt cq, utils::Real theta);
    void controlled_rz(utils::UInt tq, utils::UInt cq, utils::Real theta);

    /************************************************************************\
    | Kernel manipulations: controlled & conjugate
    \************************************************************************/

    void controlled_single(
        const quantum_kernel *k,
        utils::UInt control_qubit,
        utils::UInt ancilla_qubit
    );
    void controlled(
        const quantum_kernel *k,
        const utils::Vec<utils::UInt> &control_qubits,
        const utils::Vec<utils::UInt> &ancilla_qubits
    );
    void conjugate(const quantum_kernel *k);

};

} // namespace ql
