/** \file
 * Quantum gate abstraction implementation.
 */

#pragma once

#include "ql/utils/str.h"
#include "ql/utils/vec.h"
#include "ql/utils/json.h"
#include "ql/utils/misc.h"
#include "ql/utils/tree.h"
#include "matrix.h"

namespace ql {
namespace ir {

typedef utils::Str Instruction;

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
    MRX90,
    RX180,
    RY90,
    MRY90,
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

/*
 * additional definitions for describing conditional gates
 */
enum class ConditionType {
    // 0 operands:
    ALWAYS, NEVER,
    // 1 operand:
    UNARY, NOT,
    // 2 operands
    AND, NAND, OR, NOR, XOR, NXOR
};

std::ostream &operator<<(std::ostream &os, ConditionType condition_type);

const utils::UInt MAX_CYCLE = utils::MAX;

struct SwapParamaters {
    utils::Bool part_of_swap = false;
    // at the end of the swap r0 stores v0 and r1 stores v1
    utils::Int r0 = -1;
    utils::Int r1 = -1;
    utils::Int v0 = -1;
    utils::Int v1 = -1;

    // default constructor
    SwapParamaters() {}

    // initializer list
    SwapParamaters(utils::Bool _part_of_swap, utils::Int _r0, utils::Int _r1, utils::Int _v0, utils::Int _v1)
        : part_of_swap(_part_of_swap), r0(_r0), r1(_r1), v0(_v0), v1(_v1)
    {}
};

/**
 * gate interface
 */
class Gate : public utils::Node {
public:
    utils::Str name;
    utils::Vec<utils::UInt> operands;             // qubit operands
    utils::Vec<utils::UInt> creg_operands;
    utils::Vec<utils::UInt> breg_operands;        // bit operands e.g. assigned to by measure; cond_operands are separate
    utils::Vec<utils::UInt> cond_operands;        // 0, 1 or 2 bit operands of condition
    ConditionType condition = ConditionType::ALWAYS; // defines condition and by that number of bit operands of condition
    SwapParamaters swap_params;                  // if the gate is part of a swap/move, this will contain the real and virtual qubits involved
    utils::Int int_operand = 0;                   // FIXME: move to class 'classical'
    utils::UInt duration = 0;
    utils::Real angle = 0.0;                      // for arbitrary rotations
    utils::UInt cycle = MAX_CYCLE;                // cycle after scheduling; MAX_CYCLE indicates undefined
    virtual ~Gate() = default;
    virtual Instruction qasm() const = 0;
    virtual GateType      type() const = 0;
    virtual Complex2by2Matrix        mat()  const = 0;  // to do : change cmat_t type to avoid stack smashing on 2 qubits gate operations
    utils::Str visual_type = ""; // holds the visualization type of this gate that will be linked to a specific configuration in the visualizer
    utils::Bool is_conditional() const;           // whether gate has condition that is NOT cond_always
    Instruction cond_qasm() const;              // returns the condition expression in qasm layout
    static utils::Bool is_valid_cond(ConditionType condition, const utils::Vec<utils::UInt> &cond_operands);
    bool operator==(const Gate &rhs) const;
};

using GateRef = utils::One<Gate>;
using Gates = utils::Any<Gate>;

/****************************************************************************\
| Standard gates
\****************************************************************************/

namespace gates {
namespace matrices {

const utils::Complex IDENTITY[] = {
    1.0, 0.0,
    0.0, 1.0
};

const utils::Complex PAULI_X[] {
    0.0, 1.0,
    1.0, 0.0
};

const utils::Complex PAULI_Y[] {
    0.0, -utils::IM,
    utils::IM, 0.0
};

const utils::Complex PAULI_Z[] {
    1.0, 0.0,
    0.0, -1.0
};

const utils::Complex HADAMARD[] = {
    utils::sqrt(0.5), utils::sqrt(0.5),
    utils::sqrt(0.5), -utils::sqrt(0.5)
};

const utils::Complex PHASE[] {
    1.0, 0.0,
    0.0, utils::IM
};

const utils::Complex PHASE_DAG[] = {
    1.0, 0.0,
    0.0, -utils::IM
};

const utils::Complex T[] = {
    1.0, 0.0,
    0.0, utils::expi(0.25 * utils::PI)
};

const utils::Complex T_DAG[] = {
    1.0, 0.0,
    0.0, utils::expi(-0.25 * utils::PI)
};

const utils::Complex RX90[] = {
    utils::sqrt(0.5), -utils::IM * utils::sqrt(0.5),
    -utils::IM * utils::sqrt(0.5), utils::sqrt(0.5)
};

const utils::Complex RY90[] = {
    utils::sqrt(0.5), -utils::sqrt(0.5),
    utils::sqrt(0.5), utils::sqrt(0.5)
};

const utils::Complex MRX90[] = {
    utils::sqrt(0.5), utils::IM * utils::sqrt(0.5),
    utils::IM * utils::sqrt(0.5), utils::sqrt(0.5)
};

const utils::Complex MRY90[] = {
    utils::sqrt(0.5), utils::sqrt(0.5),
    -utils::sqrt(0.5), utils::sqrt(0.5)
};

const utils::Complex RX180[] = {
    0.0, -utils::IM,
    -utils::IM, 0.0
};

const utils::Complex RY180[] = {
    0.0, -utils::IM,
    utils::IM, 0.0
};

/**
 * to do : multi-qubit gates should not be represented by their matrix (the matrix depends on the ctrl/target qubit locations, the simulation using such matrix is inefficient as well...)
 */

const utils::Complex CNOT[] = {
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 1.0,
    0.0, 0.0, 1.0, 0.0
};

const utils::Complex CPHASE[] = {
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, -1.0
};

const utils::Complex SWAP[] = {
    1.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 1.0
};

const utils::Complex TOFFOLI[] = {
    1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0
};

const utils::Complex NOP[] = {
    1.0, 0.0,
    0.0, 1.0
};

} // namespace matrices

class Identity : public Gate {
public:
    Complex2by2Matrix m;
    explicit Identity(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

class Hadamard : public Gate {
public:
    Complex2by2Matrix m;
    explicit Hadamard(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

class Phase : public Gate {
public:
    Complex2by2Matrix m;
    explicit Phase(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

class PhaseDag : public Gate {
public:
    Complex2by2Matrix m;
    explicit PhaseDag(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

class RX : public Gate {
public:
    Complex2by2Matrix m;
    RX(utils::UInt q, utils::Real theta);
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

class RY : public Gate {
public:
    Complex2by2Matrix m;
    RY(utils::UInt q, utils::Real theta);
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

class RZ : public Gate {
public:
    Complex2by2Matrix m;
    RZ(utils::UInt q, utils::Real theta);
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

class T : public Gate {
public:
    Complex2by2Matrix m;
    explicit T(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

class TDag : public Gate {
public:
    Complex2by2Matrix m;
    explicit TDag(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

class PauliX : public Gate {
public:
    Complex2by2Matrix m;
    explicit PauliX(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

class PauliY : public Gate {
public:
    Complex2by2Matrix m;
    explicit PauliY(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

class PauliZ : public Gate {
public:
    Complex2by2Matrix m;
    explicit PauliZ(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

class RX90 : public Gate {
public:
    Complex2by2Matrix m;
    explicit RX90(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

class MRX90 : public Gate {
public:
    Complex2by2Matrix m;
    explicit MRX90(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

class RX180 : public Gate {
public:
    Complex2by2Matrix m;
    explicit RX180(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

class RY90 : public Gate {
public:
    Complex2by2Matrix m;
    explicit RY90(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

class MRY90 : public Gate {
public:
    Complex2by2Matrix m;
    explicit MRY90(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

class RY180 : public Gate {
public:
    Complex2by2Matrix m;
    explicit RY180(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

class Measure : public Gate {
public:
    Complex2by2Matrix m;
    explicit Measure(utils::UInt q);
    Measure(utils::UInt q, utils::UInt c);
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

class PrepZ : public Gate {
public:
    Complex2by2Matrix m;
    explicit PrepZ(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

class CNot : public Gate {
public:
    Complex2by2Matrix m;
    CNot(utils::UInt q1, utils::UInt q2);
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

class CPhase : public Gate {
public:
    Complex2by2Matrix m;
    CPhase(utils::UInt q1, utils::UInt q2);
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

class Toffoli : public Gate {
public:
    Complex2by2Matrix m;
    Toffoli(utils::UInt q1, utils::UInt q2, utils::UInt q3);
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

class Nop : public Gate {
public:
    Complex2by2Matrix m;
    Nop();
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

class Swap : public Gate {
public:
    Complex2by2Matrix m;
    Swap(utils::UInt q1, utils::UInt q2);
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

/****************************************************************************\
| Special gates
\****************************************************************************/

class Wait : public Gate {
public:
    Complex2by2Matrix m;
    utils::UInt duration_in_cycles;

    Wait(utils::Vec<utils::UInt> qubits, utils::UInt d, utils::UInt dc);
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

class Source : public Gate {
public:
    Complex2by2Matrix m;
    Source();
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

class Sink : public Gate {
public:
    Complex2by2Matrix m;
    Sink();
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

class Display : public Gate {
public:
    Complex2by2Matrix m;
    Display();
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

class Custom : public Gate {
public:
    Complex2by2Matrix m; // matrix representation
    utils::Str arch_operation_name;  // name of instruction in the architecture (e.g. cc_light_instr)
    explicit Custom(const utils::Str &name);
    Custom(const Custom &g);
    static bool is_qubit_id(const utils::Str &str);
    static utils::UInt qubit_id(const utils::Str &qubit);
    void load(nlohmann::json &instr);
    void print_info() const;
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

class Composite : public Custom {
public:
    Complex2by2Matrix m;
    Gates gs;
    explicit Composite(const utils::Str &name);
    Composite(const utils::Str &name, const Gates &seq);
    Instruction qasm() const override;
    GateType type() const override;
    Complex2by2Matrix mat() const override;
};

} // namespace gates
} // namespace ir
} // namespace ql
