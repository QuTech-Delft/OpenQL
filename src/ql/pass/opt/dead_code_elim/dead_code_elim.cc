/** \file
 * Dead code elimination pass.
 */

#include "ql/pass/opt/dead_code_elim/dead_code_elim.h"

#include "ql/pmgr/pass_types/base.h"
#include "ql/ir/describe.h"

#define DEBUG(x) QL_DOUT(x)

namespace ql {
namespace pass {
namespace opt {
namespace dead_code_elim {

/**
 * Constructs a dead code elimination pass.
 */
DeadCodeEliminationPass::DeadCodeEliminationPass(
    const utils::Ptr<const pmgr::Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::Transformation(pass_factory, instance_name, type_name) {
}

/**
 * Returns a user-friendly type name for this pass.
 */
utils::Str DeadCodeEliminationPass::get_friendly_type() const {
    return "Dead code eliminator";
}

/**
 * Runs the dead code elimination pass on the given block.
 */
void DeadCodeEliminationPass::run_on_block(
    const ir::BlockBaseRef &block
) {
    DEBUG("running dead code elimination on block");
    for (utils::UInt stmt_idx = 0; stmt_idx < block->statements.size(); stmt_idx++) {    // NB: we need index for add() below
        const auto &statement = block->statements[stmt_idx];

        // handle if_else.
        //
        // Note that this is especially useful for 'parameterized gate decomposition', e.g. (noting that a real
        // example would feature a longer if-tree):
        //         "_rx": {
        //           "prototype": ["X:qubit", "L:int"],
        //           "duration": 20,
        //           "decomposition": {
        //               "into": [
        //                     "if (op(1) < 45) {",
        //                    "   rx45 op(0)",
        //                    "} else {",
        //                    "   rx90 op(0)",
        //                    "}"
        if (auto if_else = statement->as_if_else()) {
            utils::UInt if_else_idx = stmt_idx;
            bool remove_if_else = false;

            // remove unreachable branches, and maybe the complete if_else statement
            for (utils::UInt branch_idx = 0; branch_idx < if_else->branches.size(); ) {    // NB: we need index for remove() below
                auto &branch = if_else->branches[branch_idx];
                if (auto condition = branch->condition->as_bit_literal()) {
                    if (condition->value) {   // condition 'true'
                        // descend body
                        run_on_block(branch->body);

                        // delete subsequent if_else branches and if_else->otherwise, since these are unreachable
                        QL_IOUT("found 'if_else(true)': removing unreachable if_else-branches and if_else->otherwise");
                        for (utils::UInt i = branch_idx + 1; i < if_else->branches.size(); i++) {
                            if_else->branches.remove(i);
                        }
                        if_else->otherwise.reset();     // removes Maybe item

                        // if we're the sole if_else-branch (remaining), turn body (sub_block) into statements
                        if(branch_idx == 0) {
                            QL_IOUT("turn body of sole 'if(true)' branch into statements");
                            for (auto &st : branch->body->statements) {
                                block->statements.add(st, stmt_idx++);  // NB: increment affects both this add and statement loop
                            }

                            // mark complete if_else statement for removal
                            remove_if_else = true;
                        }

                        // NB: done looping, since we erased everything that followed
                        break;

                    } else {    // condition 'false'
                        // NB: no need to descend body, since we'll discard it
                        QL_IOUT("removing dead if-branch " << branch_idx);
                        if_else->branches.remove(branch_idx);
                        continue;   // loop, using same value for branch_idx

                        // NB: may remove all if_else branches (which we repair later)
                    }

                // condition is not a bit_literal
                } else {
                    // descend body
                    run_on_block(branch->body);
                }
                branch_idx++;
            }

            // descend otherwise
            if (!if_else->otherwise.empty()) {
                run_on_block(if_else->otherwise);
            }

            // if we no longer have branches, but do have otherwise, promote its body into statements within this block
            if (if_else->branches.empty()) {
                if (!if_else->otherwise.empty()) {
                    QL_IOUT("turn body of final 'if_else->otherwise' into statements");

                    // move if_else->otherwise statements
                    for (auto &st : if_else->otherwise->statements) {
                        block->statements.add(st, stmt_idx++);  // NB: increment affects both this add and statement loop
                    }
                }

                remove_if_else = true;
            }

            // if needed, remove the complete if_else statement altogether
            if (remove_if_else) {
                block->statements.remove(if_else_idx-1);    // FIXME: why is -1 necessary?
            }

        // handle loop
        } else if (auto loop = statement->as_loop()) {
            // descend loop body
            run_on_block(loop->body);

            // NB: we currently have no real use for optimizing static loops, note that we cannot fully
            // remove loop anyway if break or continue exists
        }
    }
}

/**
 * Runs the dead code elimination pass.
 */
utils::Int DeadCodeEliminationPass::run(
    const ir::Ref &ir,
    const pmgr::pass_types::Context &context
) const {
    // perform dead code elimination
    if (!ir->program.empty()) {
        for (const auto &block : ir->program->blocks) {
            run_on_block(block);
        }
    }
    return 0;
}

/**
 * Dumps docs for dead code elimination pass.
 */
void DeadCodeEliminationPass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix, R"(
    This pass removes dead code, currently only unreachable if-branches.
    )");
}

} // namespace dead_code_elim
} // namespace opt
} // namespace pass
} // namespace ql
