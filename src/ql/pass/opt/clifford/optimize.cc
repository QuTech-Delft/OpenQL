/** \file
 * Defines the Clifford optimizer pass.
 */

#include "ql/pass/opt/clifford/optimize.h"

#include "ql/pmgr/pass_types/base.h"
#include "ql/pass/ana/statistics/annotations.h"
#include "detail/clifford.h"

namespace ql {
namespace pass {
namespace opt {
namespace clifford {
namespace optimize {

/**
 * Dumps docs for the Clifford optimizer.
 */
void CliffordOptimizePass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix, R"(
    This pass tries to minimize sequences of single-qubit gates in the Clifford
    C1 set to their minimal counterpart in terms of cycles. The pass returns the
    total number of cycles saved by this optimization per qubit.

    Note that the relation between the Clifford state transition corresponding
    to a particular gate is currently hardcoded based on gate name, and the
    equivalent cycle counts are also hardcoded.
    )");
}

/**
 * Returns a user-friendly type name for this pass.
 */
utils::Str CliffordOptimizePass::get_friendly_type() const {
    return "Clifford gate optimizer";
}

/**
 * Constructs a Clifford optimizer.
 */
CliffordOptimizePass::CliffordOptimizePass(
    const utils::Ptr<const pmgr::Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::KernelTransformation(pass_factory, instance_name, type_name) {
}

/**
 * Runs the Clifford optimizer.
 */
utils::Int CliffordOptimizePass::run(
    const ir::compat::ProgramRef &program,
    const ir::compat::KernelRef &kernel,
    const pmgr::pass_types::Context &context
) const {
    auto cycles_saved = detail::Clifford().optimize_kernel(kernel);
    ana::statistics::AdditionalStats::push(
        kernel,
        utils::to_string(cycles_saved) + " cycles saved by " + context.full_pass_name
    );
    return cycles_saved;
}

} // namespace optimize
} // namespace clifford
} // namespace opt
} // namespace pass
} // namespace ql
