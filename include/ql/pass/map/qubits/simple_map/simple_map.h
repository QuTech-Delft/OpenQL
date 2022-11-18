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
namespace simple_map {

class SimpleMapQubitsPass : public pmgr::pass_types::Transformation {
    static bool is_pass_registered;

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
    SimpleMapQubitsPass(
        const utils::Ptr<const pmgr::Factory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Runs the qubit mapper.
     */
    utils::Int run(
        const ir::Ref &program,
        const pmgr::pass_types::Context &context
    ) const override;

};

/**
 * Shorthand for referring to the pass using namespace notation.
 */
using Pass = SimpleMapQubitsPass;

} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
