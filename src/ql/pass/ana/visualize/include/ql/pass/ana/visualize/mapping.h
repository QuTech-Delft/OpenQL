/** \file
 * Defines the mapping graph visualization pass.
 */

#pragma once

#include "ql/pmgr/pass_types/specializations.h"

namespace ql {
namespace pass {
namespace ana {
namespace visualize {
namespace mapping {

/**
 * Mapping graph visualizer pass.
 */
class VisualizeMappingPass : public pmgr::pass_types::ProgramAnalysis {
    static bool is_pass_registered;
    
protected:

    /**
     * Dumps docs for the mapping graph visualizer.
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
     * Constructs a mapping graph visualizer pass.
     */
    VisualizeMappingPass(
        const utils::Ptr<const pmgr::Factory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Runs the mapping graph visualizer.
     */
    utils::Int run(
        const ir::compat::ProgramRef &program,
        const pmgr::pass_types::Context &context
    ) const override;

};

/**
 * Shorthand for referring to the pass using namespace notation.
 */
using Pass = VisualizeMappingPass;

} // namespace mapping
} // namespace visualize
} // namespace ana
} // namespace pass
} // namespace ql
