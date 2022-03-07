/** \file
 * handle operands within the new IR. Based on new_to_old.cc
 * FIXME: could be useful for other backends and should be moved if this becomes appropriate
 */

#pragma once

#include "ql/ir/ir.h"

#include "types.h"

/**
 * Helper macro for QL_ICE() that throws when the given condition is not
 * true. From new_to_old.cc
 */
#define CHECK_COMPAT(val, s) do { if (!(val)) QL_ICE(s); } while (false)


namespace ql {
namespace arch {
namespace cc {
namespace pass {
namespace gen {
namespace vq1asm {
namespace detail {

class OperandContext {  // NB: based on class NewToOldConverter
public:
    explicit OperandContext(const ir::Ref &ir);
    Bool is_creg_reference(const ir::ExpressionRef &ref) const;
    Int convert_creg_reference(const ir::Reference &ref) const;
    Int convert_breg_reference(const ir::ExpressionRef &ref) const;

private:
    friend class Operands;

    /**
     * The root of the new IR structure that serves as our input.
     */
    ir::Ref ir;

    /**
     * The number of qubits.
     */
    utils::UInt num_qubits;

    /**
     * The object used by the new IR to refer to bregs from num_qubits onwards.
     */
    ir::ObjectLink breg_ob;

    /**
     * The object used by the new IR to refer to cregs.
     */
    ir::ObjectLink creg_ob;
};


// FIXME: shameless copy of new_to_old.cc::Operands, edited to suit

/**
 * Handles gathering the operands for a gate.
 */
class Operands {
public:

    /**
     * Qubit operand indices.
     */
    utils::Vec<utils::UInt> qubits;

    /**
     * Creg operand indices.
     */
    utils::Vec<utils::UInt> cregs;

    /**
     * Breg operand indices.
     */
    utils::Vec<utils::UInt> bregs;

    /**
     * Angle operand existence.
     */
    utils::Bool has_angle = false;

    /**
     * Angle operand value.
     */
    utils::Real angle = 0.0;

    /**
     * Integer operand existence.
     */
    utils::Bool has_integer = false;

    /**
     * Integer operand value.
     */
    utils::Int integer = 0;

    /**
     * Appends an operand.
     */
    void append(const OperandContext &operandContext, const ir::ExpressionRef &expr);

};

} // namespace detail
} // namespace vq1asm
} // namespace gen
} // namespace pass
} // namespace cc
} // namespace arch
} // namespace ql
