/** \file
 * Structure for retaining information about a particular variant of an
 * architecture.
 */

#include "ql/arch/architecture.h"

namespace ql {
namespace arch {

/**
 * Constructs an architecture. This should only be used by the factory.
 */
Architecture::Architecture(
    const CInfoRef &family,
    const utils::Str &variant
) :
    family(family),
    variant(variant)
{ }

/**
 * Returns a user-friendly name for this architecture variant.
 */
utils::Str Architecture::get_friendly_name() const {
    return family->get_friendly_name() + " (" + variant + ")";
}

/**
 * Generates JSON for the default configuration of this architecture
 * variant.
 */
utils::Str Architecture::get_default_platform() const {
    return family->get_default_platform(variant);
}

/**
 * Preprocesses/desugars the platform JSON data for this particular
 * architecture variant.
 */
void Architecture::preprocess_platform(utils::Json &data) const {
    family->preprocess_platform(data, variant);
}

/**
 * Adds the default "backend passes" for this platform. Called by
 * pmgr::Manager::from_defaults() when no compiler configuration file is
 * specified. This typically includes at least the architecture-specific
 * code generation pass, but anything after prescheduling and optimization
 * is considered a backend pass.
 */
void Architecture::populate_backend_passes(pmgr::Manager &manager) const {
    family->populate_backend_passes(manager, variant);
}

} // namespace arch
} // namespace ql
