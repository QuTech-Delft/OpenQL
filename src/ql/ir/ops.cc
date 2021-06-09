/** \file
 * Defines basic access operations on the IR.
 */

#include "ql/ir/ops.h"

namespace ql {
namespace ir {

/**
 * Returns the data type of/returned by an expression.
 */
DataTypeRef get_type_of(const ExpressionRef &expr) {
    if (auto lit = expr->as_literal()) {
        return lit->data_type;
    } else if (auto ref = expr->as_reference()) {
        return ref->target->data_type;
    } else if (auto cast = expr->as_typecast()) {
        return cast->data_type;
    } else if (auto fnc = expr->as_function_call()) {
        return fnc->function_type->return_type;
    } else {
        throw utils::Exception("unknown expression node type encountered");
    }
}

/**
 * Returns whether the given expression can be assigned or is a qubit (i.e.,
 * whether it can appear on the left-hand side of an assignment, or can be used
 * as an operand in classical write or qubit access mode).
 */
utils::Bool is_assignable_or_qubit(const ExpressionRef &expr) {
    if (expr->as_literal()) {
        return false;
    } else if (expr->as_reference()) {
        return true;
    } else if (auto cast = expr->as_typecast()) {
        return is_assignable_or_qubit(cast->expression);
    } else if (expr->as_function_call()) {
        return false;
    } else {
        throw utils::Exception("unknown expression node type encountered");
    }
}

/**
 * Returns the duration of an instruction in quantum cycles. Note that this will
 * be zero for non-quantum instructions.
 */
utils::UInt get_duration_of(const InstructionRef &insn) {
    if (auto custom = insn->as_custom_instruction()) {
        return custom->instruction_type->duration;
    } else if (auto wait = insn->as_wait_instruction()) {
        return wait->duration;
    } else {
        return 0;
    }
}

/**
 * Returns the maximum value that an integer of the given type may have.
 */
utils::Int get_max_int_for(const IntType &ityp) {
    auto bits = ityp.bits;
    if (ityp.is_signed) bits--;
    return (utils::Int)((1ull << bits) - 1);
}

/**
 * Returns the minimum value that an integer of the given type may have.
 */
utils::Int get_min_int_for(const IntType &ityp) {
    if (!ityp.is_signed) return 0;
    return (utils::Int)(-(1ull << (ityp.bits - 1)));
}

} // namespace ir
} // namespace ql
