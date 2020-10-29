/**
 * @file   classical.h
 * @date   05/2018
 * @author Imran Ashraf
 * @brief  classical operation implementation
 */

#pragma once

#include <string>
#include <vector>

#include "gate.h"

namespace ql {

enum class operation_type_t {
    ARITHMATIC, RELATIONAL, BITWISE
};

enum class operand_type_t {
    CREG, CVAL
};

class cval;
class creg;

class coperand {
public:
    virtual ql::operand_type_t type() const = 0;
    virtual void print() const = 0;
    virtual ~coperand() = default;
    cval &as_cval();
    const cval &as_cval() const;
    creg &as_creg();
    const creg &as_creg() const;
};

class cval : public coperand {
public:
    int value;
    cval(int val);
    cval(const cval &cv);
    ql::operand_type_t type() const override;
    void print() const override;
};

class creg : public coperand {
public:
    size_t id;
    creg(size_t id);
    creg(const creg &c);
    ql::operand_type_t type() const override;
    void print() const override;
};

class operation {
public:
    std::string operation_name;
    std::string inv_operation_name;
    operation_type_t operation_type;
    std::vector<coperand*> operands;

    operation(const creg &l, const std::string &op, const creg &r);

    // used for assign
    operation(const creg &l);

    // used for initializing with an imm
    operation(const cval &v);

    // used for initializing with an imm
    operation(int val);

    operation(const std::string &op, const creg &r);
};

class classical : public gate {
public:
    // int imm_value;
    cmat_t m;

    classical(const creg &dest, const operation &oper);
    classical(const std::string &operation);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

} // namespace ql
