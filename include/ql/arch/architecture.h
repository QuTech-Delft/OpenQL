/** \file
 * Structure for retaining information about a particular variant of an
 * architecture.
 */

#pragma once

#include <iostream>
#include "ql/utils/str.h"
#include "ql/utils/ptr.h"
#include "ql/utils/json.h"
#include "ql/arch/declarations.h"
#include "ql/arch/info_base.h"
#include "ql/pmgr/declarations.h"

namespace ql {
namespace arch {

/**
 * Representation of some variant of some architecture family.
 */
class Architecture {
protected:
    friend class Factory;

    /**
     * Constructs an architecture. This should only be used by the factory.
     */
    Architecture(const CInfoRef &family, const utils::Str &variant);

public:

    /**
     * Information structure for the architecture family.
     */
    CInfoRef family;

    /**
     * Name of the particular architecture variant.
     */
    utils::Str variant;

    /**
     * Returns a user-friendly name for this architecture variant.
     */
    utils::Str get_friendly_name() const;

    /**
     * Generates JSON for the default configuration of this architecture
     * variant.
     */
    utils::Str get_default_platform() const;

    /**
     * Preprocesses/desugars the platform JSON data for this particular
     * architecture variant.
     */
    void preprocess_platform(utils::Json &data) const;

    /**
     * Adds the default "backend passes" for this platform. Called by
     * pmgr::Manager::from_defaults() when no compiler configuration file is
     * specified. This typically includes at least the architecture-specific
     * code generation pass, but anything after prescheduling and optimization
     * is considered a backend pass.
     */
    void populate_backend_passes(pmgr::Manager &manager) const;

};

} // namespace arch
} // namespace ql
