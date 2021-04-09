/** \file
 * Quantum kernel abstraction implementation.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/vec.h"
#include "ql/utils/opt.h"
#include "ql/plat/platform.h"
#include "ql/ir/gate.h"
#include "ql/ir/circuit.h"
#include "ql/ir/classical.h"

namespace ql {

// FIXME: this should not be here. IR headers should not have to include
//  anything not from ql::ir, ql::plat, or ql::utils; they should be dumb
//  containers. But decomposition etc. is still part of Kernel.
namespace com {
class Unitary;
} // namespace com

namespace ir {

/**
 * The role of a kernel in control-flow representation.
 *
 * FIXME: this representation of control-flow, while complete, is very poorly
 *  engineered. The recursive structure is flattened and thus difficult to
 *  deduce, there is redundant data everywhere, etc.
 */
enum class KernelType {
    STATIC,
    FOR_START, FOR_END,
    DO_WHILE_START, DO_WHILE_END,
    IF_START, IF_END,
    ELSE_START, ELSE_END
};

/**
 * A single kernel of a program, a.k.a. a basic block.
 */
class Kernel : public utils::Node {
public: // FIXME: should be private
    /**
     * Name given to the kernel by the user.
     */
    utils::Str name;

    /**
     * The platform associated with the kernel.
     *
     * TODO: this doesn't really belong here, but is currently necessary because
     *  the gate constructors are part of the kernel. Rather, gates should be
     *  constructed by the platform and then added to the kernel, in much the
     *  same way that kernels are created using the platform and then added to
     *  a program.
     */
    plat::PlatformRef platform;

    /**
     * Number of (virtual) qubits used by this kernel. Must be less than or
     * equal to the number of qubits in the platform. When the qubits represent
     * physical qubits (post-mapping), this must equal the number of qubits in
     * the platform.
     */
    utils::UInt qubit_count;

    /**
     * Number of (virtual) 32-bit general-purpose classical registers used by
     * this kernel. Must be less than or equal to the number of registers in the
     * platform.
     */
    utils::UInt creg_count;

    /**
     * Number of (virtual) single-bit condition registers used by this kernel.
     * Must be less than or equal to the number of registers in the platform.
     *
     * FIXME: code is not consistent about what a breg means. I (JvS) thought we
     *  were using the first num_qubits bregs as registers that always exist and
     *  implicitly receive measurement results when no breg is manually
     *  specified, and use num_qubits..num_qubits+breg_count for user-specified
     *  state variables. But that's not how it works at all; bregs are still
     *  usually implicit, code all over the place assumes that bregs only range
     *  up to breg_count (exclusive), and breg_count defaults to zero. I don't
     *  get it.
     */
    utils::UInt breg_count;

    /**
     * The list of gates that forms the body of the kernel.
     */
    Circuit c;

    /**
     * The classical control-flow behavior of this kernel.
     */
    KernelType type;

    /**
     * The number of iterations that this kernel must be run for. Exact usage
     * (if any) depends on type.
     */
    utils::UInt iteration_count;

    /**
     * The branch condition for this kernel. Exact usage (if any) depends on
     * type.
     */
    utils::Opt<ClassicalOperation> br_condition;

    /**
     * Whether the cycle numbers attached to the gates in the circuit are
     * considered to be valid. Used by the bundler to see if the kernel has been
     * scheduled.
     */
    utils::Bool cycles_valid;

    /**
     * A conditional gate type used when adding gates to the kernel.
     *
     * FIXME: does NOT exactly serve as a condition for the kernel itself unless
     *  it's only at construction, not changed after that, and all kernels are
     *  added via the kernel.gate()-like functions (rather than being added to
     *  the circuit directly, as done by unitary decomposition, for example.
     */
    ConditionType condition;

    /**
     * Operands for the above condition.
     */
    utils::Vec<utils::UInt> cond_operands;

public:

    Kernel(
        const utils::Str &name,
        const plat::PlatformRef &platform,
        utils::UInt qubit_count,
        utils::UInt creg_count=0,
        utils::UInt breg_count=0
    );

    // FIXME: add constructor which allows setting iterations and type, and use that in program.h::add_for(), etc

    void set_condition(const ClassicalOperation &oper);
    void set_kernel_type(KernelType typ);

    utils::Str get_gates_definition() const;
    utils::Str get_name() const;
    Circuit &get_circuit();
    const Circuit &get_circuit() const;

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
        ConditionType gcond = ConditionType::ALWAYS,
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
        ConditionType gcond = ConditionType::ALWAYS,
        const utils::Vec<utils::UInt> &gcondregs = {}
    );

    // FIXME: move to class composite_gate?
    // return the subinstructions of a composite gate
    // while doing, test whether the subinstructions have a definition (so they cannot be specialized or default ones!)
    void get_decomposed_ins(
        const gates::Composite &gate,
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
        ConditionType gcond = ConditionType::ALWAYS,
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
        ConditionType gcond = ConditionType::ALWAYS,
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
        ConditionType gcond = ConditionType::ALWAYS,
        const utils::Vec<utils::UInt> &gcondregs = {}
    );
    void gate_preset_condition(
        ConditionType gcond,
        const utils::Vec<utils::UInt> &gcondregs
    );
    void gate_clear_condition();
    void condgate(
        const utils::Str &gname,
        const utils::Vec<utils::UInt> &qubits,
        ConditionType gcond,
        const utils::Vec<utils::UInt> &gcondregs
    );
    // to add unitary to kernel
    void gate(const com::Unitary &u, const utils::Vec<utils::UInt> &qubits);

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
        ConditionType gcond = ConditionType::ALWAYS,
        const utils::Vec<utils::UInt> &gcondregs = {}
    );

    /**
     * support function for Python conditional execution interfaces to pass condition
     */
    ConditionType condstr2condvalue(const std::string &condstring);

private:
    void gate_add_implicits(
        const utils::Str &gname,
        utils::Vec<utils::UInt> &qubits,
        utils::Vec<utils::UInt> &cregs,
        utils::UInt &duration,
        utils::Real &angle,
        utils::Vec<utils::UInt> &bregs,
        ConditionType &gcond,
        const utils::Vec<utils::UInt> &gcondregs
    );

public:

    /**
     * qasm output
     */
    // FIXME: create a separate QASM backend?
    utils::Str get_prologue() const;
    utils::Str get_epilogue() const;
    utils::Str qasm() const;

    void classical(const ClassicalRegister &destination, const ClassicalOperation &oper);
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
        const Kernel &k,
        utils::UInt control_qubit,
        utils::UInt ancilla_qubit
    );
    void controlled(
        const Kernel &k,
        const utils::Vec<utils::UInt> &control_qubits,
        const utils::Vec<utils::UInt> &ancilla_qubits
    );
    void conjugate(const Kernel &k);

};

/**
 * A "reference" (actually a smart pointer) to a single kernel node.
 */
using KernelRef = utils::One<Kernel>;

/**
 * A vector of "references" (actually smart pointers) to kernel nodes.
 */
using KernelRefs = utils::Any<Kernel>;

} // namespace ir
} // namespace ql
