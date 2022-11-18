/** \file
 * Defines the qubit router pass.
 */

#include "ql/pass/map/qubits/simple_map/simple_map.h"

#include "ql/pmgr/factory.h"
#include "detail/impl.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace simple_map {

bool SimpleMapQubitsPass::is_pass_registered = pmgr::Factory::register_pass<SimpleMapQubitsPass>("map.qubits.Map");

/**
 * Dumps docs for the qubit mapper.
 */
void SimpleMapQubitsPass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix, R"()");
}

/**
 * Returns a user-friendly type name for this pass.
 */
utils::Str SimpleMapQubitsPass::get_friendly_type() const {
    return "Simple mapper";
}

/**
 * Constructs a qubit mapper.
 */
SimpleMapQubitsPass::SimpleMapQubitsPass(
    const utils::Ptr<const pmgr::Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::Transformation(pass_factory, instance_name, type_name) {}

utils::Int SimpleMapQubitsPass::run(
    const ir::Ref &ir,
    const pmgr::pass_types::Context &context
) const {
    detail::simpleMap(ir);

    return 0;
}

} // namespace simple_map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
