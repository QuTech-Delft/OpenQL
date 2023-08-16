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
 * Returns a list of platform variants for this architecture. Variants may
 * be specified by the user by adding a dot-separated suffix to the
 * "eqasm_compiler" key or architecture namespace. If specified, the variant
 * must match a variant from this list. If not specified, the first variant
 * returned by this function serves as the default value.
 */
utils::List<utils::Str> InfoBase::get_variant_names() const {
    return {"default"};
}

/**
 * Writes documentation for a particular variant of this architecture to the
 * given output stream.
 */
void InfoBase::dump_variant_docs(
    const utils::Str &variant,
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    if (variant == "default") {
        utils::dump_str(os, line_prefix, R"(
        This architecture does not have multiple variants.
        )");
    }
    QL_ASSERT(false);
}

/**
 * Preprocessing logic for the platform JSON configuration file. May be used
 * to generate/expand certain things that are always the same for that
 * platform, to save typing in the configuration file (and reduce the amount
 * of mistakes made).
 */
void InfoBase::preprocess_platform(
    utils::Json &,
    const utils::Str &
) const {
}

/**
 * Post-processing logic for the Platform data structure. This may for
 * instance add annotations with architecture-specific configuration data.
 */
void InfoBase::post_process_platform(
    const ir::compat::PlatformRef &,
    const utils::Str &
) const {
}

/**
 * Adds the default "backend passes" for this platform. Called by
 * pmgr::Manager::from_defaults() when no compiler configuration file is
 * specified. This typically includes at least the architecture-specific
 * code generation pass, but anything after prescheduling and optimization
 * is considered a backend pass.
 */
void InfoBase::populate_backend_passes(pmgr::Manager &, const utils::Str &) const {
}

} // namespace arch
} // namespace ql
