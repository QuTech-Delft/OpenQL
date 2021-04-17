/** \file
 * Defines the initial placement pass.
 */

#include "ql/pass/map/qubits/place/place.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace place {

/**
 * Dumps docs for the initial qubit placer.
 */
void PlaceQubitsPass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix, R"(
    This pass tries to map the two-qubit gate interaction graph onto the target
    device topology without adding swaps or moves by means of an exhaustive
    search.

    NOTE: this pass currently operates purely on a per-kernel basis. Because it
    may adjust the qubit mapping of each kernel, a program consisting of
    multiple kernels that maintains a quantum state between the kernels may be
    silently destroyed.
    )");
}

/**
 * Constructs an initial qubit placer.
 */
PlaceQubitsPass::PlaceQubitsPass(
    const utils::Ptr<const pmgr::PassFactory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::KernelTransformation(pass_factory, instance_name, type_name) {
    options.add_str(
        "timeout",
        "Timeout for the initial placement algorithm. When specified, this must "
        "be a floating-point number, optionally followed by an 's', 'm', or 'h' "
        "to specify the unit, or \"no\". If no unit is specified, seconds are "
        "assumed. \"no\" or zero means no time limit. The time is per kernel.",
        "no"
    );
    options.add_bool(
        "fail_on_timeout",
        "This sets the behavior when the timeout expires: either the pass "
        "makes no changes to the program but processing continues (useful when "
        "it is followed by the heuristic router), or an exception is thrown "
        "to indicate failure.",
        false
    );
    options.add_int(
        "horizon",
        "When specified, the placement algorithm will only consider the "
        "connectivity required to perform the first N two-qubit gates of each "
        "kernel. When not specified or zero, all two-qubit gates are "
        "considered.",
        "0", 0, 100
    );


}

/**
 * Runs initial qubit placement.
 */
utils::Int PlaceQubitsPass::run(
    const ir::ProgramRef &program,
    const ir::KernelRef &kernel,
    const pmgr::pass_types::Context &context
) const {

    return 0;
}

} // namespace place
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
