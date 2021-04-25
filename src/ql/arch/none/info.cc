/** \file
 * Defines information about the no-op architecture.
 */

#include "ql/arch/none/info.h"

#include "ql/arch/none/resources/hwconf_default.inc"

namespace ql {
namespace arch {
namespace none {

/**
 * Writes the documentation for this architecture to the given output
 * stream.
 */
void Info::dump_docs(std::ostream &os, const utils::Str &line_prefix) const {
    utils::dump_str(os, line_prefix, R"(
    This is just a dummy architecture that does not include any backend passes
    by default, does not provide shortcuts for any architecture-specific passes
    and resources, and does not do any platform-specific preprocessing on the
    platform configuration file. You can use it when you just want to try OpenQL
    out, or when your target is an architecture-agnostic simulator.

    The default configuration file consists of relatively sane defaults for
    simulating the resulting cQASM output with the QX simulator.
    )");
}

/**
 * Returns a user-friendly type name for this architecture. Used for
 * documentation generation.
 */
utils::Str Info::get_friendly_name() const {
    return "None";
}

/**
 * Returns the name of the namespace for this architecture.
 */
utils::Str Info::get_namespace_name() const {
    return "none";
}

/**
 * Returns a list of strings accepted for the "eqasm_compiler" key in the
 * platform configuration file. This can be more than one, to support both
 * legacy (inconsistent) names and the new namespace names. The returned
 * set must include at least the name of the namespace.
 */
utils::List<utils::Str> Info::get_eqasm_compiler_names() const {
    return {"none", "qx", ""};
}

/**
 * Should generate a sane default platform JSON file, for when the user
 * constructs a Platform without JSON data. This is done by specifying an
 * architecture namespace identifier instead of a JSON filename. Optionally,
 * the user may specify a variant suffix, separated using a dot, to select
 * a variation of the architecture; for instance, for CC-light, there might
 * be variations for surface-5, surface-7, and surface-17. This JSON data
 * will still be preprocessed by preprocess_platform().
 */
utils::Str Info::get_default_platform(const utils::Str &variant) const {

    // NOTE: based on tests/hardware_config_qx.json at the time of writing.
    return HWCONF_DEFAULT_DATA;

}

} // namespace none
} // namespace arch
} // namespace ql
