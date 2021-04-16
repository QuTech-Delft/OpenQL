/** \file
 * Defines the scheduler pass.
 */

#pragma once

#include "ql/com/options.h"
#include "ql/pmgr/pass_types.h"

namespace ql {
namespace pass {
namespace sch {
namespace schedule {

/**
 * Main entry point of the non-resource-constrained scheduler.
 * FIXME JvS: remove; only used by old pass manager
 */
void schedule(
    const ir::ProgramRef &program,
    const plat::PlatformRef &platform,
    const utils::Str &passname
);

/**
 * Main entry point of the resource-constrained scheduler.
 * FIXME JvS: remove; only used by old pass manager
 */
void rcschedule(
    const ir::ProgramRef &program,
    const plat::PlatformRef &platform,
    const utils::Str &passname
);

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
     * Constructs a scheduler.
     */
    SchedulePass(
        const utils::Ptr<const pmgr::PassFactory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Runs the scheduler.
     */
    utils::Int run(
        const ir::ProgramRef &program,
        const ir::KernelRef &kernel,
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
