/** \file
 * Defines the QuTech Central Controller Q1 processor assembly generator pass.
 */

#include "ql/arch/cc/pass/gen/vq1asm/vq1asm.h"

#include "ql/pmgr/pass_types.h"
#include "detail/backend.h"

namespace ql {
namespace arch {
namespace cc {
namespace pass {
namespace gen {
namespace vq1asm {

/**
 * Dumps docs for the code generator.
 */
void GenerateVQ1AsmPass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix, R"(
    Assembly code generator for the Q1 processor in the QuTech Central
    Controller.
    )");
}

/**
 * Constructs a code generator.
 */
GenerateVQ1AsmPass::GenerateVQ1AsmPass(
    const utils::Ptr<const pmgr::PassFactory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::KernelTransformation(pass_factory, instance_name, type_name) {

}

/**
 * Runs the code generator.
 */
utils::Int GenerateVQ1AsmPass::run(
    const ir::ProgramRef &program,
    const ir::KernelRef &kernel,
    const pmgr::pass_types::Context &context
) const {

    return 0;
}

} // namespace vq1asm
} // namespace gen
} // namespace pass
} // namespace cc
} // namespace arch
} // namespace ql
