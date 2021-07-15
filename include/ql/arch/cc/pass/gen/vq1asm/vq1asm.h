/** \file
 * Defines the QuTech Central Controller Q1 processor assembly generator pass.
 */

#pragma once

#include "ql/com/options.h"
#include "ql/pmgr/pass_types/specializations.h"

namespace ql {
namespace arch {
namespace cc {
namespace pass {
namespace gen {
namespace vq1asm {

/**
 * QuTech Central Controller Q1 processor assembly generator pass.
 */
class GenerateVQ1AsmPass : public pmgr::pass_types::ProgramTransformation {
protected:

    /**
     * Dumps docs for the code generator.
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
     * Constructs a code generator.
     */
    GenerateVQ1AsmPass(
        const utils::Ptr<const pmgr::Factory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Runs the code generator.
     */
    utils::Int run(
        const ir::compat::ProgramRef &program,
        const pmgr::pass_types::Context &context
    ) const override;

};

/**
 * Shorthand for referring to the pass using namespace notation.
 */
using Pass = GenerateVQ1AsmPass;

} // namespace vq1asm
} // namespace gen
} // namespace pass
} // namespace cc
} // namespace arch
} // namespace ql
