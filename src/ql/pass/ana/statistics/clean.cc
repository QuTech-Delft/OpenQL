/** \file
 * Defines the statistics cleaning pass.
 */

#include "ql/pass/ana/statistics/clean.h"

namespace ql {
namespace pass {
namespace ana {
namespace statistics {
namespace clean {

/**
 * Dumps docs for the statistics cleaner.
 */
void CleanStatisticsPass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix, R"(
    This pass just discards any statistics that previous passes might have
    attached to the kernel and program. It is inserted automatically after every
    normal pass that does not have statistics reporting enabled.
    )");
}

/**
 * Constructs a statistics cleaner.
 */
CleanStatisticsPass::CleanStatisticsPass(
    const utils::Ptr<const pmgr::Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::ProgramAnalysis(pass_factory, instance_name, type_name) {
}

/**
 * Runs the statistics cleaner.
 */
utils::Int CleanStatisticsPass::run(
    const ir::ProgramRef &program,
    const pmgr::pass_types::Context &context
) const {
    for (const auto &kernel : program->kernels) {
        AdditionalStats::pop(kernel);
    }
    AdditionalStats::pop(program);

    return 0;
}

} // namespace clean
} // namespace statistics
} // namespace ana
} // namespace pass
} // namespace ql
