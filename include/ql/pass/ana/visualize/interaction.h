/** \file
 * Defines the interaction graph visualization pass.
 */

#pragma once

#include "ql/pmgr/pass_types/specializations.h"

namespace ql {
namespace pass {
namespace ana {
namespace visualize {
namespace interaction {

/**
 * Interaction graph visualizer pass.
 */
class VisualizeInteractionPass : public pmgr::pass_types::Analysis {
private:
    static bool is_pass_registered;
    
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
     * Returns a user-friendly type name for this pass.
     */
    utils::Str get_friendly_type() const override;

    /**
     * Constructs a interaction graph visualizer pass.
     */
    VisualizeInteractionPass(
        const utils::Ptr<const pmgr::Factory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Runs the interaction graph visualizer.
     */
    utils::Int run(
        const ir::Ref &ir,
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
