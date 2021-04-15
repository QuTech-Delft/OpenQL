/** \file
 * Classical operation implementation.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/vec.h"
#include "ql/utils/tree.h"

#include "ql/ir/gate.h"

namespace ql {
namespace ir {

enum class ClassicalOperationType {
    ARITHMATIC, RELATIONAL, BITWISE
};

enum class ClassicalOperandType {
    REGISTER, VALUE
};

class ClassicalValue;
class ClassicalRegister;

class ClassicalOperand : public utils::Node {
public:
    virtual ClassicalOperandType type() const = 0;
    virtual void print() const = 0;
    ClassicalValue &as_value();
    const ClassicalValue &as_value() const;
    ClassicalRegister &as_register();
    const ClassicalRegister &as_register() const;
};

class ClassicalValue : public ClassicalOperand {
public:
    utils::Int value;
    ClassicalValue(utils::Int val);
    ClassicalValue(const ClassicalValue &cv);
    ClassicalOperandType type() const override;
    void print() const override;
};

class ClassicalRegister : public ClassicalOperand {
public:
    utils::UInt id;
    ClassicalRegister(utils::UInt id);
    ClassicalRegister(const ClassicalRegister &c);
    ClassicalOperandType type() const override;
    void print() const override;
};

class ClassicalOperation : public utils::Node {
public:
    utils::Str operation_name;
    utils::Str inv_operation_name;
    ClassicalOperationType operation_type;
    utils::Any<ClassicalOperand> operands;

    ClassicalOperation(const ClassicalRegister &l, const utils::Str &op, const ClassicalRegister &r);

    // used for assign
    ClassicalOperation(const ClassicalRegister &l);

    // used for initializing with an imm
    ClassicalOperation(const ClassicalValue &v);

    // used for initializing with an imm
    ClassicalOperation(utils::Int val);

    ClassicalOperation(const utils::Str &op, const ClassicalRegister &r);
};

namespace gates {

class Classical : public Gate {
public:
    Classical(const ClassicalRegister &dest, const ClassicalOperation &oper);
    Classical(const utils::Str &operation);
    Instruction qasm() const override;
    GateType type() const override;
};

} // namespace gates
} // namespace ir
} // namespace ql
