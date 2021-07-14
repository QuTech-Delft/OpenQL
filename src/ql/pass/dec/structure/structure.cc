/** \file
 * Structure decomposition pass.
 */

#include "ql/pass/dec/structure/structure.h"

#include "ql/utils/filesystem.h"
#include "ql/com/dec/structure.h"
#include "ql/com/cfg/build.h"
#include "ql/com/cfg/consistency.h"
#include "ql/com/cfg/dot.h"
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

    Optionally, the control-flow graph of the resulting program can be printed
    as in graphviz dot format.
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
    options.add_bool(
        "write_dot_graph",
        "Writes the control-flow graph of the resulting program in the dot "
        "format. The file is written with suffix \".dot\".",
        false
    );
}

/**
 * Runs the structure decomposer.
 */
utils::Int DecomposeStructurePass::run(
    const ir::Ref &ir,
    const pmgr::pass_types::Context &context
) const {

    // Perform the decomposition.
    ir->program = com::dec::decompose_structure(ir, true);

    // If requested, write a control-flow graph of the result.
    if (options["write_dot_graph"].as_bool()) {
        com::cfg::build(ir->program);
        com::cfg::check_consistency(ir->program);
        com::cfg::dump_dot(ir, utils::OutFile(context.output_prefix + ".dot").unwrap());
        com::cfg::clear(ir->program);
    }

    return 0;
}

} // namespace structure
} // namespace dec
} // namespace pass
} // namespace ql
