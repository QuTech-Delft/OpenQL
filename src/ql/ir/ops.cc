/** \file
 * Defines basic access operations on the IR.
 */

#include "ql/ir/ops.h"

namespace ql {
namespace ir {

/**
 * Returns the data type of/returned by an expression.
 */
DataTypeLink get_type_of(const ExpressionRef &expr) {
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
 * Returns the contained dependency list.
 */
const DataDependencies::Deps &DataDependencies::get() const {
    return deps;
}

/**
 * Adds a single dependency on an object. Literal access mode is upgraded to
 * read mode, as it makes no sense to access an object in literal mode (this
 * should never happen for consistent IRs though, unless this is explicitly
 * called this way). If there was already a dependency for the object, the
 * access mode is combined: if they match the mode is maintained, otherwise
 * the mode is changed to write.
 */
void DataDependencies::add_object(
    prim::AccessMode mode,
    const ObjectLink &obj
) {
    if (mode == prim::AccessMode::LITERAL) {
        mode = prim::AccessMode::READ;
    }
    auto it = deps.find(obj);
    if (it == deps.end()) {
        deps.insert(it, {obj, mode});
    } else if (it->second != mode) {
        it->second = prim::AccessMode::WRITE;
    }
}

/**
 * Adds dependencies on whatever is used by a complete expression.
 */
void DataDependencies::add_expression(
    prim::AccessMode mode,
    const ExpressionRef &expr
) {
    if (auto ref = expr->as_reference()) {
        add_object(mode, ref->target);
    } else if (auto cast = expr->as_typecast()) {
        add_expression(mode, cast->expression);
    } else if (auto call = expr->as_function_call()) {
        add_operands(call->function_type->operand_types, call->operands);
    }
}

/**
 * Adds dependencies on the operands of a function or instruction.
 */
void DataDependencies::add_operands(
    const utils::Any<OperandType> &prototype,
    const utils::Any<Expression> &operands
) {
    for (utils::UInt i = 0; i < prototype.size(); i++) {
        add_expression(prototype[i]->mode, operands[i]);
    }
}

/**
 * Adds dependencies for a complete statement.
 */
void DataDependencies::add_statement(const StatementRef &stmt) {
    if (auto cond = stmt->as_conditional_instruction()) {
        add_expression(prim::AccessMode::READ, cond->condition);
        if (auto custom = stmt->as_custom_instruction()) {
        add_operands(custom->instruction_type->operand_types, custom->operands);
        } else if (auto set = stmt->as_set_instruction()) {
            add_expression(prim::AccessMode::WRITE, set->lhs);
            add_expression(prim::AccessMode::READ, set->rhs);
        } else if (stmt->as_goto_instruction()) {
            // no dependencies
        } else {
            QL_ASSERT(false);
        }
    } else if (stmt->as_wait_instruction()) {
        // no dependencies
    } else if (stmt->as_dummy_instruction()) {
        // no dependencies
    } else if (auto if_else = stmt->as_if_else()) {
        for (const auto &branch : if_else->branches) {
            add_expression(prim::AccessMode::READ, branch->condition);
            add_block(branch->body);
        }
        if (!if_else->otherwise.empty()) {
            add_block(if_else->otherwise);
        }
    } else if (auto loop = stmt->as_loop()) {
        add_block(loop->body);
        if (auto stat = stmt->as_static_loop()) {
            add_expression(prim::AccessMode::WRITE, stat->lhs);
        } else if (auto dyn = stmt->as_dynamic_loop()) {
            add_expression(prim::AccessMode::READ, dyn->condition);
            if (auto forl = stmt->as_for_loop()) {
                add_statement(forl->initialize);
                add_statement(forl->update);
            } else if (stmt->as_repeat_until_loop()) {
                // no further dependencies
            } else {
                QL_ASSERT(false);
            }
        } else {
            QL_ASSERT(false);
        }
    } else if (stmt->as_loop_control_statement()) {
        // no dependencies
    } else {
        QL_ASSERT(false);
    }
}

/**
 * Adds dependencies for a whole (sub)block of statements.
 */
void DataDependencies::add_block(const SubBlockRef &block) {
    for (const auto &stmt : block->statements) {
        add_statement(stmt);
    }
}

/**
 * Clears the dependency list, allowing the object to be reused.
 */
void DataDependencies::reset() {
    deps.clear();
}

} // namespace ir
} // namespace ql
