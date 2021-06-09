/** \file
 * Defines basic access operations on the IR.
 */

#pragma once

#include "ql/ir/ir.h"

namespace ql {
namespace ir {

/**
 * Returns the data type of/returned by an expression.
 */
DataTypeRef get_type_of(const ExpressionRef &expr);

/**
 * Returns whether the given expression can be assigned or is a qubit (i.e.,
 * whether it can appear on the left-hand side of an assignment, or can be used
 * as an operand in classical write or qubit access mode).
 */
utils::Bool is_assignable_or_qubit(const ExpressionRef &expr);

/**
 * Returns the duration of an instruction in quantum cycles. Note that this will
 * be zero for non-quantum instructions.
 */
utils::UInt get_duration_of(const InstructionRef &insn);

/**
 * Returns the maximum value that an integer of the given type may have.
 */
utils::Int get_max_int_for(const IntType &ityp);

/**
 * Returns the minimum value that an integer of the given type may have.
 */
utils::Int get_min_int_for(const IntType &ityp);

} // namespace ir
} // namespace ql
