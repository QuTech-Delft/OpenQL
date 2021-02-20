/** \file
 * Classical operation implementation.
 */

#include "classical.h"

#include "utils/exception.h"

namespace ql {

using namespace utils;

cval &coperand::as_cval() {
    try {
        return dynamic_cast<cval &>(*this);
    } catch (std::bad_cast &e) {
        throw Exception("coperand is not a cval");
    }
}

const cval &coperand::as_cval() const {
    try {
        return dynamic_cast<const cval &>(*this);
    } catch (std::bad_cast &e) {
        throw Exception("coperand is not a cval");
    }
}

creg &coperand::as_creg() {
    try {
        return dynamic_cast<creg &>(*this);
    } catch (std::bad_cast &e) {
        throw Exception("coperand is not a creg");
    }
}

const creg &coperand::as_creg() const {
    try {
        return dynamic_cast<const creg &>(*this);
    } catch (std::bad_cast &e) {
        throw Exception("coperand is not a creg");
    }
}

cval::cval(Int val) : value(val) {
}

cval::cval(const cval &cv) : value(cv.value) {
}

ql::operand_type_t cval::type() const {
    return operand_type_t::CVAL;
}

void cval::print() const {
    QL_COUT("cval with value: " << value);
}

creg::creg(UInt id) : id(id) {
    QL_DOUT("creg constructor, used id: " << id);
}

creg::creg(const creg &c) : id(c.id) {
    QL_DOUT("creg copy constructor, used id: " << id);
}

ql::operand_type_t creg::type() const {
    return operand_type_t::CREG;
}

void creg::print() const {
    QL_COUT("creg with id: " << id);
}

operation::operation(const creg &l, const Str &op, const creg &r) {
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
        QL_EOUT("Unknown binary operation '" << op);
        throw Exception("Unknown binary operation '" + op + "' !", false);
    }
}

// used for assign
operation::operation(const creg &l) {
    operation_name = "mov";
    operation_type = ql::operation_type_t::ARITHMATIC;
    operands.push_back(new ql::creg(l));
}

// used for initializing with an imm
operation::operation(const cval &v) {
    operation_name = "ldi";
    operation_type = ql::operation_type_t::ARITHMATIC;
    operands.push_back(new ql::cval(v));
}

// used for initializing with an imm
operation::operation(Int val) {
    operation_name = "ldi";
    operation_type = ql::operation_type_t::ARITHMATIC;
    operands.push_back(new ql::cval(val));
}

operation::operation(const Str &op, const creg &r) {
    if (op == "~") {
        operation_name = "not";
        operation_type = ql::operation_type_t::BITWISE;
        operands.push_back(new ql::creg(r));
    } else {
        QL_EOUT("Unknown unary operation '" << op);
        throw Exception("Unknown unary operation '" + op + "' !", false);
    }
}

classical::classical(const creg &dest, const operation &oper) {
    QL_DOUT("Classical gate constructor with destination for "
             << oper.operation_name);
    name = oper.operation_name;
    duration = 20;
    creg_operands.push_back(dest.id);
    if (name == "ldi") {
        int_operand = oper.operands[0]->as_cval().value;
        QL_DOUT("... setting int_operand of " << oper.operation_name << " to "
                                              << int_operand);
    } else {
        for (auto &op : oper.operands) {
            creg_operands.push_back(op->as_creg().id);
        }
    }
}

classical::classical(const Str &operation) {
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

instruction_t classical::qasm() const {
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

gate_type_t classical::type() const {
    return __classical_gate__;
}

cmat_t classical::mat() const {
    return m;
}

};
