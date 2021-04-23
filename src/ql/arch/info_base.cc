/** \file
 * Base class for common architecture-specific logic.
 */

#include "ql/arch/info_base.h"

namespace ql {
namespace arch {

/**
 * Returns a list of strings accepted for the "eqasm_compiler" key in the
 * platform configuration file. This can be more than one, to support both
 * legacy (inconsistent) names and the new namespace names. The returned
 * set must include at least the name of the namespace.
 */
utils::List<utils::Str> InfoBase::get_eqasm_compiler_names() const {
    return {get_namespace_name()};
}

/**
 * Preprocessing logic for the platform JSON configuration file. May be used
 * to generate/expand certain things that are always the same for that
 * platform, to save typing in the configuration file (and reduce the amount
 * of mistakes made).
 */
void InfoBase::preprocess_platform(utils::Json &data) const {
}

/**
 * Adds the default "backend passes" for this platform. Called by
 * pmgr::Manager::from_defaults() when no compiler configuration file is
 * specified. This typically includes at least the architecture-specific
 * code generation pass, but anything after prescheduling and optimization
 * is considered a backend pass.
 */
void InfoBase::populate_backend_passes(pmgr::Manager &manager) const {
}

} // namespace arch
} // namespace ql
