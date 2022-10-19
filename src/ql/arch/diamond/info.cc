/** \file
 * Defines information about the diamond architecture.
 */

#include "ql/arch/diamond/info.h"

#include "ql/arch/diamond/resources/hwconf_default.inc"
#include "ql/arch/factory.h"

namespace ql {
namespace arch {
namespace diamond {

bool Info::is_architecture_registered = Factory::register_architecture<Info>();

/**
 * Writes the documentation for this architecture to the given output
 * stream.
 */
void Info::dump_docs(std::ostream &os, const utils::Str &line_prefix) const {
    utils::dump_str(os, line_prefix, R"(
    This architecture is aimed towards computing with qubits made in color
    centers in diamond. It is part of the Fujitsu project and is a work in
    progress. The backend will, for now as it is in its early stages, work as a
    translation tool from a high-level algorithm to our own defined microcode.
    It is mostly a proof of concept at this time.
    )");
}

/**
 * Returns a user-friendly type name for this architecture. Used for
 * documentation generation.
 */
utils::Str Info::get_friendly_name() const {
    return "Diamond";
}

/**
 * Returns the name of the namespace for this architecture.
 */
utils::Str Info::get_namespace_name() const {
    return "diamond";
}

/**
 * Returns a list of strings accepted for the "eqasm_compiler" key in the
 * platform configuration file. This can be more than one, to support both
 * legacy (inconsistent) names and the new namespace names. The returned
 * set must include at least the name of the namespace.
 */
utils::List<utils::Str> Info::get_eqasm_compiler_names() const {
    return {"diamond"};
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

void Info::populate_backend_passes(pmgr::Manager &manager, const utils::Str &variant) const {

    // Add the microcode generator pass
    manager.append_pass("arch.diamond.gen.Microcode", "diamond_codegen");
}

} // namespace diamond
} // namespace arch
} // namespace ql
