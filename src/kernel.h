/**
 * @file   kernel.h
 * @date   04/2017
 * @author Nader Khammassi
 *         Imran Ashraf
 *         Anneriet Krol
 * @brief  openql kernel
 */

#pragma once

#include "utils/str.h"
#include "utils/vec.h"
#include "utils/opt.h"
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
    utils::Str     name;
    size_t         iterations;
    size_t         qubit_count;
    size_t         creg_count;
    kernel_type_t  type;
    circuit        c;
    bool           cycles_valid; // used in bundler to check if kernel has been scheduled
    utils::Opt<operation> br_condition;
    size_t         cycle_time;   // FIXME HvS just a copy of platform.cycle_time
    instruction_map_t instruction_map;

public:
    quantum_kernel(const utils::Str &name);
    quantum_kernel(
        const utils::Str &name,
        const quantum_platform &platform,
        size_t qcount,
        size_t ccount=0
    );

    // FIXME: add constructor which allows setting iterations and type, and use that in program.h::add_for(), etc

    void set_condition(const operation &oper);
    void set_kernel_type(kernel_type_t typ);

    utils::Str get_gates_definition() const;
    utils::Str get_name() const;
    circuit &get_circuit();
    const circuit &get_circuit() const;

    void identity(size_t qubit);
    void i(size_t qubit);
    void hadamard(size_t qubit);
    void h(size_t qubit);
    void rx(size_t qubit, double angle);
    void ry(size_t qubit, double angle);
    void rz(size_t qubit, double angle);
    void s(size_t qubit);
    void sdag(size_t qubit);
    void t(size_t qubit);
    void tdag(size_t qubit);
    void x(size_t qubit);
    void y(size_t qubit);
    void z(size_t qubit);
    void rx90(size_t qubit);
    void mrx90(size_t qubit);
    void rx180(size_t qubit);
    void ry90(size_t qubit);
    void mry90(size_t qubit);
    void ry180(size_t qubit);
    void measure(size_t qubit);
    void prepz(size_t qubit);
    void cnot(size_t qubit1, size_t qubit2);
    void cz(size_t qubit1, size_t qubit2);
    void cphase(size_t qubit1, size_t qubit2);
    void toffoli(size_t qubit1, size_t qubit2, size_t qubit3);
    void swap(size_t qubit1, size_t qubit2);
    void wait(const utils::Vec<size_t> &qubits, size_t duration);
    void display();
    void clifford(int id, size_t qubit=0);

private:
    // a default gate is the last resort of user gate resolution and is of a build-in form, as below in the code;
    // the "using_default_gates" option can be used to enable ("yes") or disable ("no") default gates;
    // the use of default gates is deprecated; use the .json configuration file instead;
    //
    // if a default gate definition is available for the given gate name and qubits, add it to circuit and return true
    bool add_default_gate_if_available(
        const utils::Str &gname,
        const utils::Vec<size_t> &qubits,
        const utils::Vec<size_t> &cregs = {},
        size_t duration=0,
        double angle=0.0
    );

    // if a specialized custom gate ("e.g. cz q0,q4") is available, add it to circuit and return true
    // if a parameterized custom gate ("e.g. cz") is available, add it to circuit and return true
    //
    // note that there is no check for the found gate being a composite gate
    bool add_custom_gate_if_available(
        const utils::Str &gname,
        const utils::Vec<size_t> &qubits,
        const utils::Vec<size_t> &cregs = {},
        size_t duration=0,
        double angle=0.0
    );

    // FIXME: move to class composite_gate?
    // return the subinstructions of a composite gate
    // while doing, test whether the subinstructions have a definition (so they cannot be specialized or default ones!)
    void get_decomposed_ins(
        const composite_gate *gptr,
        utils::Vec<utils::Str> &sub_instructons
    ) const;

    // if specialized composed gate: "e.g. cz q0,q3" available, with composition of subinstructions, return true
    //      also check each subinstruction for presence of a custom_gate (or a default gate)
    // otherwise, return false
    // don't add anything to circuit
    //
    // add specialized decomposed gate, example JSON definition: "cl_14 q1": ["rx90 %0", "rym90 %0", "rxm90 %0"]
    bool add_spec_decomposed_gate_if_available(
        const utils::Str &gate_name,
        const utils::Vec<size_t> &all_qubits,
        const utils::Vec<size_t> &cregs = {}
    );

    // if composite gate: "e.g. cz %0 %1" available, return true;
    //      also check each subinstruction for availability as a custom gate (or default gate)
    // if not, return false
    // don't add anything to circuit
    //
    // add parameterized decomposed gate, example JSON definition: "cl_14 %0": ["rx90 %0", "rym90 %0", "rxm90 %0"]
    bool add_param_decomposed_gate_if_available(
        const utils::Str &gate_name,
        const utils::Vec<size_t> &all_qubits,
        const utils::Vec<size_t> &cregs = {}
    );

public:

    void gate(const utils::Str &gname, size_t q0);
    void gate(const utils::Str &gname, size_t q0, size_t q1);
    void gate(
        const utils::Str &gname,
        const utils::Vec<size_t> &qubits = {},
        const utils::Vec<size_t> &cregs = {},
        size_t duration = 0,
        double angle = 0.0
    );

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
    // if not, then error
    /**
     * custom gate with arbitrary number of operands
     * as gate above but return whether gate was successfully matched in gate_definition, next to gate in kernel.c
     */
    bool gate_nonfatal(
        const utils::Str &gname,
        const utils::Vec<size_t> &qubits = {},
        const utils::Vec<size_t> &cregs = {},
        size_t duration = 0,
        double angle = 0.0
    );

    // to add unitary to kernel
    void gate(const unitary &u, const utils::Vec<size_t> &qubits);

private:
    //recursive gate count function
    //n is number of qubits
    //i is the start point for the instructionlist
    int recursiveRelationsForUnitaryDecomposition(
        const unitary &u,
        const utils::Vec<size_t> &qubits,
        int n,
        int i
    );

    //controlled qubit is the first in the list.
    void multicontrolled_rz(
        const utils::Vec<double> &instruction_list,
        int start_index,
        int end_index,
        const utils::Vec<size_t> &qubits
    );

    //controlled qubit is the first in the list.
    void multicontrolled_ry(
        const utils::Vec<double> &instruction_list,
        int start_index,
        int end_index,
        const utils::Vec<size_t> &qubits
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
    void controlled_x(size_t tq, size_t cq);
    void controlled_y(size_t tq, size_t cq);
    void controlled_z(size_t tq, size_t cq);
    void controlled_h(size_t tq, size_t cq);
    void controlled_i(size_t tq, size_t cq);
    void controlled_s(size_t tq, size_t cq);
    void controlled_sdag(size_t tq, size_t cq);
    void controlled_t(size_t tq, size_t cq, size_t aq);
    void controlled_tdag(size_t tq, size_t cq, size_t aq);
    void controlled_ix(size_t tq, size_t cq);
    void controlled_cnot_AM(size_t tq, size_t cq1, size_t cq2);
    void controlled_cnot_NC(size_t tq, size_t cq1, size_t cq2);
    void controlled_swap(size_t tq1, size_t tq2, size_t cq);
    void controlled_rx(size_t tq, size_t cq, double theta);
    void controlled_ry(size_t tq, size_t cq, double theta);
    void controlled_rz(size_t tq, size_t cq, double theta);

    /************************************************************************\
    | Kernel manipulations: controlled & conjugate
    \************************************************************************/

    void controlled_single(
        const quantum_kernel *k,
        size_t control_qubit,
        size_t ancilla_qubit
    );
    void controlled(
        const quantum_kernel *k,
        const utils::Vec<size_t> &control_qubits,
        const utils::Vec<size_t> &ancilla_qubits
    );
    void conjugate(const quantum_kernel *k);

};

} // namespace ql
