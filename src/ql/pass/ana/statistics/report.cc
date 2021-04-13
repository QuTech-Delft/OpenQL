/** \file
 * Defines the statistics reporting pass.
 */

#include "ql/pass/ana/statistics/report.h"

#include "ql/utils/filesystem.h"
#include "ql/pass/ana/statistics/common.h"

namespace ql {
namespace pass {
namespace ana {
namespace statistics {
namespace report {

/**
 * Dumps docs for the statistics reporter.
 */
void ReportStatisticsPass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix, R"(
    This pass reports some basic statistics of the program and each kernel to a
    report file. Some passes may also attach additional pass-specific statistics
    to the program and kernels, in which case these are printed and subsequently
    discarded as well.
    )");
}

/**
 * Constructs a statistics reporter.
 */
ReportStatisticsPass::ReportStatisticsPass(
    const utils::Ptr<const pmgr::PassFactory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::ProgramAnalysis(pass_factory, instance_name, type_name) {
    options.add_str(
        "output_suffix",
        "Suffix to use for the output filename.",
        ".txt"
    );
    options.add_str(
        "line_prefix",
        "Historically, report files contain a \"# \" prefix before each line. You can "
        "use this option to emulate that behavior.",
        ""
    );
}

/**
 * Runs the statistics reporter.
 */
utils::Int ReportStatisticsPass::run(
    const ir::ProgramRef &program,
    const pmgr::pass_types::Context &context
) const {
    auto line_prefix = options["line_prefix"].as_str();
    auto filename = context.output_prefix + options["output_suffix"].as_str();

    utils::OutFile file{filename};
    for (const auto &kernel : program->kernels) {
        dump(kernel, file.unwrap(), line_prefix);
    }
    dump(program, file.unwrap(), line_prefix);

    return 0;
}

} // namespace report
} // namespace statistics
} // namespace ana
} // namespace pass
} // namespace ql
