#include "classical.h"

#include "utils.h"
#include "str.h"
#include "exception.h"

namespace ql {

cval::cval(int val) {
    value = val;
}

cval::cval(const cval &cv) {
    value = cv.value;
}

ql::operand_type_t cval::type() const {
    return operand_type_t::CVAL;
}

void cval::print() const {
    COUT("cval with value: " << value);
}

creg::creg(size_t id) {
    this->id = id;
    DOUT("creg constructor, used id: " << id);
}

creg::creg(const creg &c) {
    id = c.id;
    DOUT("creg copy constructor, used id: " << id);
}

ql::operand_type_t creg::type() const {
    return operand_type_t::CREG;
}

void creg::print() const {
    COUT("creg with id: " << id);
}

operation::operation(creg& l, std::string op, creg& r) {
    operands.push_back(new ql::creg(l));
    operands.push_back(new ql::creg(r));
    if (op == "+") {
        operation_name = "add";
        operation_type = ql::operation_type_t::ARITHMATIC;
    } else if (op == "-") {
        operation_name = "sub";
        operation_type = ql::operation_type_t::ARITHMATIC;
    } else if (op == "&") {
        operation_name = "and";
        operation_type = ql::operation_type_t::BITWISE;
    } else if (op == "|") {
        operation_name = "or";
        operation_type = ql::operation_type_t::BITWISE;
    } else if (op == "^") {
        operation_name = "xor";
        operation_type = ql::operation_type_t::BITWISE;
    } else if (op == "==") {
        operation_name = "eq";
        inv_operation_name = "ne";
        operation_type = ql::operation_type_t::RELATIONAL;
    } else if (op == "!=") {
        operation_name = "ne";
        inv_operation_name = "eq";
        operation_type = ql::operation_type_t::RELATIONAL;
    } else if (op == "<") {
        operation_name = "lt";
        inv_operation_name = "ge";
        operation_type = ql::operation_type_t::RELATIONAL;
    } else if (op == ">") {
        operation_name = "gt";
        inv_operation_name = "le";
        operation_type = ql::operation_type_t::RELATIONAL;
    } else if (op == "<=") {
        operation_name = "le";
        inv_operation_name = "gt";
        operation_type = ql::operation_type_t::RELATIONAL;
    } else if (op == ">=") {
        operation_name = "ge";
        inv_operation_name = "lt";
        operation_type = ql::operation_type_t::RELATIONAL;
    } else {
        EOUT("Unknown binary operation '" << op);
        throw ql::exception("Unknown binary operation '" + op + "' !", false);
    }
}

// used for assign
operation::operation(creg &l) {
    operation_name = "mov";
    operation_type = ql::operation_type_t::ARITHMATIC;
    operands.push_back(new ql::creg(l));
}

// used for initializing with an imm
operation::operation(cval &v) {
    operation_name = "ldi";
    operation_type = ql::operation_type_t::ARITHMATIC;
    operands.push_back(new ql::cval(v));
}

// used for initializing with an imm
operation::operation(int val) {
    operation_name = "ldi";
    operation_type = ql::operation_type_t::ARITHMATIC;
    operands.push_back(new ql::cval(val));
}

operation::operation(std::string op, creg &r) {
    if (op == "~") {
        operation_name = "not";
        operation_type = ql::operation_type_t::BITWISE;
        operands.push_back(new ql::creg(r));
    } else {
        EOUT("Unknown unary operation '" << op);
        throw ql::exception("Unknown unary operation '" + op + "' !", false);
    }
}

classical::classical(creg &dest, operation &oper) {
    DOUT("Classical gate constructor with destination for "
             << oper.operation_name);
    name = oper.operation_name;
    duration = 20;
    creg_operands.push_back(dest.id);
    if (name == "ldi") {
        int_operand = (oper.operands[0])->value;
        DOUT("... setting int_operand of " << oper.operation_name << " to "
                                           << int_operand);
    } else {
        for (auto &op : oper.operands) {
            creg_operands.push_back(op->id);
        }
    }
}

classical::classical(const std::string &operation) {
    DOUT("Classical gate constructor for " << operation);
    auto operation_lower = operation;
    str::lower_case(operation_lower);
    if ((operation_lower == "nop")) {
        name = operation_lower;
        duration = 20;
        DOUT("Adding 0 operand operation: " << name);
    } else {
        EOUT("Unknown classical operation '" << name << "' with '0' operands!");
        throw ql::exception(
            "Unknown classical operation'" + name + "' with'0' operands!",
            false);
    }
}

instruction_t classical::qasm() const {
    std::string iopers;
    int sz = creg_operands.size();
    for (int i = 0; i < sz; ++i) {
        if (i == sz - 1) {
            iopers += " r" + std::to_string(creg_operands[i]);
        } else {
            iopers += " r" + std::to_string(creg_operands[i]) + ",";
        }
    }

    if (name == "ldi") {
        return "ldi" + iopers + ", " + std::to_string(int_operand);
    } else {
        return name + iopers;
    }
}

gate_type_t classical::type() const {
    return __classical_gate__;
}

cmat_t classical::mat() const {
    return m;
}

};
