/** \file
 * Dead code elimination pass.
 */

#pragma once

#include "ql/pmgr/pass_types/specializations.h"

namespace ql {
namespace pass {
namespace opt {
namespace dead_code_elim {

/**
 * Dead code elimination pass.
 */
class DeadCodeEliminationPass : public pmgr::pass_types::Transformation {
public:

    /**
     * Constructs a dead code elimination pass.
     */
    DeadCodeEliminationPass(
        const utils::Ptr<const pmgr::Factory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Returns a user-friendly type name for this pass.
     */
    utils::Str get_friendly_type() const override;

    /**
     * Runs the dead code elimination pass.
     */
    utils::Int run(
        const ir::Ref &ir,
        const pmgr::pass_types::Context &context
    ) const override;

protected:

    /**
     * Dumps docs for dead code elimination pass.
     */
    void dump_docs(
        std::ostream &os,
        const utils::Str &line_prefix
    ) const override;

private:
    /**
     * Runs the dead code elimination pass on the given block.
     */
    static void run_on_block(
        const ir::BlockBaseRef &block,
        utils::UInt level = 0
    );

};

/**
 * Shorthand for referring to the pass using namespace notation.
 */
using Pass = DeadCodeEliminationPass;

} // namespace dead_code_elim
} // namespace opt
} // namespace pass
} // namespace ql
