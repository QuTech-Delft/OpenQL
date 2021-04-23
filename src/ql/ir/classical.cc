/** \file
 * Classical operation implementation.
 */

#include "ql/ir/classical.h"

#include "ql/utils/exception.h"

namespace ql {
namespace ir {

using namespace utils;

ClassicalValue &ClassicalOperand::as_value() {
    try {
        return dynamic_cast<ClassicalValue &>(*this);
    } catch (std::bad_cast &e) {
        throw Exception("coperand is not a cval");
    }
}

const ClassicalValue &ClassicalOperand::as_value() const {
    try {
        return dynamic_cast<const ClassicalValue &>(*this);
    } catch (std::bad_cast &e) {
        throw Exception("coperand is not a cval");
    }
}

ClassicalRegister &ClassicalOperand::as_register() {
    try {
        return dynamic_cast<ClassicalRegister &>(*this);
    } catch (std::bad_cast &e) {
        throw Exception("coperand is not a creg");
    }
}

const ClassicalRegister &ClassicalOperand::as_register() const {
    try {
        return dynamic_cast<const ClassicalRegister &>(*this);
    } catch (std::bad_cast &e) {
        throw Exception("coperand is not a creg");
    }
}

ClassicalValue::ClassicalValue(Int val) : value(val) {
}

ClassicalValue::ClassicalValue(const ClassicalValue &cv) : value(cv.value) {
}

ClassicalOperandType ClassicalValue::type() const {
    return ClassicalOperandType::VALUE;
}

void ClassicalValue::print() const {
    QL_COUT("cval with value: " << value);
}

ClassicalRegister::ClassicalRegister(UInt id) : id(id) {
    QL_DOUT("creg constructor, used id: " << id);
}

ClassicalRegister::ClassicalRegister(const ClassicalRegister &c) : id(c.id) {
    QL_DOUT("creg copy constructor, used id: " << id);
}

ClassicalOperandType ClassicalRegister::type() const {
    return ClassicalOperandType::REGISTER;
}

void ClassicalRegister::print() const {
    QL_COUT("creg with id: " << id);
}

ClassicalOperation::ClassicalOperation(
    const ClassicalRegister &l,
    const Str &op,
    const ClassicalRegister &r
) {
    operands.emplace<ClassicalRegister>(l);
    operands.emplace<ClassicalRegister>(r);
    if (op == "+") {
        operation_name = "add";
        operation_type = ClassicalOperationType::ARITHMATIC;
    } else if (op == "-") {
        operation_name = "sub";
        operation_type = ClassicalOperationType::ARITHMATIC;
    } else if (op == "&") {
        operation_name = "and";
        operation_type = ClassicalOperationType::BITWISE;
    } else if (op == "|") {
        operation_name = "or";
        operation_type = ClassicalOperationType::BITWISE;
    } else if (op == "^") {
        operation_name = "xor";
        operation_type = ClassicalOperationType::BITWISE;
    } else if (op == "==") {
        operation_name = "eq";
        inv_operation_name = "ne";
        operation_type = ClassicalOperationType::RELATIONAL;
    } else if (op == "!=") {
        operation_name = "ne";
        inv_operation_name = "eq";
        operation_type = ClassicalOperationType::RELATIONAL;
    } else if (op == "<") {
        operation_name = "lt";
        inv_operation_name = "ge";
        operation_type = ClassicalOperationType::RELATIONAL;
    } else if (op == ">") {
        operation_name = "gt";
        inv_operation_name = "le";
        operation_type = ClassicalOperationType::RELATIONAL;
    } else if (op == "<=") {
        operation_name = "le";
        inv_operation_name = "gt";
        operation_type = ClassicalOperationType::RELATIONAL;
    } else if (op == ">=") {
        operation_name = "ge";
        inv_operation_name = "lt";
        operation_type = ClassicalOperationType::RELATIONAL;
    } else {
        QL_EOUT("Unknown binary operation '" << op);
        throw Exception("Unknown binary operation '" + op + "' !", false);
    }
}

// used for assign
ClassicalOperation::ClassicalOperation(const ClassicalRegister &l) {
    operation_name = "mov";
    operation_type = ClassicalOperationType::ARITHMATIC;
    operands.emplace<ClassicalRegister>(l);
}

// used for initializing with an imm
ClassicalOperation::ClassicalOperation(const ClassicalValue &v) {
    operation_name = "ldi";
    operation_type = ClassicalOperationType::ARITHMATIC;
    operands.emplace<ClassicalValue>(v);
}

// used for initializing with an imm
ClassicalOperation::ClassicalOperation(Int val) {
    operation_name = "ldi";
    operation_type = ClassicalOperationType::ARITHMATIC;
    operands.emplace<ClassicalValue>(val);
}

ClassicalOperation::ClassicalOperation(const Str &op, const ClassicalRegister &r) {
    if (op == "~") {
        operation_name = "not";
        operation_type = ClassicalOperationType::BITWISE;
        operands.emplace<ClassicalRegister>(r);
    } else {
        QL_EOUT("Unknown unary operation '" << op);
        throw Exception("Unknown unary operation '" + op + "' !", false);
    }
}

namespace gate_types {

Classical::Classical(const ClassicalRegister &dest, const ClassicalOperation &oper) {
    QL_DOUT("Classical gate constructor with destination for "
             << oper.operation_name);
    name = oper.operation_name;
    duration = 20;
    creg_operands.push_back(dest.id);
    if (name == "ldi") {
        int_operand = oper.operands[0]->as_value().value;
        QL_DOUT("... setting int_operand of " << oper.operation_name << " to "
                                              << int_operand);
    } else {
        for (auto &op : oper.operands) {
            creg_operands.push_back(op->as_register().id);
        }
    }
}

Classical::Classical(const Str &operation) {
    QL_DOUT("Classical gate constructor for " << operation);
    auto operation_lower = to_lower(operation);
    if ((operation_lower == "nop")) {
        name = operation_lower;
        duration = 20;
        QL_DOUT("Adding 0 operand operation: " << name);
    } else {
        QL_EOUT("Unknown classical operation '" << name << "' with '0' operands!");
        throw Exception(
            "Unknown classical operation'" + name + "' with'0' operands!",
            false);
    }
}

Instruction Classical::qasm() const {
    Str iopers;
    Int sz = creg_operands.size();
    for (Int i = 0; i < sz; ++i) {
        if (i == sz - 1) {
            iopers += " r" + to_string(creg_operands[i]);
        } else {
            iopers += " r" + to_string(creg_operands[i]) + ",";
        }
    }

    if (name == "ldi") {
        return "ldi" + iopers + ", " + to_string(int_operand);
    } else {
        return name + iopers;
    }
}

GateType Classical::type() const {
    return GateType::CLASSICAL;
}

} // namespace gates
} // namespace ir
} // namespace ql
