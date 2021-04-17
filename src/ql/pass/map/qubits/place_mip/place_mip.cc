/** \file
 * Defines the initial placement pass.
 */

#include "ql/pass/map/qubits/place_mip/place_mip.h"

#include "ql/pass/ana/statistics/annotations.h"
#include "detail/algorithm.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace place_mip {

/**
 * Dumps docs for the initial qubit placer.
 */
void PlaceQubitsPass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
#ifdef INITIALPLACE
    utils::dump_str(os, line_prefix, R"asdf(
    This pass tries to map the two-qubit gate interaction graph onto the target
    device topology without adding swaps or moves by means of an exhaustive
    search.

    NOTE: this pass currently operates purely on a per-kernel basis. Because it
    may adjust the qubit mapping of each kernel, a program consisting of
    multiple kernels that maintains a quantum state between the kernels may be
    silently destroyed.

    The program is modelled as a Quadratic Assignment Problem by Lingling Lao in
    her mapping paper:

    variables:
        forall i: forall k: x[i][k], x[i][k] is integral and 0 or 1, meaning qubit i is in location k
    objective:
        min z = sum i: sum j: sum k: sum l: refcount[i][j] * distance(k,l) * x[i][k] * x[j][l]
    subject to:
        forall k: ( sum i: x[i][k] <= 1 )        allow more locations than qubits
        forall i: ( sum k: x[i][k] == 1 )        but each qubit must have one locations

    the article "An algorithm for the quadratic assignment problem using
    Benders' decomposition" by L. Kaufman and F. Broeckx, transforms this
    problem by introducing w[i][k] as follows:

    forall i: forall k: w[i][k] =  x[i][k] * ( sum j: sum l: refcount[i][j] * distance(k,l) * x[j][l] )

    to the following mixed integer linear problem:

    precompute:
        forall i: forall k: costmax[i][k] = sum j: sum l: refcount[i][j] * distance(k,l)
        (note: each of these costmax[][] is >= 0, so the "max(this,0)" around this is not needed)
    variables:
        forall i: forall k: x[i][k], x[i][k] is integral and 0 or 1
        forall i: forall k: w[i][k], w[i][k] is real and >= 0
    objective:
        min z = sum i: sum k: w[i][k]
    subject to:
        forall k: ( sum i: x[i][k] <= 1 )
        forall i: ( sum k: x[i][k] == 1 )
        forall i: forall k: costmax[i][k] * x[i][k]
            + ( sum j: sum l: refcount[i][j]*distance(k,l)*x[j][l] ) - w[i][k] <= costmax[i][k]
    )asdf");
#else
    utils::dump_str(os, line_prefix, R"(
    This pass was disabled due to configuration options when building the
    compiler. Therefore, this pass will simply fail when run. If you compiled
    OpenQL yourself and intend to use this pass, you're probably missing GLPK.
    )");
#endif
}

/**
 * Constructs an initial qubit placer.
 */
PlaceQubitsPass::PlaceQubitsPass(
    const utils::Ptr<const pmgr::PassFactory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::KernelTransformation(pass_factory, instance_name, type_name) {
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
#ifdef INITIALPLACE

    // Parse the options.
    // NOTE: timeout is not mapped to a user-visible option because the
    //  implementation is broken.
    detail::Options opts;
    opts.timeout = 0.0;
    opts.horizon = options["horizon"].as_uint();
    opts.map_all = true;

    // Run the algorithm.
    com::QubitMapping mapping(kernel->platform->qubit_count, true);
    detail::Algorithm algorithm;
    auto result = algorithm.run(kernel, opts, mapping);

    // Save the results as statistics.
    ana::statistics::AdditionalStats::push(
        kernel,
        context.full_pass_name + " result: " + utils::to_string(result)
    );
    ana::statistics::AdditionalStats::push(
        kernel,
        context.full_pass_name + " time taken: " + utils::to_string(algorithm.get_time_taken()) + "s"
    );

    // If the algorithm gave us a new mapping, apply it to all gates in the
    // kernel.
    if (result == detail::Result::NEW_MAP) {
        for (const auto &gate : kernel->c) {
            for (auto &qubit : gate->operands) {
                qubit = mapping[qubit];
            }
        }
        ana::statistics::AdditionalStats::push(
            kernel,
            context.full_pass_name + " mapping applied: " + utils::to_string(mapping.get_virt_to_real())
        );
    }

    return 0;

#else

    throw utils::Exception(
        "The " + get_type() + " pass type was disabled due to configuration " +
        "options when building the compiler. If you compiled OpenQL yourself, " +
        "you're probably missing GLPK."
    );

#endif
}

} // namespace place
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
