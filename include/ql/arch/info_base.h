/** \file
 * Base class for common architecture-specific logic.
 */

#pragma once

#include <iostream>
#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/list.h"
#include "ql/utils/ptr.h"
#include "ql/utils/json.h"
#include "ql/pmgr/manager_decl.h"

namespace ql {
namespace arch {

/**
 * Base class for architecture information retrieval and some miscellaneous
 * architecture-specific logic.
 */
class InfoBase {
public:

    /**
     * Writes the documentation for this architecture to the given output
     * stream.
     */
    virtual void dump_docs(std::ostream &os, const utils::Str &line_prefix) const = 0;

    /**
     * Returns a user-friendly type name for this pass. Used for documentation
     * generation.
     */
    virtual utils::Str get_friendly_name() const = 0;

    /**
     * Returns the name of the namespace for this architecture.
     */
    virtual utils::Str get_namespace_name() const = 0;

    /**
     * Returns a list of strings accepted for the "eqasm_compiler" key in the
     * platform configuration file. This can be more than one, to support both
     * legacy (inconsistent) names and the new namespace names. The returned
     * set must include at least the name of the namespace.
     */
    virtual utils::List<utils::Str> get_eqasm_compiler_names() const;

    /**
     * Should generate a sane default platform JSON file. This JSON data will
     * still be preprocessed by preprocess_platform().
     */
    virtual utils::Str get_default_platform() const = 0;

    /**
     * Preprocessing logic for the platform JSON configuration file. May be used
     * to generate/expand certain things that are always the same for that
     * platform, to save typing in the configuration file (and reduce the amount
     * of mistakes made).
     */
    virtual void preprocess_platform(utils::Json &data) const;

    /**
     * Adds the default "backend passes" for this platform. Called by
     * pmgr::Manager::from_defaults() when no compiler configuration file is
     * specified. This typically includes at least the architecture-specific
     * code generation pass, but anything after prescheduling and optimization
     * is considered a backend pass.
     */
    virtual void populate_backend_passes(pmgr::Manager &manager) const;

};

/**
 * Shared pointer reference to an architecture information wrapper.
 */
using InfoRef = utils::Ptr<InfoBase>;

/**
 * Immutable shared pointer reference to an architecture information wrapper.
 */
using CInfoRef = utils::Ptr<const InfoBase>;

} // namespace arch
} // namespace ql
