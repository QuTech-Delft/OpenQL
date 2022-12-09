/** \file
 * Defines the list scheduler pass.
 */

#pragma once

#include "ql/pmgr/pass_types/specializations.h"

namespace ql {
namespace pass {
namespace sch {
namespace list_schedule {

/**
 * Scheduler pass.
 */
class ListSchedulePass : public pmgr::pass_types::Transformation {
    static bool is_pass_registered;

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
    ListSchedulePass(
        const utils::Ptr<const pmgr::Factory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

private:

    /**
     * Runs the scheduler on the given block.
     */
    static void run_on_block(
        const ir::Ref &ir,
        const ir::BlockBaseRef &block,
        const utils::Str &name_path,
        utils::Set<utils::Str> &used_names,
        const pmgr::pass_types::Context &context
    );

public:

    /**
     * Runs the scheduler.
     */
    utils::Int run(
        const ir::Ref &ir,
        const pmgr::pass_types::Context &context
    ) const override;

};

/**
 * Shorthand for referring to the pass using namespace notation.
 */
using Pass = ListSchedulePass;

} // namespace list_schedule
} // namespace sch
} // namespace pass
} // namespace ql
