/** \file
 * Defines the Clifford optimizer pass.
 */

#pragma once

#include "ql/pmgr/pass_types.h"

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
    const ir::ProgramRef &programp,
    const plat::PlatformRef &platform,
    const utils::Str &passname
);

/**
 * Clifford optimizer pass.
 */
class CliffordOptimizePass : public pmgr::pass_types::KernelTransformation {
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
     * Constructs a Clifford optimizer.
     */
    CliffordOptimizePass(
        const utils::Ptr<const pmgr::PassFactory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Runs the Clifford optimizer.
     */
    utils::Int run(
        const ir::ProgramRef &program,
        const ir::KernelRef &kernel,
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
