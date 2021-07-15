/** \file
 * Defines the legacy (old-IR) list scheduler pass.
 */

#pragma once

#include "ql/com/options.h"
#include "ql/pmgr/pass_types/specializations.h"

namespace ql {
namespace pass {
namespace sch {
namespace schedule {

/**
 * Scheduler pass.
 */
class SchedulePass : public pmgr::pass_types::KernelTransformation {
protected:

    /**
     * Dumps docs for the scheduler.
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
     * Constructs a scheduler.
     */
    SchedulePass(
        const utils::Ptr<const pmgr::Factory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Runs the scheduler.
     */
    utils::Int run(
        const ir::compat::ProgramRef &program,
        const ir::compat::KernelRef &kernel,
        const pmgr::pass_types::Context &context
    ) const override;

};

/**
 * Shorthand for referring to the pass using namespace notation.
 */
using Pass = SchedulePass;

} // namespace schedule
} // namespace sch
} // namespace pass
} // namespace ql
