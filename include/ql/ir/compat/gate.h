/** \file
 * Quantum gate abstraction implementation.
 */

#pragma once

#include "ql/utils/str.h"
#include "ql/utils/vec.h"
#include "ql/utils/json.h"
#include "ql/utils/misc.h"
#include "ql/utils/tree.h"

namespace ql {
namespace ir {
namespace compat {

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

struct SwapParameters {
    utils::Bool part_of_swap = false;
    // at the end of the swap r0 stores v0 and r1 stores v1
    utils::Int r0 = -1;
    utils::Int r1 = -1;
    utils::Int v0 = -1;
    utils::Int v1 = -1;

    // default constructor
    SwapParameters() {}

    // initializer list
    SwapParameters(utils::Bool _part_of_swap, utils::Int _r0, utils::Int _r1, utils::Int _v0, utils::Int _v1)
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
    SwapParameters swap_params;                  // if the gate is part of a swap/move, this will contain the real and virtual qubits involved
    utils::Int int_operand = 0;                   // FIXME: move to class 'classical'
    utils::UInt duration = 0;
    utils::Real angle = 0.0;                      // for arbitrary rotations
    utils::UInt cycle = MAX_CYCLE;                // cycle after scheduling; MAX_CYCLE indicates undefined
    virtual ~Gate() = default;
    virtual Instruction qasm() const = 0;
    virtual GateType      type() const = 0;
    utils::Bool is_conditional() const;           // whether gate has condition that is NOT cond_always
    Instruction cond_qasm() const;              // returns the condition expression in qasm layout
    static utils::Bool is_valid_cond(ConditionType condition, const utils::Vec<utils::UInt> &cond_operands);
};

using GateRef = utils::One<Gate>;
using GateRefs = utils::Any<Gate>;

/****************************************************************************\
| Standard gates
\****************************************************************************/

namespace gate_types {

class Identity : public Gate {
public:
    explicit Identity(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
};

class Hadamard : public Gate {
public:
    explicit Hadamard(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
};

class Phase : public Gate {
public:
    explicit Phase(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
};

class PhaseDag : public Gate {
public:
    explicit PhaseDag(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
};

class RX : public Gate {
public:
    RX(utils::UInt q, utils::Real theta);
    Instruction qasm() const override;
    GateType type() const override;
};

class RY : public Gate {
public:
    RY(utils::UInt q, utils::Real theta);
    Instruction qasm() const override;
    GateType type() const override;
};

class RZ : public Gate {
public:
    RZ(utils::UInt q, utils::Real theta);
    Instruction qasm() const override;
    GateType type() const override;
};

class T : public Gate {
public:
    explicit T(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
};

class TDag : public Gate {
public:
    explicit TDag(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
};

class PauliX : public Gate {
public:
    explicit PauliX(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
};

class PauliY : public Gate {
public:
    explicit PauliY(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
};

class PauliZ : public Gate {
public:
    explicit PauliZ(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
};

class RX90 : public Gate {
public:
    explicit RX90(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
};

class MRX90 : public Gate {
public:
    explicit MRX90(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
};

class RX180 : public Gate {
public:
    explicit RX180(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
};

class RY90 : public Gate {
public:
    explicit RY90(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
};

class MRY90 : public Gate {
public:
    explicit MRY90(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
};

class RY180 : public Gate {
public:
    explicit RY180(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
};

class Measure : public Gate {
public:
    explicit Measure(utils::UInt q);
    Measure(utils::UInt q, utils::UInt c);
    Instruction qasm() const override;
    GateType type() const override;
};

class PrepZ : public Gate {
public:
    explicit PrepZ(utils::UInt q);
    Instruction qasm() const override;
    GateType type() const override;
};

class CNot : public Gate {
public:
    CNot(utils::UInt q1, utils::UInt q2);
    Instruction qasm() const override;
    GateType type() const override;
};

class CPhase : public Gate {
public:
    CPhase(utils::UInt q1, utils::UInt q2);
    Instruction qasm() const override;
    GateType type() const override;
};

class Toffoli : public Gate {
public:
    Toffoli(utils::UInt q1, utils::UInt q2, utils::UInt q3);
    Instruction qasm() const override;
    GateType type() const override;
};

class Nop : public Gate {
public:
    Nop();
    Instruction qasm() const override;
    GateType type() const override;
};

class Swap : public Gate {
public:
    Swap(utils::UInt q1, utils::UInt q2);
    Instruction qasm() const override;
    GateType type() const override;
};

/****************************************************************************\
| Special gates
\****************************************************************************/

class Wait : public Gate {
public:
    utils::UInt duration_in_cycles;

    Wait(utils::Vec<utils::UInt> qubits, utils::UInt d, utils::UInt dc);
    Instruction qasm() const override;
    GateType type() const override;
};

class Source : public Gate {
public:
    Source();
    Instruction qasm() const override;
    GateType type() const override;
};

class Sink : public Gate {
public:
    Sink();
    Instruction qasm() const override;
    GateType type() const override;
};

class Display : public Gate {
public:
    Display();
    Instruction qasm() const override;
    GateType type() const override;
};

class Custom : public Gate {
public:
    explicit Custom(const utils::Str &name);
    void load(const utils::Json &instr, utils::UInt num_qubits, utils::UInt cycle_time);
    void print_info() const;
    Instruction qasm() const override;
    GateType type() const override;
};

class Composite : public Custom {
public:
    GateRefs gs;
    explicit Composite(const utils::Str &name);
    Composite(const utils::Str &name, const GateRefs &seq);
    Instruction qasm() const override;
    GateType type() const override;
};

} // namespace gates
} // namespace compat
} // namespace ir
} // namespace ql
