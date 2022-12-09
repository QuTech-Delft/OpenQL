/** \file
 * Defines the Clifford optimizer pass.
 */

#pragma once

#include "ql/pmgr/pass_types/specializations.h"

namespace ql {
namespace pass {
namespace opt {
namespace clifford {
namespace optimize {

/**
 * Clifford sequence optimizer.
 * FIXME JvS: remove; only used by old pass manager
 */
void clifford_optimize(
    const ir::compat::ProgramRef &programp,
    const ir::compat::PlatformRef &platform,
    const utils::Str &passname
);

/**
 * Clifford optimizer pass.
 */
class CliffordOptimizePass : public pmgr::pass_types::KernelTransformation {
    static bool is_pass_registered;

protected:

    /**
     * Dumps docs for the Clifford optimizer.
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
     * Constructs a Clifford optimizer.
     */
    CliffordOptimizePass(
        const utils::Ptr<const pmgr::Factory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Runs the Clifford optimizer.
     */
    utils::Int run(
        const ir::compat::ProgramRef &program,
        const ir::compat::KernelRef &kernel,
        const pmgr::pass_types::Context &context
    ) const override;

};

/**
 * Shorthand for referring to the pass using namespace notation.
 */
using Pass = CliffordOptimizePass;

} // namespace optimize
} // namespace clifford
} // namespace opt
} // namespace pass
} // namespace ql
