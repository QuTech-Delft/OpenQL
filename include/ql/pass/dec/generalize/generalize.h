/** \file
 * Instruction generalization pass.
 */

#pragma once

#include "ql/com/dec/rules.h"
#include "ql/pmgr/pass_types/specializations.h"

namespace ql {
namespace pass {
namespace dec {
namespace generalize {

/**
 * Instruction generalization pass.
 */
class GeneralizeInstructionsPass : public pmgr::pass_types::Transformation {
    static bool is_pass_registered;

protected:

    /**
     * Dumps docs for the instruction generalizer.
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
     * Constructs an instruction generalizer.
     */
    GeneralizeInstructionsPass(
        const utils::Ptr<const pmgr::Factory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

private:

    /**
     * Runs the instruction generalizer on the given block.
     */
    static void run_on_block(
        const ir::Ref &ir,
        const ir::BlockBaseRef &block
    );

public:

    /**
     * Runs the instruction generalizer.
     */
    utils::Int run(
        const ir::Ref &ir,
        const pmgr::pass_types::Context &context
    ) const override;

};

/**
 * Shorthand for referring to the pass using namespace notation.
 */
using Pass = GeneralizeInstructionsPass;

} // namespace generalize
} // namespace dec
} // namespace pass
} // namespace ql
