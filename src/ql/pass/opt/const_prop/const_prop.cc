/** \file
 * Constant propagation pass.
 */

#include "ql/pass/opt/const_prop/const_prop.h"
#include "detail/propagate.h"

#include "ql/pmgr/pass_types/base.h"

namespace ql {
namespace pass {
namespace opt {
namespace const_prop {

/**
 * Constructs a constant propagation pass.
 */
ConstantPropagationPass::ConstantPropagationPass(
    const utils::Ptr<const pmgr::Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::Transformation(pass_factory, instance_name, type_name) {
}

/**
 * Returns a user-friendly type name for this pass.
 */
utils::Str ConstantPropagationPass::get_friendly_type() const {
    return "Constant propagator";
}

/**
 * Runs the constant propagation pass.
 */
utils::Int ConstantPropagationPass::run(
    const ir::Ref &ir,
    const pmgr::pass_types::Context &context
) const {
    // perform constant propagation
    if (!ir->program.empty()) {
        for (auto &block : ir->program->blocks) {
            detail::propagate(ir, *block);
        }
    }
    return 0;
}

/**
 * Dumps docs for constant propagation pass.
 */
void ConstantPropagationPass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix, R"(
    This pass replaces constant expressions by their result.
    )");
}

} // namespace const_prop
} // namespace opt
} // namespace pass
} // namespace ql
