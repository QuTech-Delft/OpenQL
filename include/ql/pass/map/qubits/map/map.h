/** \file
 * Defines the mapper pass.
 */

#pragma once

#include "ql/utils/ptr.h"
#include "ql/com/options.h"
#include "ql/pmgr/pass_types/specializations.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace map {

namespace detail {
struct Options;
} // namespace detail

/**
 * Qubit mapper pass.
 */
class MapQubitsPass : public pmgr::pass_types::KernelTransformation {
private:

    /**
     * Parsed options structure.
     */
    utils::Ptr<detail::Options> parsed_options;

protected:

    /**
     * Dumps docs for the qubit mapper.
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
     * Constructs a qubit mapper.
     */
    MapQubitsPass(
        const utils::Ptr<const pmgr::Factory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Builds the options structure for the mapper.
     */
    pmgr::pass_types::NodeType on_construct(
        const utils::Ptr<const pmgr::Factory> &factory,
        utils::List<pmgr::PassRef> &passes,
        pmgr::condition::Ref &condition
    ) override;

    /**
     * Runs the qubit mapper.
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
using Pass = MapQubitsPass;

} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
