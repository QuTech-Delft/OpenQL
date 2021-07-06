/** \file
 * Defines the structures and functions used to construct the data dependency
 * graph for a block.
 */

#include "ql/com/ddg/build.h"

#include "ql/ir/ops.h"
#include "ql/com/ddg/ops.h"

namespace ql {
namespace com {
namespace ddg {

/**
 * Constructs an object reference gatherer.
 */
EventGatherer::EventGatherer(const ir::Ref &ir) : ir(ir) {}

/**
 * Returns the contained dependency list.
 */
const Events &EventGatherer::get() const {
    return events;
}

/**
 * Adds a single reference. Literal access mode is upgraded to read
 * mode, as it makes no sense to access an object in literal mode (this
 * should never happen for consistent IRs though, unless this is explicitly
 * called this way). Measure access mode is upgraded to a write access to
 * both the qubit and the implicit bit associated with it. If there was
 * already an access for the object, the access mode is combined: if they
 * match the mode is maintained, otherwise the mode is changed to write.
 */
void EventGatherer::add_reference(
    ir::prim::OperandMode mode,
    const utils::One<ir::Reference> &reference
) {
    AccessMode amode = AccessMode::WRITE;
    switch (mode) {
        case ir::prim::OperandMode::WRITE:     amode = AccessMode::WRITE;     break;
        case ir::prim::OperandMode::READ:      amode = AccessMode::READ;      break;
        case ir::prim::OperandMode::LITERAL:   amode = AccessMode::READ;      break;
        case ir::prim::OperandMode::COMMUTE_X: amode = AccessMode::COMMUTE_X; break;
        case ir::prim::OperandMode::COMMUTE_Y: amode = AccessMode::COMMUTE_Y; break;
        case ir::prim::OperandMode::COMMUTE_Z: amode = AccessMode::COMMUTE_Z; break;
        case ir::prim::OperandMode::MEASURE: {
            QL_ASSERT(reference->data_type->as_qubit_type());
            auto copy = reference->copy().as<ir::Reference>();
            copy->data_type = ir->platform->implicit_bit_type;
            add_reference(ir::prim::OperandMode::WRITE, copy);
            amode = AccessMode::WRITE;
            break;
        }
        case ir::prim::OperandMode::IGNORE: return;
    }
    auto sref = Reference(reference);
    auto it = events.find(reference);
    if (it == events.end()) {
        events.insert(it, {sref, amode});
    } else {
        it->second = combine_modes(it->second, amode);
    }
}

/**
 * Adds dependencies on whatever is used by a complete expression.
 */
void EventGatherer::add_expression(
    ir::prim::OperandMode mode,
    const ir::ExpressionRef &expr
) {
    if (expr->as_reference()) {
        add_reference(mode, expr.as<ir::Reference>());
    } else if (auto call = expr->as_function_call()) {
        add_operands(call->function_type->operand_types, call->operands);
    }
}

/**
 * Adds dependencies on the operands of a function or instruction.
 */
void EventGatherer::add_operands(
    const utils::Any<ir::OperandType> &prototype,
    const utils::Any<ir::Expression> &operands
) {
    utils::UInt num_qubits = 0;
    for (auto &otyp : prototype) {
        if (otyp->data_type->as_qubit_type()) {
            num_qubits++;
        }
    }
    auto disable_qubit_commutation = (
        (num_qubits == 1 && disable_single_qubit_commutation) ||
        (num_qubits > 1 && disable_multi_qubit_commutation)
    );
    for (utils::UInt i = 0; i < prototype.size(); i++) {
        auto mode = prototype[i]->mode;
        if (disable_qubit_commutation) {
            switch (mode) {
                case ir::prim::OperandMode::COMMUTE_X:
                case ir::prim::OperandMode::COMMUTE_Y:
                case ir::prim::OperandMode::COMMUTE_Z:
                    mode = ir::prim::OperandMode::WRITE;
                default:
                    break;
            }
        }
        add_expression(mode, operands[i]);
    }
}

/**
 * Adds dependencies for a complete statement.
 */
void EventGatherer::add_statement(const ir::StatementRef &stmt) {
    auto barrier = false;
    if (auto cond = stmt->as_conditional_instruction()) {
        add_expression(ir::prim::OperandMode::READ, cond->condition);
        if (auto custom = stmt->as_custom_instruction()) {
            add_operands(custom->instruction_type->operand_types, custom->operands);
            if (!custom->instruction_type->template_operands.empty()) {
                auto gen = custom->instruction_type;
                while (!gen->generalization.empty()) gen = gen->generalization;
                for (utils::UInt i = 0; i < custom->instruction_type->template_operands.size(); i++) {
                    add_expression(
                        gen->operand_types[i]->mode,
                        custom->instruction_type->template_operands[i]
                    );
                }
            }
        } else if (auto set = stmt->as_set_instruction()) {
            add_expression(ir::prim::OperandMode::WRITE, set->lhs);
            add_expression(ir::prim::OperandMode::READ, set->rhs);
        } else if (stmt->as_goto_instruction()) {
            barrier = true;
        } else {
            QL_ASSERT(false);
        }
    } else if (auto wait = stmt->as_wait_instruction()) {
        if (wait->objects.empty()) {
            barrier = true;
        } else {
            for (const auto &ref : wait->objects) {
                add_expression(ir::prim::OperandMode::WRITE, ref);
            }
        }
    } else if (auto if_else = stmt->as_if_else()) {
        for (const auto &branch : if_else->branches) {
            add_expression(ir::prim::OperandMode::READ, branch->condition);
            add_block(branch->body);
        }
        if (!if_else->otherwise.empty()) {
            add_block(if_else->otherwise);
        }
    } else if (auto loop = stmt->as_loop()) {
        add_block(loop->body);
        if (auto stat = stmt->as_static_loop()) {
            add_expression(ir::prim::OperandMode::WRITE, stat->lhs);
        } else if (auto dyn = stmt->as_dynamic_loop()) {
            add_expression(ir::prim::OperandMode::READ, dyn->condition);
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
        barrier = true;
    } else if (stmt->as_sentinel_statement()) {
        barrier = true;
    } else {
        QL_ASSERT(false);
    }

    // Generate data dependencies for barrier-like instructions. Instructions
    // can shift around between barriers (as read accesses commute), but they
    // cannot cross a barrier, and barriers themselves cannot commute.
    add_reference(barrier ? ir::prim::OperandMode::WRITE : ir::prim::OperandMode::READ, {});

}

/**
 * Adds dependencies for a whole (sub)block of statements.
 */
void EventGatherer::add_block(const ir::SubBlockRef &block) {
    for (const auto &stmt : block->statements) {
        add_statement(stmt);
    }
}

/**
 * Clears the dependency list, allowing the object to be reused.
 */
void EventGatherer::reset() {
    events.clear();
}

/**
 * Builds a forward data dependency graph for the given block.
 * commute_multi_qubit and commute_single_qubit allow the COMMUTE_* operand
 * access modes to be disabled for single- and/or multi-qubit gates.
 *
 * The nodes of the graph are represented by the statements in the block and
 * two sentinel statements, known as the source and the sink. The edges are
 * formed by dependencies from one instruction to another. Edges are weighted
 * such that the absolute value of the weight indicates the minimum number of
 * cycles that must be between the start cycle of the source and destination
 * node in the final schedule, and such that the sign indicates the direction
 */
void build(
    const ir::BlockBaseRef &block,
    utils::Bool commute_multi_qubit,
    utils::Bool commute_single_qubit
) {
    clear(block);
    // TODO
}

} // namespace ddg
} // namespace com
} // namespace ql
