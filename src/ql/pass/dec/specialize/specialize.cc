/** \file
 * Instruction specializer pass.
 */

#include "ql/pass/dec/specialize/specialize.h"

#include "ql/ir/ops.h"
#include "ql/pmgr/pass_types/base.h"
#include "ql/pmgr/factory.h"

namespace ql {
namespace pass {
namespace dec {
namespace specialize {

bool SpecializeInstructionsPass::is_pass_registered = pmgr::Factory::register_pass<SpecializeInstructionsPass>("dec.Specialize");

/**
 * Dumps docs for the instruction specializer.
 */
void SpecializeInstructionsPass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix, R"(
    This pass converts the format of all instructions in the program to their
    most specialized form. For example, if a generalized CNOT gate exists for
    qubits 1 and 2, and a specialization exists for this qubit pair as well,
    the instruction is changed to the specialized version This implements the
    reverse operation of `dec.Generalize`.
    )");
}

/**
 * Returns a user-friendly type name for this pass.
 */
utils::Str SpecializeInstructionsPass::get_friendly_type() const {
    return "Instruction specializer";
}

/**
 * Constructs an instruction specializer.
 */
SpecializeInstructionsPass::SpecializeInstructionsPass(
    const utils::Ptr<const pmgr::Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::Transformation(pass_factory, instance_name, type_name) {
}

/**
 * Runs the instruction specializer on the given block.
 */
void SpecializeInstructionsPass::run_on_block(
    const ir::Ref &ir,
    const ir::BlockBaseRef &block
) {
    for (const auto &statement : block->statements) {
        auto insn = statement.as<ir::Instruction>();
        if (!insn.empty()) {
            ir::specialize_instruction(insn);
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
 * Runs the instruction specializer.
 */
utils::Int SpecializeInstructionsPass::run(
    const ir::Ref &ir,
    const pmgr::pass_types::Context &context
) const {
    if (!ir->program.empty()) {
        for (const auto &block : ir->program->blocks) {
            run_on_block(ir, block);
        }
    }
    return 0;
}

} // namespace specialize
} // namespace dec
} // namespace pass
} // namespace ql
