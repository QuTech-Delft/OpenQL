/** \file
 * Structure decomposition pass.
 */

#include "ql/pass/dec/structure/structure.h"

#include "ql/com/dec/structure.h"
#include "ql/pmgr/pass_types/base.h"

namespace ql {
namespace pass {
namespace dec {
namespace structure {

/**
 * Dumps docs for the structure decomposer.
 */
void DecomposeStructurePass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix, R"(
    This pass converts the program to basic block form. Specifically, the
    postcondition for this pass is:

     - all blocks consist of only instructions (no control-flow statements like
       loops or if-conditionals); and
     - only the last instruction of each block may be a goto instruction.

    All control-flow that exists in the program before this pass is reduced to
    this basic form. This doesn't change the behavior of the program, but all
    information about the program structure is lost. Because of this, this
    should be one of the last passes, if the pass is needed at all; this depends
    on the code generator used, or on whether there is a need for passes that
    rely on basic-block form and the corresponding control-flow graph to
    operate.
    )");
}

/**
 * Returns a user-friendly type name for this pass.
 */
utils::Str DecomposeStructurePass::get_friendly_type() const {
    return "Structure decomposer";
}

/**
 * Constructs a structure decomposer.
 */
DecomposeStructurePass::DecomposeStructurePass(
    const utils::Ptr<const pmgr::Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::Transformation(pass_factory, instance_name, type_name) {
}

/**
 * Runs the structure decomposer.
 */
utils::Int DecomposeStructurePass::run(
    const ir::Ref &ir,
    const pmgr::pass_types::Context &context
) const {
    ir->program = com::dec::decompose_structure(ir, true);
    return 0;
}

} // namespace structure
} // namespace dec
} // namespace pass
} // namespace ql
