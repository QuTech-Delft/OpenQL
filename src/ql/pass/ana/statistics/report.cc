/** \file
 * Defines the statistics reporting pass.
 */

#include "ql/pass/ana/statistics/report.h"

#include "ql/utils/filesystem.h"
#include "ql/com/metrics.h"

namespace ql {
namespace pass {
namespace ana {
namespace statistics {
namespace report {

/**
 * Dumps basic statistics for the given kernel to the given output stream.
 */
void dump(
    const ir::compat::KernelRef &kernel,
    std::ostream &os,
    const utils::Str &line_prefix
) {
    using namespace com::metrics;

    os << line_prefix << "kernel: " << kernel->name << "\n";
    os << line_prefix << "----- circuit_latency: " << compute<Latency>(kernel) << "\n";
    os << line_prefix << "----- quantum gates: " << compute<QuantumGateCount>(kernel) << "\n";
    os << line_prefix << "----- non single qubit gates: " << compute<MultiQubitGateCount>(kernel) << "\n";
    os << line_prefix << "----- classical operations: " << compute<ClassicalOperationCount>(kernel) << "\n";
    os << line_prefix << "----- qubits used: " << compute<QubitUsageCount>(kernel).sparse_size() << "\n";
    os << line_prefix << "----- qubit cycles use:" << compute<QubitUsedCycleCount>(kernel) << "\n";
    for (const auto &line : AdditionalStats::pop(kernel)) {
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
    const ir::compat::ProgramRef &program,
    std::ostream &os,
    const utils::Str &line_prefix
) {
    using namespace com::metrics;

    os << line_prefix << "Total circuit_latency: " << compute<Latency>(program) << "\n";
    os << line_prefix << "Total no. of quantum gates: " << compute<QuantumGateCount>(program) << "\n";
    os << line_prefix << "Total no. of non single qubit gates: " << compute<MultiQubitGateCount>(program) << "\n";
    os << line_prefix << "Total no. of classical operations: " << compute<ClassicalOperationCount>(program) << "\n";
    os << line_prefix << "Qubits used: " << compute<QubitUsageCount>(program).sparse_size() << "\n";
    os << line_prefix << "No. kernels: " << compute<QubitUsedCycleCount>(program) << "\n";
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
    const ir::compat::ProgramRef &program,
    std::ostream &os,
    const utils::Str &line_prefix
) {
    for (const auto &kernel : program->kernels) {
        dump(kernel, os, line_prefix);
    }
    dump(program, os, line_prefix);
}

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
    const ir::compat::ProgramRef &program,
    const pmgr::pass_types::Context &context
) const {
    auto line_prefix = options["line_prefix"].as_str();
    auto filename = context.output_prefix + options["output_suffix"].as_str();
    dump_all(program, utils::OutFile(filename).unwrap(), line_prefix);
    return 0;
}

} // namespace report
} // namespace statistics
} // namespace ana
} // namespace pass
} // namespace ql
