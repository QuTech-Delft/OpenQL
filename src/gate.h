/** \file
 * Quantum gate abstraction implementation.
 */

#pragma once

#include "ql/utils/str.h"
#include "ql/utils/vec.h"
#include "ql/utils/json.h"
#include "ql/utils/misc.h"
#include "matrix.h"

namespace ql {

typedef utils::Str instruction_t;

// gate types
enum class GateType {
    IDENTITY,
    HADAMARD,
    PAULI_X,
    PAULI_Y,
    PAULI_Z,
    PHASE,
    PHASE_DAG,
    T,
    T_DAG,
    RX90,
    RXM90,
    RX180,
    RY90,
    RYM90,
    RY180,
    RX,
    RY,
    RZ,
    PREP_Z,
    CNOT,
    CPHASE,
    TOFFOLI,
    CUSTOM,
    COMPOSITE,
    MEASURE,
    DISPLAY,
    DISPLAY_BINARY,
    NOP,
    DUMMY,
    SWAP,
    WAIT,
    CLASSICAL
};

std::ostream &operator<<(std::ostream &os, GateType gate_type);

const utils::Complex identity_c[] = {
    1.0, 0.0,
    0.0, 1.0
};

const utils::Complex pauli_x_c[] {
    0.0, 1.0,
    1.0, 0.0
};

const utils::Complex pauli_y_c[] {
    0.0, -utils::IM,
    utils::IM, 0.0
};

const utils::Complex pauli_z_c[] {
    1.0, 0.0,
    0.0, -1.0
};

const utils::Complex hadamard_c[] = {
    utils::sqrt(0.5), utils::sqrt(0.5),
    utils::sqrt(0.5), -utils::sqrt(0.5)
};

const utils::Complex phase_c[] {
    1.0, 0.0,
    0.0, utils::IM
};

const utils::Complex phasedag_c[] = {
    1.0, 0.0,
    0.0, -utils::IM
};

const utils::Complex t_c[] = {
    1.0, 0.0,
    0.0, utils::expi(0.25 * utils::PI)
};

const utils::Complex tdag_c[] = {
    1.0, 0.0,
    0.0, utils::expi(-0.25 * utils::PI)
};

const utils::Complex rx90_c[] = {
    utils::sqrt(0.5), -utils::IM * utils::sqrt(0.5),
    -utils::IM * utils::sqrt(0.5), utils::sqrt(0.5)
};

const utils::Complex ry90_c[] = {
    utils::sqrt(0.5), -utils::sqrt(0.5),
    utils::sqrt(0.5), utils::sqrt(0.5)
};

const utils::Complex mrx90_c[] = {
    utils::sqrt(0.5), utils::IM * utils::sqrt(0.5),
    utils::IM * utils::sqrt(0.5), utils::sqrt(0.5)
};

const utils::Complex mry90_c[] = {
    utils::sqrt(0.5), utils::sqrt(0.5),
    -utils::sqrt(0.5), utils::sqrt(0.5)
};

const utils::Complex rx180_c[] = {
    0.0, -utils::IM,
    -utils::IM, 0.0
};

const utils::Complex ry180_c[] = {
    0.0, -utils::IM,
    -utils::IM, 0.0
};

/**
 * to do : multi-qubit gates should not be represented by their matrix (the matrix depends on the ctrl/target qubit locations, the simulation using such matrix is inefficient as well...)
 */

const utils::Complex cnot_c[] = {
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 1.0,
    0.0, 0.0, 1.0, 0.0
};

const utils::Complex cphase_c[] = {
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, -1.0
};

const utils::Complex swap_c[] = {
    1.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 1.0
};

const utils::Complex toffoli_c[] = {
    1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0
};

const utils::Complex nop_c[] = {
    1.0, 0.0,
    0.0, 1.0
};

/*
 * additional definitions for describing conditional gates
 */
typedef enum e_cond_type {
    // 0 operands:
    cond_always, cond_never,
    // 1 operand:
    cond_unary, cond_not,
    // 2 operands
    cond_and, cond_nand, cond_or, cond_nor, cond_xor, cond_nxor
} cond_type_t;

const utils::UInt MAX_CYCLE = utils::MAX;

struct swap_parameters {
    utils::Bool part_of_swap = false;
    // at the end of the swap r0 stores v0 and r1 stores v1
    utils::Int r0 = -1;
    utils::Int r1 = -1;
    utils::Int v0 = -1;
    utils::Int v1 = -1;

    // default constructor
    swap_parameters() {}

    // initializer list
    swap_parameters(utils::Bool _part_of_swap, utils::Int _r0, utils::Int _r1, utils::Int _v0, utils::Int _v1)
        : part_of_swap(_part_of_swap), r0(_r0), r1(_r1), v0(_v0), v1(_v1)
    {}
};

/**
 * gate interface
 */
class gate {
public:
    utils::Str name;
    utils::Vec<utils::UInt> operands;             // qubit operands
    utils::Vec<utils::UInt> creg_operands;
    utils::Vec<utils::UInt> breg_operands;        // bit operands e.g. assigned to by measure; cond_operands are separate
    utils::Vec<utils::UInt> cond_operands;        // 0, 1 or 2 bit operands of condition
    cond_type_t condition = cond_always;          // defines condition and by that number of bit operands of condition
    swap_parameters swap_params;                  // if the gate is part of a swap/move, this will contain the real and virtual qubits involved
    utils::Int int_operand = 0;                   // FIXME: move to class 'classical'
    utils::UInt duration = 0;
    utils::Real angle = 0.0;                      // for arbitrary rotations
    utils::UInt cycle = MAX_CYCLE;                // cycle after scheduling; MAX_CYCLE indicates undefined
    virtual ~gate() = default;
    virtual instruction_t qasm() const = 0;
    virtual GateType      type() const = 0;
    virtual cmat_t        mat()  const = 0;  // to do : change cmat_t type to avoid stack smashing on 2 qubits gate operations
    utils::Str visual_type = ""; // holds the visualization type of this gate that will be linked to a specific configuration in the visualizer
    utils::Bool is_conditional() const;           // whether gate has condition that is NOT cond_always
    instruction_t cond_qasm() const;              // returns the condition expression in qasm layout
    static utils::Bool is_valid_cond(cond_type_t condition, const utils::Vec<utils::UInt> &cond_operands);
};



/****************************************************************************\
| Standard gates
\****************************************************************************/

class identity : public gate {
public:
    cmat_t m;
    explicit identity(utils::UInt q);
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

class hadamard : public gate {
public:
    cmat_t m;
    explicit hadamard(utils::UInt q);
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

class phase : public gate {
public:
    cmat_t m;
    explicit phase(utils::UInt q);
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

class phasedag : public gate {
public:
    cmat_t m;
    explicit phasedag(utils::UInt q);
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

class rx : public gate {
public:
    cmat_t m;
    rx(utils::UInt q, utils::Real theta);
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

class ry : public gate {
public:
    cmat_t m;
    ry(utils::UInt q, utils::Real theta);
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

class rz : public gate {
public:
    cmat_t m;
    rz(utils::UInt q, utils::Real theta);
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

class t : public gate {
public:
    cmat_t m;
    explicit t(utils::UInt q);
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

class tdag : public gate {
public:
    cmat_t m;
    explicit tdag(utils::UInt q);
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

class pauli_x : public gate {
public:
    cmat_t m;
    explicit pauli_x(utils::UInt q);
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

class pauli_y : public gate {
public:
    cmat_t m;
    explicit pauli_y(utils::UInt q);
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

class pauli_z : public gate {
public:
    cmat_t m;
    explicit pauli_z(utils::UInt q);
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

class rx90 : public gate {
public:
    cmat_t m;
    explicit rx90(utils::UInt q);
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

class mrx90 : public gate {
public:
    cmat_t m;
    explicit mrx90(utils::UInt q);
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

class rx180 : public gate {
public:
    cmat_t m;
    explicit rx180(utils::UInt q);
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

class ry90 : public gate {
public:
    cmat_t m;
    explicit ry90(utils::UInt q);
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

class mry90 : public gate {
public:
    cmat_t m;
    explicit mry90(utils::UInt q);
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

class ry180 : public gate {
public:
    cmat_t m;
    explicit ry180(utils::UInt q);
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

class measure : public gate {
public:
    cmat_t m;
    explicit measure(utils::UInt q);
    measure(utils::UInt q, utils::UInt c);
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

class prepz : public gate {
public:
    cmat_t m;
    explicit prepz(utils::UInt q);
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

class cnot : public gate {
public:
    cmat_t m;
    cnot(utils::UInt q1, utils::UInt q2);
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

class cphase : public gate {
public:
    cmat_t m;
    cphase(utils::UInt q1, utils::UInt q2);
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

class toffoli : public gate {
public:
    cmat_t m;
    toffoli(utils::UInt q1, utils::UInt q2, utils::UInt q3);
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

class nop : public gate {
public:
    cmat_t m;
    nop();
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

class swap : public gate {
public:
    cmat_t m;
    swap(utils::UInt q1, utils::UInt q2);
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

/****************************************************************************\
| Special gates
\****************************************************************************/

class wait : public gate {
public:
    cmat_t m;
    utils::UInt duration_in_cycles;

    wait(utils::Vec<utils::UInt> qubits, utils::UInt d, utils::UInt dc);
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

class SOURCE : public gate {
public:
    cmat_t m;
    SOURCE();
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

class SINK : public gate {
public:
    cmat_t m;
    SINK();
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

class display : public gate {
public:
    cmat_t m;
    display();
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

class custom_gate : public gate {
public:
    cmat_t m; // matrix representation
    utils::Str arch_operation_name;  // name of instruction in the architecture (e.g. cc_light_instr)
    explicit custom_gate(const utils::Str &name);
    custom_gate(const custom_gate &g);
    static bool is_qubit_id(const utils::Str &str);
    static utils::UInt qubit_id(const utils::Str &qubit);
    void load(nlohmann::json &instr);
    void print_info() const;
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

class composite_gate : public custom_gate {
public:
    cmat_t m;
    utils::Vec<gate *> gs;
    explicit composite_gate(const utils::Str &name);
    composite_gate(const utils::Str &name, const utils::Vec<gate*> &seq);
    instruction_t qasm() const override;
    GateType type() const override;
    cmat_t mat() const override;
};

} // namespace ql
