/** \file
 * Structure decomposition pass.
 */

#pragma once

#include "ql/com/dec/rules.h"
#include "ql/pmgr/pass_types/specializations.h"

namespace ql {
namespace pass {
namespace dec {
namespace structure {

/**
 * Structure decomposition pass.
 */
class DecomposeStructurePass : public pmgr::pass_types::Transformation {
    static bool is_pass_registered;
    
protected:

    /**
     * Dumps docs for the structure decomposer.
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
     * Constructs a structure decomposer.
     */
    DecomposeStructurePass(
        const utils::Ptr<const pmgr::Factory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Runs the structure decomposer.
     */
    utils::Int run(
        const ir::Ref &ir,
        const pmgr::pass_types::Context &context
    ) const override;

};

/**
 * Shorthand for referring to the pass using namespace notation.
 */
using Pass = DecomposeStructurePass;

} // namespace structure
} // namespace dec
} // namespace pass
} // namespace ql
