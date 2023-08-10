/** \file
 * Instruction generalizer pass.
 */

#include "ql/pass/dec/generalize/generalize.h"

#include "ql/ir/ops.h"
#include "ql/pmgr/pass_types/base.h"
#include "ql/pmgr/factory.h"

namespace ql {
namespace pass {
namespace dec {
namespace generalize {

bool GeneralizeInstructionsPass::is_pass_registered = pmgr::Factory::register_pass<GeneralizeInstructionsPass>("dec.Generalize");

/**
 * Dumps docs for the instruction generalizer.
 */
void GeneralizeInstructionsPass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix, R"(
    This pass converts the format of all instructions in the program to their
    most generalized form. For example, if a specialized CNOT gate exists for
    qubits 1 and 2 and this specialization is used in the program, the
    instruction is changed to the generalized version for any set of qubits.
    This implements the reverse operation of `dec.Specialize`.
    )");
}

/**
 * Returns a user-friendly type name for this pass.
 */
utils::Str GeneralizeInstructionsPass::get_friendly_type() const {
    return "Instruction generalizer";
}

/**
 * Constructs an instruction generalizer.
 */
GeneralizeInstructionsPass::GeneralizeInstructionsPass(
    const utils::Ptr<const pmgr::Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::Transformation(pass_factory, instance_name, type_name) {
}

/**
 * Runs the instruction generalizer on the given block.
 */
void GeneralizeInstructionsPass::run_on_block(
    const ir::Ref &ir,
    const ir::BlockBaseRef &block
) {
    for (const auto &statement : block->statements) {
        auto insn = statement.as<ir::Instruction>();
        if (!insn.empty()) {
            ir::generalize_instruction(insn);
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
 * Runs the instruction generalizer.
 */
utils::Int GeneralizeInstructionsPass::run(
    const ir::Ref &ir,
    const pmgr::pass_types::Context &/* context */
) const {
    if (!ir->program.empty()) {
        for (const auto &block : ir->program->blocks) {
            run_on_block(ir, block);
        }
    }
    return 0;
}

} // namespace generalize
} // namespace dec
} // namespace pass
} // namespace ql
