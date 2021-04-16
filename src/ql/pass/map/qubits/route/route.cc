/** \file
 * Defines the qubit router pass.
 */

#include "ql/pass/map/qubits/route/route.h"

#include "detail/mapper.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace route {

/**
 * Dumps docs for the qubit router.
 */
void RouteQubitsPass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix, R"(
    This pass heuristically satisfies the qubit connectivity constraints of the
    platform by inserting swap and/or move gates when a gate operating on two
    non-connected qubits is found.

    NOTE: this pass currently operates purely on a per-kernel basis. Because it
    may adjust the qubit mapping from input to output, a program consisting of
    multiple kernels that maintains a quantum state between the kernels may be
    silently destroyed.
    )");
}

/**
 * Constructs a qubit router.
 */
RouteQubitsPass::RouteQubitsPass(
    const utils::Ptr<const pmgr::PassFactory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::KernelTransformation(pass_factory, instance_name, type_name) {
    options.add_bool(
        "commute_multi_qubit",
        "Whether to consider commutation rules for the CZ and CNOT quantum "
        "gates.",
        false
    );
    options.add_bool(
        "commute_single_qubit",
        "Whether to consider commutation rules for single-qubit X and Z "
        "rotations.",
        false
    );

}

/**
 * Runs the qubit router.
 */
utils::Int RouteQubitsPass::run(
    const ir::ProgramRef &program,
    const ir::KernelRef &kernel,
    const pmgr::pass_types::Context &context
) const {

    return 0;
}

} // namespace route
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
