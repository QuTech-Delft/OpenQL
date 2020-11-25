/**
 * @file   kernel.h
 * @date   04/2017
 * @author Nader Khammassi
 *         Imran Ashraf
 *         Anneriet Krol
 * @brief  openql kernel
 */

#pragma once

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
    std::string   name;
    size_t        iterations;
    size_t        qubit_count;
    size_t        creg_count;
    size_t        breg_count;
    kernel_type_t type;
    circuit       c;
    bool          cycles_valid; // used in bundler to check if kernel has been scheduled
    operation     br_condition;
    size_t        cycle_time;   // FIXME HvS just a copy of platform.cycle_time
    instruction_map_t instruction_map;

public:
    quantum_kernel(const std::string &name);
    quantum_kernel(
        const std::string &name,
        const ql::quantum_platform &platform,
        size_t qcount,
        size_t ccount=0,
        size_t bcount=0
    );

    // FIXME: add constructor which allows setting iterations and type, and use that in program.h::add_for(), etc

    void set_condition(const operation &oper);
    void set_kernel_type(kernel_type_t typ);

    std::string get_gates_definition() const;
    std::string get_name() const;
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
    void measure(size_t qubit, size_t bit);
    void prepz(size_t qubit);
    void cnot(size_t qubit1, size_t qubit2);
    void cz(size_t qubit1, size_t qubit2);
    void cphase(size_t qubit1, size_t qubit2);
    void toffoli(size_t qubit1, size_t qubit2, size_t qubit3);
    void swap(size_t qubit1, size_t qubit2);
    void wait(const std::vector<size_t> &qubits, size_t duration);
    void display();
    void clifford(int id, size_t qubit=0);

private:
    // a default gate is the last resort of user gate resolution and is of a build-in form, as below in the code;
    // the "using_default_gates" option can be used to enable ("yes") or disable ("no") default gates;
    // the use of default gates is deprecated; use the .json configuration file instead;
    //
    // if a default gate definition is available for the given gate name and qubits, add it to circuit and return true
    bool add_default_gate_if_available(
        const std::string &gname,
        const std::vector<size_t> &qubits,
        const std::vector<size_t> &cregs,
        size_t duration,
        double angle,
        const std::vector<size_t> &bregs,
        const cond_type_t &cond,
        const std::vector<size_t> &condregs
    );

    // if a specialized custom gate ("e.g. cz q0,q4") is available, add it to circuit and return true
    // if a parameterized custom gate ("e.g. cz") is available, add it to circuit and return true
    //
    // note that there is no check for the found gate being a composite gate
    bool add_custom_gate_if_available(
        const std::string &gname,
        const std::vector<size_t> &qubits,
        const std::vector<size_t> &cregs,
        size_t duration,
        double angle,
        const std::vector<size_t> &bregs,
        const cond_type_t &cond,
        const std::vector<size_t> &condregs
    );

    // FIXME: move to class composite_gate?
    // return the subinstructions of a composite gate
    // while doing, test whether the subinstructions have a definition (so they cannot be specialized or default ones!)
    void get_decomposed_ins(
        const ql::composite_gate *gptr,
        std::vector<std::string> &sub_instructons
    ) const;

    // if specialized composed gate: "e.g. cz q0,q3" available, with composition of subinstructions, return true
    //      also check each subinstruction for presence of a custom_gate (or a default gate)
    // otherwise, return false
    // don't add anything to circuit
    //
    // add specialized decomposed gate, example JSON definition: "cl_14 q1": ["rx90 %0", "rym90 %0", "rxm90 %0"]
    bool add_spec_decomposed_gate_if_available(
        const std::string &gate_name,
        const std::vector<size_t> &all_qubits,
        const std::vector<size_t> &cregs,
        const std::vector<size_t> &bregs,
        const cond_type_t &cond,
        const std::vector<size_t> &condregs
    );

    // if composite gate: "e.g. cz %0 %1" available, return true;
    //      also check each subinstruction for availability as a custom gate (or default gate)
    // if not, return false
    // don't add anything to circuit
    //
    // add parameterized decomposed gate, example JSON definition: "cl_14 %0": ["rx90 %0", "rym90 %0", "rxm90 %0"]
    bool add_param_decomposed_gate_if_available(
        const std::string &gate_name,
        const std::vector<size_t> &all_qubits,
        const std::vector<size_t> &cregs,
        const std::vector<size_t> &bregs,
        const cond_type_t &cond,
        const std::vector<size_t> &condregs
    );

public:

    void gate(const std::string &gname, size_t q0);
    void gate(const std::string &gname, size_t q0, size_t q1);
    void gate(
        const std::string &gname,
        const std::vector<size_t> &qubits = {},
        const std::vector<size_t> &cregs = {},
        size_t duration = 0,
        double angle = 0.0,
        const std::vector<size_t> &bregs = {},
        const cond_type_t &cond = cond_always,
        const std::vector<size_t> &condregs = {}
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
    // if not, then FATAL (for gate()) or return false (for gate_nonfatal())
    /**
     * custom gate with arbitrary number of operands
     * as gate above but return whether gate was successfully matched in gate_definition, next to gate in kernel.c
     */
    bool gate_nonfatal(
        const std::string &gname,
        const std::vector<size_t> &qubits = {},
        const std::vector<size_t> &cregs = {},
        size_t duration = 0,
        double angle = 0.0,
        const std::vector<size_t> &bregs = {},
        const cond_type_t &cond = cond_always,
        const std::vector<size_t> &condregs = {}
    );

    // to add unitary to kernel
    void gate(const ql::unitary &u, const std::vector<size_t> &qubits);

private:
    //recursive gate count function
    //n is number of qubits
    //i is the start point for the instructionlist
    int recursiveRelationsForUnitaryDecomposition(
        const ql::unitary &u,
        const std::vector<size_t> &qubits,
        int n,
        int i
    );

    //controlled qubit is the first in the list.
    void multicontrolled_rz(
        const std::vector<double> &instruction_list,
        int start_index,
        int end_index,
        const std::vector<size_t> &qubits
    );

    //controlled qubit is the first in the list.
    void multicontrolled_ry(
        const std::vector<double> &instruction_list,
        int start_index,
        int end_index,
        const std::vector<size_t> &qubits
    );

public:

    /**
     * qasm output
     */
    // FIXME: create a separate QASM backend?
    std::string get_prologue() const;
    std::string get_epilogue() const;
    std::string qasm() const;

    void classical(const creg &destination, const operation &oper);
    void classical(const std::string &operation);

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
        const ql::quantum_kernel *k,
        size_t control_qubit,
        size_t ancilla_qubit
    );
    void controlled(
        const ql::quantum_kernel *k,
        const std::vector<size_t> &control_qubits,
        const std::vector<size_t> &ancilla_qubits
    );
    void conjugate(const ql::quantum_kernel *k);

};

} // namespace ql
