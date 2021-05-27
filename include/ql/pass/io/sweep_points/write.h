/** \file
 * Defines the sweep point writer pass.
 */

#pragma once

#include "ql/pmgr/pass_types/specializations.h"

namespace ql {
namespace pass {
namespace io {
namespace sweep_points {
namespace write {

/**
 * Sweep point writer pass.
 */
class WriteSweepPointsPass : public pmgr::pass_types::ProgramAnalysis {
protected:

    /**
     * Dumps docs for the sweep point writer.
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
     * Constructs a sweep point writer.
     */
    WriteSweepPointsPass(
        const utils::Ptr<const pmgr::Factory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Runs the sweep point writer.
     */
    utils::Int run(
        const ir::ProgramRef &program,
        const pmgr::pass_types::Context &context
    ) const override;

};

/**
 * Shorthand for referring to the pass using namespace notation.
 */
using Pass = WriteSweepPointsPass;

} // namespace write
} // namespace sweep_points
} // namespace io
} // namespace pass
} // namespace ql
