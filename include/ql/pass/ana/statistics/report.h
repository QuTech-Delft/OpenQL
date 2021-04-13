/** \file
 * Defines the statistics reporting pass.
 */

#pragma once

#include "ql/pmgr/pass_types.h"
#include "ql/pass/ana/statistics/annotations.h"

namespace ql {
namespace pass {
namespace ana {
namespace statistics {
namespace report {

/**
 * Dumps basic statistics for the given kernel to the given output stream.
 */
void dump(
    const ir::KernelRef &kernel,
    std::ostream &os = std::cout,
    const utils::Str &line_prefix = ""
);

/**
 * Dumps basic statistics for the given program to the given output stream. This
 * only dumps the global statistics, not the statistics for each individual
 * kernel.
 */
void dump(
    const ir::ProgramRef &program,
    std::ostream &os = std::cout,
    const utils::Str &line_prefix = ""
);

/**
 * Dumps statistics for the given program and its kernels to the given output
 * stream.
 */
void dump_all(
    const ir::ProgramRef &program,
    std::ostream &os = std::cout,
    const utils::Str &line_prefix = ""
);

/**
 * Statistics reporting pass.
 */
class ReportStatisticsPass : public pmgr::pass_types::ProgramAnalysis {
protected:

    /**
     * Dumps docs for the statistics reporter.
     */
    void dump_docs(
        std::ostream &os,
        const utils::Str &line_prefix
    ) const override;

public:

    /**
     * Constructs a statistics reporter.
     */
    ReportStatisticsPass(
        const utils::Ptr<const pmgr::PassFactory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Runs the statistics reporter.
     */
    utils::Int run(
        const ir::ProgramRef &program,
        const pmgr::pass_types::Context &context
    ) const override;

};

/**
 * Shorthand for referring to the pass using namespace notation.
 */
using Pass = ReportStatisticsPass;

} // namespace report
} // namespace statistics
} // namespace ana
} // namespace pass
} // namespace ql
