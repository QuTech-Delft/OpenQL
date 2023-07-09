/** \file
 * Defines the initial placement pass.
 */

#include "ql/pass/map/qubits/place_mip/place_mip.h"

#include "detail/impl.h"
#include "ql/ir/ir_gen_ex.h"
#include "ql/pass/ana/statistics/annotations.h"
#include "ql/pmgr/factory.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace place_mip {

bool PlaceQubitsPass::is_pass_registered = pmgr::Factory::register_pass<PlaceQubitsPass>("map.qubits.PlaceMIP");

/**
 * Dumps docs for the initial qubit placer.
 */
void PlaceQubitsPass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix, R"asdf(
    This step attempts to find a single mapping of the virtual qubits of a
    circuit to the real qubits of the platform's qubit topology that minimizes
    the sum of the distances between the two mapped operands of all
    two-qubit gates in the circuit. The distance between two real qubits is
    the minimum number of swaps that is required to move the state of one of
    the two qubits to the other. It employs a Mixed Integer Linear Programming
    (MIP) algorithm to solve this, modelled as a Quadratic Assignment Problem.
    Because the time-complexity of the MIP solving is exponential with respect to
    the number of pairs of 2 virtual qubits that interact, this may take quite some computer time.
    That is why a timeout option is provided that controls how long the solving can take.

    This initial mapping program is modelled as a Quadratic Assignment Problem by Lingling Lao in
    her 2018 mapping paper "Mapping of lattice surgery-based quantum circuits on surface code architectures":

    variables:
        forall i: forall k: x[i][k], x[i][k] is integral and 0 or 1, meaning facility i is in location k
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

    This model is resolved with the HiGHS solver (see www.highs.dev), licensed under the MIT license.

    Since solving might take some time, two options are offered to deal with this (and
    these can be combined):
    - the initial placement "horizon" may be used to limit the number of
    different two-qubit gates considered by the solver to the first N for each kernel;
    - a timeout may be specified.

    )asdf");
}

/**
 * Returns a user-friendly type name for this pass.
 */
utils::Str PlaceQubitsPass::get_friendly_type() const {
    return "MIP-based initial placer";
}

/**
 * Constructs an initial qubit placer.
 */
PlaceQubitsPass::PlaceQubitsPass(
    const utils::Ptr<const pmgr::Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::Transformation(pass_factory, instance_name, type_name) {
    options.add_int(
        "horizon",
        "When specified, the placement algorithm will only consider the "
        "connectivity required to perform the first N most frequent two-qubit gate types. "
        "When not specified or zero, all two-qubit gate types are "
        "considered.",
        "0", 0, utils::MAX
    );
    options.add_real(
        "timeout",
        "A float duration in seconds after which the MIP problem resolution by HiGHS will terminate. "
        "If this happens, the pass will emit a warning and will not update the qubit indices. "
        "When set to 0, there is no time limit.",
        "0.", 0., utils::INF
    );
    options.add_bool(
        "write_model_to_file",
        "Whether to write the model as MPS format to a file, as well as a pixel array representing the "
        "matrice of the linear optimization problem.",
        true
    );
    options.add_str(
        "model_filename",
        "Filename where to write the MPS optimization model.",
        "highs_model.mps"
    );
    options.add_bool(
        "fail_on_timeout",
        "Whether to exit compilation with an error when the MIP solving times out.",
        true
    );
}

class ReferenceUpdater : public ir::RecursiveVisitor {
public:
    ReferenceUpdater(ir::Ref aIr, const utils::Vec<utils::UInt> &aMapping) : ir(aIr), mapping(aMapping) {}

    void visit_node(ir::Node &node) override {}

    void visit_reference(ir::Reference &ref) override {
        if (ref.target == ir->platform->qubits &&
            ref.data_type == ir->platform->qubits->data_type) {
            QL_ASSERT(ref.indices.size() == 1);
            ref.indices[0].as<ir::IntLiteral>()->value = mapping[ref.indices[0].as<ir::IntLiteral>()->value];
        }
    }

private:
    ir::Ref ir;
    const utils::Vec<utils::UInt> &mapping;
};

/**
 * Runs initial qubit placement.
 */
utils::Int PlaceQubitsPass::run(
    const ir::Ref &ir,
    const pmgr::pass_types::Context &context
) const {
    detail::Options opts;
    opts.timeout = options["timeout"].as_real();
    opts.horizon = options["horizon"].as_uint();
    opts.write_model_to_file = options["write_model_to_file"].as_bool();
    opts.model_filename = options["model_filename"].as_str();
    opts.fail_on_timeout = options["fail_on_timeout"].as_bool();

    auto qubit_count = ir->platform->qubits->shape[0];
    
    utils::Vec<utils::UInt> mapping(qubit_count, detail::UNDEFINED_QUBIT);
    auto result = detail::performInitialPlacement(ir, opts, mapping);

    if (result == detail::Result::TIMED_OUT && opts.fail_on_timeout) {
        QL_FATAL("Initial placement pass timed out. You can disable this error by setting pass option fail_on_timeout to false.");
        QL_ASSERT(false);
    }

    if (result == detail::Result::NEW_MAP) {
        ReferenceUpdater referenceUpdater(ir, mapping);
        ir->program->visit(referenceUpdater);
    }

    return 0;
}

} // namespace place
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
