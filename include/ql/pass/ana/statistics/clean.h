/** \file
 * Defines the statistics cleaning pass.
 */

#pragma once

#include "ql/pmgr/pass_types/specializations.h"
#include "ql/pass/ana/statistics/annotations.h"

namespace ql {
namespace pass {
namespace ana {
namespace statistics {
namespace clean {

/**
 * Statistics cleaning pass.
 */
class CleanStatisticsPass : public pmgr::pass_types::ProgramAnalysis {
protected:

    /**
     * Dumps docs for the statistics cleaner.
     */
    void dump_docs(
        std::ostream &os,
        const utils::Str &line_prefix
    ) const override;

public:

    /**
     * Returns a user-friendly type name for this pass.
     */
    utils::Str get_friendly_type() const override;

    /**
     * Constructs a statistics cleaner.
     */
    CleanStatisticsPass(
        const utils::Ptr<const pmgr::Factory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Runs the statistics cleaner.
     */
    utils::Int run(
        const ir::ProgramRef &program,
        const pmgr::pass_types::Context &context
    ) const override;

};

/**
 * Shorthand for referring to the pass using namespace notation.
 */
using Pass = CleanStatisticsPass;

} // namespace clean
} // namespace statistics
} // namespace ana
} // namespace pass
} // namespace ql
