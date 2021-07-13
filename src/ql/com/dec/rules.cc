/** \file
 * Custom instruction decomposition rule processing logic.
 */

#include "ql/com/dec/rules.h"

#include "ql/ir/ops.h"
#include "ql/ir/describe.h"
#include "ql/com/map/expression_mapper.h"

namespace ql {
namespace com {
namespace dec {

/**
 * Class for implementing map operations on expressions or references. While not
 * an abstract class, this must be subclassed to be useful; either or both of
 * on_expression() and on_reference() must be overridden with the desired map
 * operation.
 */
class DecompositionRuleExpressionMapper : public map::ExpressionMapper {
public:

    /**
     * Map of variable references that need to be updated.
     */
    utils::Map<ir::ObjectLink, ir::ObjectLink> variable_map;

    /**
     * Map of operand references/parameters that need to be updated.
     */
    utils::Map<ir::ObjectLink, ir::ExpressionRef> operand_map;

protected:

    /**
     * Called when an expression of any kind is encountered in the tree. The
     * subtree formed by the expression will already have been processed (i.e.
     * traversal is depth-first.) The method may assign the Maybe edge to
     * change the complete expression (including its node type), or may change
     * the contents of the expression. If the method returns true, the subtree
     * formed by the new expression will be processed as well. The default
     * implementation calls on_reference() if the expression is a reference.
     */
    utils::Bool on_expression(utils::Maybe<ir::Expression> &expr) override {

        // We only have to worry about replacing references with things.
        auto ref = expr->as_reference();
        if (!ref) {
            return false;
        }

        // Handle variables.
        auto it1 = variable_map.find(ref->target);
        if (it1 != variable_map.end()) {
            QL_ASSERT(ref->target->data_type == it1->second->data_type);
            ref->target = it1->second;
            return true;
        }

        // Handle parameters.
        auto it2 = operand_map.find(ref->target);
        if (it2 != operand_map.end()) {
            expr = it2->second.clone();
            return true;
        }

        return false;
    }

    /**
     * Like on_expression(), but called for edges that must always be a
     * reference of some kind. The default implementation is no-op and just
     * returns false.
     */
    utils::Bool on_reference(utils::Maybe<ir::Reference> &ref) override {
        utils::Maybe<ir::Expression> expr = ref;
        if (on_expression(expr)) {
            auto ref2 = expr.as<ir::Reference>();
            if (ref2.empty()) {
                QL_USER_ERROR(
                    "failed to perform expansion of " << ir::describe(ref) <<
                    " to " << ir::describe(expr) << "; reference expected " <<
                    "expansion"
                );
            }
            ref = ref2;
            return true;
        }
        return false;
    }

public:

    /**
     * Default constructor.
     */
    DecompositionRuleExpressionMapper() = default;

};

/**
 * Recursively applies all available decomposition rules (that match the
 * predicate, if given) to the given block. If ignore_schedule is set, the
 * schedule of the decomposition rules is ignored, and instead the statements
 * in the rule are all given the same cycle number as the original statement.
 * If ignore_schedule is not set, the schedule is copied from the decomposition
 * rule, possibly resulting in instructions being reordered.
 */
utils::UInt apply_decomposition_rules(
    const ir::Ref &ir,
    const ir::BlockBaseRef &block,
    utils::Bool ignore_schedule,
    const std::function<utils::Bool(const ir::DecompositionRef&)> &predicate
) {

    // Make a list of the statements we haven't processed yet, and clear the
    // block. We'll add the statements back to the block as we process them.
    utils::List<ir::StatementRef> remaining;
    for (const auto &statement : block->statements) {
        remaining.push_back(statement);
    }
    block->statements.reset();

    // Process the statements.
    utils::UInt number_of_applications = 0;
    while (!remaining.empty()) {
        const auto &stmt = remaining.front();
        utils::Bool rule_applied = false;
        if (auto insn = stmt->as_custom_instruction()) {
            for (const auto &rule : insn->instruction_type->decompositions) {

                // Ignore decomposition rules that don't match the predicate.
                if (!predicate(rule)) {
                    continue;
                }

                // Expression mapper for updating variable and parameter
                // references in the expansion.
                DecompositionRuleExpressionMapper mapper;

                // Add any variables declared in the decomposition rule as
                // temporary objects, and remember that we have to remap them.
                for (const auto &var : rule->objects) {
                    mapper.variable_map.insert({
                        var,
                        make_temporary(ir, var->data_type, var->shape)
                    });
                }

                // Figure out how to map the operand placeholders.
                QL_ASSERT(rule->parameters.size() == insn->operands.size());
                for (utils::UInt i = 0; i < rule->parameters.size(); i++) {
                    mapper.operand_map.insert({
                        rule->parameters[i],
                        insn->operands[i]
                    });
                }

                // Perform the expansion.
                for (const auto &orig_exp_stmt : rule->expansion) {
                    auto exp_stmt = orig_exp_stmt.clone();
                    mapper.process_statement(exp_stmt);
                    remaining.push_front(exp_stmt);
                }

                rule_applied = true;
                break;
            }
        }
        if (rule_applied) {
            number_of_applications++;
        } else {
            block->statements.add(stmt);
        }
        remaining.pop_front();
    }

    // Make sure that the statements are ordered by cycle. This is only
    // necessary if we respected the schedule of the decomposition rules.
    if (!ignore_schedule) {
        std::stable_sort(
            block->statements.begin(),
            block->statements.end(),
            [](const ir::StatementRef &a, const ir::StatementRef &b) {
                return a->cycle < b->cycle;
            }
        );
    }

    return number_of_applications;
}

} // namespace dec
} // namespace com
} // namespace ql
