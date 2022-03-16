/** \file
 * Defines the ExpressionMapper base class.
 */

#include "ql/com/map/expression_mapper.h"
#include "ql/ir/describe.h"

namespace ql {
namespace com {
namespace map {

/**
 * Called when an expression of any kind is encountered in the tree. The
 * subtree formed by the expression will already have been processed (i.e.
 * traversal is depth-first.) The method may assign the Maybe edge to
 * change the complete expression (including its node type), or may change
 * the contents of the expression. If the method returns true, the subtree
 * formed by the new expression will be processed as well. The default
 * implementation calls on_reference() if the expression is a reference.
 */
utils::Bool ExpressionMapper::on_expression(utils::Maybe<ir::Expression> &expr) {
    auto ref = expr.as<ir::Reference>();
    if (!ref.empty() && on_reference(ref)) {
        expr = ref;
        return true;
    }
    return false;
}

/**
 * Like on_expression(), but called for edges that must always be a
 * reference of some kind. The default implementation is no-op and just
 * returns false.
 */
utils::Bool ExpressionMapper::on_reference(utils::Maybe<ir::Reference> &ref) {
    return false;
}

/**
 * Handles visiting an expression subtree before or after on_expression()
 * is called by the callee.
 */
void ExpressionMapper::recurse_into_expression(const ir::ExpressionRef &expression) {
    if (expression->as_literal()) {
        // nothing to do
    } else if (expression->as_reference()) {
        // nothing to do
    } else if (auto func = expression->as_function_call()) {
        for (auto &operand : func->operands) {
            process_expression(operand);
        }
    } else {
        QL_ASSERT(false);
    }
}

/**
 * Visits an expression. This processes the subtree formed by the expression
 * depth-first, then calls on_expression(), and if that returns true
 * processes the new subtree depth-first.
 */
void ExpressionMapper::process_expression(utils::Maybe<ir::Expression> &expression) {
    recurse_into_expression(expression);
    if (on_expression(expression)) {
        recurse_into_expression(expression);
    }
}

/**
 * Visits a statement. on_expression()/on_reference() will be called for
 * all expression/reference edges found in the statement, depth-first.
 */
void ExpressionMapper::process_statement(const ir::StatementRef &statement) {
    if (auto cond_insn = statement->as_conditional_instruction()) {
        QL_IOUT("processing condition: " + ir::describe(cond_insn->condition));
        process_expression(cond_insn->condition);
        QL_IOUT("resulting condition: " + ir::describe(cond_insn->condition));
        if (auto custom_insn = statement->as_custom_instruction()) {
            for (auto &operand : custom_insn->operands) {
                process_expression(operand);
            }
        } else if (auto set_insn = statement->as_set_instruction()) {
            on_reference(set_insn->lhs);
            process_expression(set_insn->rhs);
        } else if (statement->as_goto_instruction()) {
            // no expressions here
        } else {
            QL_ASSERT(false);
        }
    } else if (auto wait_insn = statement->as_wait_instruction()) {
        for (auto &reference : wait_insn->objects) {
            on_reference(reference);
        }
    } else if (auto if_else = statement->as_if_else()) {
        for (const auto &branch : if_else->branches) {
            process_expression(branch->condition);
            process_block(branch->body);
        }
        if (!if_else->otherwise.empty()) {
            process_block(if_else->otherwise);
        }
    } else if (auto loop_stmt = statement->as_loop()) {
        process_block(loop_stmt->body);
        if (auto static_loop = statement->as_static_loop()) {
            on_reference(static_loop->lhs);
        } else if (auto dynamic_loop = statement->as_dynamic_loop()) {
            process_expression(dynamic_loop->condition);
            if (auto for_loop = statement->as_for_loop()) {
                if (!for_loop->initialize.empty()) {
                    process_statement(for_loop->initialize);
                }
                if (!for_loop->update.empty()) {
                    process_statement(for_loop->update);
                }
            } else if (statement->as_repeat_until_loop()) {
                // no expressions here
            } else {
                QL_ASSERT(false);
            }
        }
    } else if (statement->as_loop_control_statement()) {
        // no expressions here
    } else {
        QL_ASSERT(false);
    }
}

/**
 * Visits a block. on_expression()/on_reference() will be called for
 * all expression/reference edges found in the block, depth-first.
 */
void ExpressionMapper::process_block(const ir::BlockBaseRef &block) {
    for (const auto &stmt : block->statements) {
        process_statement(stmt);
    }
}

} // namespace map
} // namespace com
} // namespace ql
