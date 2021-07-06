/** \file
 * Defines static information about operator types and names, such as their
 * associativity and precedence level.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/pair.h"
#include "ql/utils/map.h"

namespace ql {
namespace ir {

/**
 * The associativity of an operator.
 */
enum class OperatorAssociativity {

    /**
     * Left-associative, i.e. a # b # c === (a # b) # c.
     */
    LEFT,

    /**
     * Right-associative, i.e. a # b # c === a # (b # c).
     */
    RIGHT

};

/**
 * Operator information structure.
 */
struct OperatorInfo {

    /**
     * The precedence level for the operator. If the precedence of operator #
     * is higher than the precedence of operator %, a # b % c === (a # b) % c
     * and a % b # c === a % (b # c), regardless of the associativity of either.
     */
    utils::UInt precedence;

    /**
     * The associativity of the operator. Indicates whether a # b # c is
     * identical to (a # b) # c (= left) or to a # (b # c) (= right).
     */
    OperatorAssociativity associativity;

    /**
     * String to prefix before the operands.
     */
    const char *prefix;

    /**
     * String to insert between the first and second operand.
     */
    const char *infix;

    /**
     * String to insert between the second and third operand.
     */
    const char *infix2;

};

/**
 * Metadata for operators as they appear in cQASM (or just logically in
 * general). Used to avoid excessive parentheses when printing expressions.
 * The first element in the key pair is the function name, the second is the
 * number of operands.
 */
extern const utils::Map<utils::Pair<utils::Str, utils::UInt>, OperatorInfo> OPERATOR_INFO;

} // namespace ir
} // namespace ql
