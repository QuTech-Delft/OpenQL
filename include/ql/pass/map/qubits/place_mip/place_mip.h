/** \file
 * Defines the initial placement pass.
 */

#pragma once

#include "ql/com/options.h"
#include "ql/pmgr/pass_types/specializations.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace place_mip {

/**
 * Initial qubit placer pass.
 */
class PlaceQubitsPass : public pmgr::pass_types::Transformation {
    static bool is_pass_registered;

protected:

    /**
     * Dumps docs for the initial qubit placer.
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
     * Constructs an initial qubit placer.
     */
    PlaceQubitsPass(
        const utils::Ptr<const pmgr::Factory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Runs initial qubit placement.
     */
    utils::Int run(
        const ir::Ref &ir,
        const pmgr::pass_types::Context &context
    ) const override;

};

/**
 * Shorthand for referring to the pass using namespace notation.
 */
using Pass = PlaceQubitsPass;

} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
