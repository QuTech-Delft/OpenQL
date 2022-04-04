/** \file
 * Constant propagation pass.
 */

#include "ql/pass/opt/const_prop/const_prop.h"

#include "ql/ir/ops.h"
#include "ql/pmgr/pass_types/base.h"

namespace ql {
namespace pass {
namespace opt {
namespace const_prop {

/**
 * Dumps docs for constant propagator.
 */
void ConstantPropagationPass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix, R"(
    This pass replaces constant expressions by their result.
    )");
}

/**
 * Returns a user-friendly type name for this pass.
 */
utils::Str ConstantPropagationPass::get_friendly_type() const {
    return "Constant propagator";
}

/**
 * Constructs an constant propagator.
 */
ConstantPropagationPass::ConstantPropagationPass(
    const utils::Ptr<const pmgr::Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::Transformation(pass_factory, instance_name, type_name) {
}

/**
 * Runs the constant propagator on the given block.
 */
void ConstantPropagationPass::run_on_block(
    const ir::Ref &ir,
    const ir::BlockBaseRef &block
) {
    for (const auto &statement : block->statements) {
        auto insn = statement.as<ir::Instruction>();
        if (!insn.empty()) {
//            ir::generalize_instruction(insn);
        }
        if (auto if_else = statement->as_if_else()) {
            for (const auto &branch : if_else->branches) {
                run_on_block(ir, branch->body);
            }
            if (!if_else->otherwise.empty()) {
                run_on_block(ir, if_else->otherwise);
            }
        } else if (auto loop = statement->as_loop()) {
            run_on_block(ir, loop->body);
        }
    }
}

/**
 * Runs the constant propagator.
 */
utils::Int ConstantPropagationPass::run(
    const ir::Ref &ir,
    const pmgr::pass_types::Context &context
) const {
    // register functions

    // perform constant propagation
    if (!ir->program.empty()) {
        for (const auto &block : ir->program->blocks) {
            run_on_block(ir, block);
        }
    }
    return 0;
}

} // namespace const_prop
} // namespace opt
} // namespace pass
} // namespace ql
