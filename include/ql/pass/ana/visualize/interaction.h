/** \file
 * Defines the interaction graph visualization pass.
 */

#pragma once

#include "ql/pmgr/pass_types.h"

namespace ql {
namespace pass {
namespace ana {
namespace visualize {
namespace interaction {

/**
 * Interaction graph visualizer pass.
 */
class VisualizeInteractionPass : public pmgr::pass_types::ProgramAnalysis {
protected:

    /**
     * Dumps docs for the interaction graph visualizer.
     */
    void dump_docs(
        std::ostream &os,
        const utils::Str &line_prefix
    ) const override;

public:

    /**
     * Constructs a interaction graph visualizer pass.
     */
    VisualizeInteractionPass(
        const utils::Ptr<const pmgr::PassFactory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Runs the interaction graph visualizer.
     */
    utils::Int run(
        const ir::ProgramRef &program,
        const pmgr::pass_types::Context &context
    ) const override;

};

/**
 * Shorthand for referring to the pass using namespace notation.
 */
using Pass = VisualizeInteractionPass;

} // namespace interaction
} // namespace visualize
} // namespace ana
} // namespace pass
} // namespace ql
