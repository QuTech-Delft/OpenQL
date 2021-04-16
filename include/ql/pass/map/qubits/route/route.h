/** \file
 * Defines the mapper pass.
 */

#pragma once

#include "ql/com/options.h"
#include "ql/pmgr/pass_types.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace route {

/**
 * Qubit router pass.
 */
class RouteQubitsPass : public pmgr::pass_types::KernelTransformation {
protected:

    /**
     * Dumps docs for the qubit router.
     */
    void dump_docs(
        std::ostream &os,
        const utils::Str &line_prefix
    ) const override;

public:

    /**
     * Constructs a qubit router.
     */
    RouteQubitsPass(
        const utils::Ptr<const pmgr::PassFactory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Runs the qubit router.
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
using Pass = RouteQubitsPass;

} // namespace route
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
