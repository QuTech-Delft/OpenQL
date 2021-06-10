/** \file
 * Defines basic access operations on the IR.
 */

#pragma once

#include "ql/utils/map.h"
#include "ql/ir/ir.h"

namespace ql {
namespace ir {

/**
 * Returns the data type of/returned by an expression.
 */
DataTypeLink get_type_of(const ExpressionRef &expr);

/**
 * Returns the maximum value that an integer of the given type may have.
 */
utils::Int get_max_int_for(const IntType &ityp);

/**
 * Returns the minimum value that an integer of the given type may have.
 */
utils::Int get_min_int_for(const IntType &ityp);

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
 * Container for gathering and representing data dependencies of instructions
 * and expressions.
 */
class DataDependencies {
public:
    using Deps = utils::Map<ObjectLink, prim::AccessMode>;

private:

    /**
     * The actual dependency list.
     */
    Deps deps;

public:

    /**
     * Returns the contained dependency list.
     */
    const Deps &get() const;

    /**
     * Adds a single dependency on an object. Literal access mode is upgraded to
     * read mode, as it makes no sense to access an object in literal mode (this
     * should never happen for consistent IRs though, unless this is explicitly
     * called this way). If there was already a dependency for the object, the
     * access mode is combined: if they match the mode is maintained, otherwise
     * the mode is changed to write.
     */
    void add_object(prim::AccessMode mode, const ObjectLink &obj);

    /**
     * Adds dependencies on whatever is used by a complete expression.
     */
    void add_expression(prim::AccessMode mode, const ExpressionRef &expr);

    /**
     * Adds dependencies on the operands of a function or instruction.
     */
    void add_operands(const utils::Any<OperandType> &prototype, const utils::Any<Expression> &operands);

    /**
     * Adds dependencies for a complete statement.
     */
    void add_statement(const StatementRef &stmt);

    /**
     * Adds dependencies for a whole (sub)block of statements.
     */
    void add_block(const SubBlockRef &block);

    /**
     * Clears the dependency list, allowing the object to be reused.
     */
    void reset();

};

} // namespace ir
} // namespace ql
