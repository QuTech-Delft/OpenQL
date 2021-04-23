/** \file
 * Defines information about the CC architecture.
 */

#pragma once

#include <iostream>
#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/set.h"
#include "ql/utils/ptr.h"
#include "ql/utils/json.h"
#include "ql/pmgr/manager.h"
#include "ql/arch/info_base.h"

namespace ql {
namespace arch {
namespace cc {

/**
 * Architecture-specific information class for CC.
 */
class Info : public InfoBase {
public:

    /**
     * Writes the documentation for this architecture to the given output
     * stream.
     */
    void dump_docs(std::ostream &os, const utils::Str &line_prefix) const override;

    /**
     * Returns a user-friendly type name for this pass. Used for documentation
     * generation.
     */
    utils::Str get_friendly_name() const override;

    /**
     * Returns the name of the namespace for this architecture.
     */
    utils::Str get_namespace_name() const override;

    /**
     * Returns a list of strings accepted for the "eqasm_compiler" key in the
     * platform configuration file. This can be more than one, to support both
     * legacy (inconsistent) names and the new namespace names. The returned
     * set must include at least the name of the namespace.
     */
    utils::List<utils::Str> get_eqasm_compiler_names() const override;

    /**
     * Should generate a sane default platform JSON file. This JSON data will
     * still be preprocessed by preprocess_platform().
     */
    utils::Str get_default_platform() const override;

    /**
     * Preprocessing logic for the platform JSON configuration file. May be used
     * to generate/expand certain things that are always the same for that
     * platform, to save typing in the configuration file (and reduce the amount
     * of mistakes made).
     */
    void preprocess_platform(utils::Json &data) const override;

    /**
     * Adds the default "backend passes" for this platform. Called by
     * pmgr::Manager::from_defaults() when no compiler configuration file is
     * specified. This typically includes at least the architecture-specific
     * code generation pass, but anything after prescheduling and optimization
     * is considered a backend pass.
     */
    void populate_backend_passes(pmgr::Manager &manager) const override;

};

} // namespace cc
} // namespace arch
} // namespace ql
