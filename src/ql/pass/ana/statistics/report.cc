/** \file
 * Defines the statistics reporting pass.
 */

#include "ql/pass/ana/statistics/report.h"

#include "ql/utils/filesystem.h"
#include "ql/com/ana/metrics.h"
#include "ql/pmgr/factory.h"

namespace ql {
namespace pass {
namespace ana {
namespace statistics {
namespace report {

/**
 * Dumps basic statistics for the given kernel to the given output stream.
 */
void dump(
    const ir::Ref &ir,
    const ir::BlockRef &block,
    std::ostream &os,
    const utils::Str &line_prefix
) {
    using namespace com::ana;

    os << line_prefix << "Duration (assuming no control-flow): " << compute_block<Latency>(ir, block) << "\n";
    os << line_prefix << "Number of quantum gates: " << compute_block<QuantumGateCount>(ir, block) << "\n";
    os << line_prefix << "Number of multi-qubit gates: " << compute_block<MultiQubitGateCount>(ir, block) << "\n";
    os << line_prefix << "Number of classical operations: " << compute_block<ClassicalOperationCount>(ir, block) << "\n";
    os << line_prefix << "Number of qubits used: " << compute_block<QubitUsageCount>(ir, block).sparse_size() << "\n";
    os << line_prefix << "Qubit cycles use (assuming no control-flow): " << compute_block<QubitUsedCycleCount>(ir, block) << "\n";
    for (const auto &line : AdditionalStats::pop(block)) {
        os << line_prefix << "----- " << line << "\n";
    }
    os.flush();
}

/**
 * Dumps basic statistics for the given program to the given output stream. This
 * only dumps the global statistics, not the statistics for each individual
 * kernel.
 */
void dump(
    const ir::Ref &ir,
    const ir::ProgramRef &program,
    std::ostream &os,
    const utils::Str &line_prefix
) {
    using namespace com::ana;

    os << line_prefix << "Total duration (assuming no control-flow): " << compute_program<Latency>(ir) << "\n";
    os << line_prefix << "Total number of quantum gates: " << compute_program<QuantumGateCount>(ir) << "\n";
    os << line_prefix << "Total number of multi-qubit gates: " << compute_program<MultiQubitGateCount>(ir) << "\n";
    os << line_prefix << "Total number of classical operations: " << compute_program<ClassicalOperationCount>(ir) << "\n";
    os << line_prefix << "Number of qubits used: " << compute_program<QubitUsageCount>(ir).sparse_size() << "\n";
    os << line_prefix << "Qubit cycles use (assuming no control-flow): " << compute_program<QubitUsedCycleCount>(ir) << "\n";
    for (const auto &line : AdditionalStats::pop(program)) {
        os << line_prefix << line << "\n";
    }
    os.flush();
}

/**
 * Dumps statistics for the given program and its kernels to the given output
 * stream.
 */
void dump_all(
    const ir::Ref &ir,
    std::ostream &os,
    const utils::Str &line_prefix
) {
    if (ir->program.empty()) {
        os << line_prefix << "no program node to dump statistics for" << std::endl;
    } else {
        for (const auto &block : ir->program->blocks) {
            os << line_prefix << "For block with name \"" << block->name << "\":\n";
            dump(ir, block, os, line_prefix + "    ");
            os << "\n";
        }
        os << line_prefix << "Global statistics:\n";
        dump(ir, ir->program, os, line_prefix);
    }
}

bool ReportStatisticsPass::is_pass_registered = pmgr::Factory::register_pass<ReportStatisticsPass>("ana.statistics.Report");

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
 * Returns a user-friendly type name for this pass.
 */
utils::Str ReportStatisticsPass::get_friendly_type() const {
    return "Statistics reporter";
}

/**
 * Constructs a statistics reporter.
 */
ReportStatisticsPass::ReportStatisticsPass(
    const utils::Ptr<const pmgr::Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::Analysis(pass_factory, instance_name, type_name) {
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
    const ir::Ref &ir,
    const pmgr::pass_types::Context &context
) const {
    auto line_prefix = options["line_prefix"].as_str();
    auto filename = context.output_prefix + options["output_suffix"].as_str();
    dump_all(ir, utils::OutFile(filename).unwrap(), line_prefix);
    return 0;
}

} // namespace report
} // namespace statistics
} // namespace ana
} // namespace pass
} // namespace ql
