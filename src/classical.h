/** \file
 * Classical operation implementation.
 */

#pragma once

#include "utils/num.h"
#include "utils/str.h"
#include "utils/vec.h"

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
    virtual operand_type_t type() const = 0;
    virtual void print() const = 0;
    virtual ~coperand() = default;
    cval &as_cval();
    const cval &as_cval() const;
    creg &as_creg();
    const creg &as_creg() const;
};

class cval : public coperand {
public:
    utils::Int value;
    cval(utils::Int val);
    cval(const cval &cv);
    operand_type_t type() const override;
    void print() const override;
};

class creg : public coperand {
public:
    utils::UInt id;
    creg(utils::UInt id);
    creg(const creg &c);
    operand_type_t type() const override;
    void print() const override;
};

class operation {
public:
    utils::Str operation_name;
    utils::Str inv_operation_name;
    operation_type_t operation_type;
    utils::Vec<coperand*> operands;

    operation(const creg &l, const utils::Str &op, const creg &r);

    // used for assign
    operation(const creg &l);

    // used for initializing with an imm
    operation(const cval &v);

    // used for initializing with an imm
    operation(utils::Int val);

    operation(const utils::Str &op, const creg &r);
};

class classical : public gate {
public:
    // utils::Int imm_value;
    cmat_t m;

    classical(const creg &dest, const operation &oper);
    classical(const utils::Str &operation);
    instruction_t qasm() const override;
    gate_type_t type() const override;
    cmat_t mat() const override;
};

} // namespace ql
