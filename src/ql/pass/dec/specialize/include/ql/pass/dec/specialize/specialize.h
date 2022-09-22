/** \file
 * Instruction specialization pass.
 */

#pragma once

#include "ql/com/dec/rules.h"
#include "ql/pmgr/pass_types/specializations.h"

namespace ql {
namespace pass {
namespace dec {
namespace specialize {

/**
 * Instruction specialization pass.
 */
class SpecializeInstructionsPass : public pmgr::pass_types::Transformation {
    static bool is_pass_registered;
    
protected:

    /**
     * Dumps docs for the instruction specializer.
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
     * Constructs an instruction specializer.
     */
    SpecializeInstructionsPass(
        const utils::Ptr<const pmgr::Factory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

private:

    /**
     * Runs the instruction specializer on the given block.
     */
    static void run_on_block(
        const ir::Ref &ir,
        const ir::BlockBaseRef &block
    );

public:

    /**
     * Runs the instruction specializer.
     */
    utils::Int run(
        const ir::Ref &ir,
        const pmgr::pass_types::Context &context
    ) const override;

};

/**
 * Shorthand for referring to the pass using namespace notation.
 */
using Pass = SpecializeInstructionsPass;

} // namespace specialize
} // namespace dec
} // namespace pass
} // namespace ql
