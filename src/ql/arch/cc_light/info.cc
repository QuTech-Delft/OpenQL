/** \file
 * Defines information about the CC-light architecture.
 */

#include "ql/arch/cc_light/info.h"

#include "ql/com/options.h"
#include "ql/arch/cc_light/resources/hwconf_default.inc"
#include "ql/arch/cc_light/resources/hwconf_s5.inc"
#include "ql/arch/cc_light/resources/hwconf_s7.inc"
#include "ql/arch/cc_light/resources/hwconf_s17.inc"
#include "ql/arch/factory.h"

namespace ql {
namespace arch {
namespace cc_light {

bool Info::is_architecture_registered = Factory::register_architecture<Info>();

/**
 * Writes the documentation for this architecture to the given output
 * stream.
 */
void Info::dump_docs(std::ostream &os, const utils::Str &line_prefix) const {
    utils::dump_str(os, line_prefix, R"(
    This architecture represents what remains of the CC-light backend from past
    versions of OpenQL. The CC-light is being/has been phased out in our labs,
    thus code generation was no longer necessary, and has thus been removed
    entirely. However, most test cases and most compiler-development-related
    activities still rely on parts of the CC-light architecture, hence the
    architecture itself remains. It is also useful as an example for what a
    basic architecture should look like within OpenQL's codebase.

    For extensive documentation on what the architecture was and how it worked,
    please refer to the documentation pages of older versions of OpenQL. What
    still remains in OpenQL now is almost entirely based on configuring
    reusable generalizations of CC-light specific code; therefore, its function
    can largely be derived from the default configuration file and the
    documentation that documents the relevant sections of it.
    )");
}

/**
 * Returns a user-friendly type name for this architecture. Used for
 * documentation generation.
 */
utils::Str Info::get_friendly_name() const {
    return "CC-light";
}

/**
 * Returns the name of the namespace for this architecture.
 */
utils::Str Info::get_namespace_name() const {
    return "cc_light";
}

/**
 * Returns a list of strings accepted for the "eqasm_compiler" key in the
 * platform configuration file. This can be more than one, to support both
 * legacy (inconsistent) names and the new namespace names. The returned
 * set must include at least the name of the namespace.
 */
utils::List<utils::Str> Info::get_eqasm_compiler_names() const {
    return {"cc_light", "cc_light_compiler"};
}

/**
 * Returns a list of platform variants for this architecture. For instance,
 * the CC-light may control different kinds of chips (surface-5, surface-7,
 * surface-17, etc), yet still in essence be a CC-light. Variants may be
 * specified by the user by adding a dot-separated suffix to the
 * "eqasm_compiler" key or architecture namespace. If specified, the variant
 * must match a variant from this list. If not specified, the first variant
 * returned by this function serves as the default value.
 */
utils::List<utils::Str> Info::get_variant_names() const {
    return {
        "default",
        "s5",
        "s7",
        "s17"
    };
}

/**
 * Writes documentation for a particular variant of this architecture to the
 * given output stream.
 */
void Info::dump_variant_docs(
    const utils::Str &variant,
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    if (variant == "default") {
        utils::dump_str(os, line_prefix, R"(
        This is the default CC-light configuration, based on what used to be
        ``config_cc_light.json``, which in turn is a simplified
        version of the surface-7 configuration (the instruction durations are
        comparatively short and uniform).
        )");
        return;
    } else if (variant == "s5") {
        utils::dump_str(os, line_prefix, R"(
        This variant models the surface-5 chip. It is primarily intended as a
        baseline configuration for testing mapping and scheduling, as the eQASM
        backend is no longer part of OpenQL.
        )");
        return;
    } else if (variant == "s7") {
        utils::dump_str(os, line_prefix, R"(
        This variant models the surface-7 chip. It is primarily intended as a
        baseline configuration for testing mapping and scheduling, as the eQASM
        backend is no longer part of OpenQL.
        )");
        return;
    } else if (variant == "s17") {
        utils::dump_str(os, line_prefix, R"(
        This variant models the surface-17 chip. It is primarily intended as a
        baseline configuration for testing mapping and scheduling, as the eQASM
        backend is no longer part of OpenQL.
        )");
        return;
    }
    QL_ASSERT(false);
}

/**
 * Should generate a sane default platform JSON file for the given variant
 * of this architecture. This JSON data will still be preprocessed by
 * preprocess_platform().
 */
utils::Str Info::get_default_platform(const utils::Str &variant) const {
    if (variant == "default") {
        return HWCONF_DEFAULT_DATA;
    } else if (variant == "s5") {
        return HWCONF_S5_DATA;
    } else if (variant == "s7") {
        return HWCONF_S7_DATA;
    } else if (variant == "s17") {
        return HWCONF_S17_DATA;
    }
    QL_ASSERT(false);
}

/**
 * Adds the default "backend passes" for this platform. Called by
 * pmgr::Manager::from_defaults() when no compiler configuration file is
 * specified. This typically includes at least the architecture-specific
 * code generation pass, but anything after prescheduling and optimization
 * is considered a backend pass.
 */
void Info::populate_backend_passes(pmgr::Manager &manager, const utils::Str &) const {
    // Mapping.
    if (com::options::global["clifford_premapper"].as_bool()) {
        manager.append_pass(
            "opt.clifford.Optimize",
            "clifford_premapper"
        );
    }
    if (com::options::global["mapper"].as_str() != "no") {
        manager.append_pass(
            "map.qubits.Map",
            "mapper"
        );
    }
    if (com::options::global["clifford_postmapper"].as_bool()) {
        manager.append_pass(
            "opt.clifford.Optimize",
            "clifford_postmapper"
        );
    }

    // Scheduling.
    if (com::options::global["scheduler_heuristic"].is_set()) {
        manager.append_pass(
            "sch.Schedule",
            "rcscheduler",
            {
                {"resource_constraints", "yes"}
            }
        );
    } else {
        manager.append_pass(
            "sch.ListSchedule",
            "rcscheduler",
            {
                {"resource_constraints", "yes"}
            }
        );
    }
    manager.append_pass(
        "io.cqasm.Report",
        "lastqasmwriter",
        {
            {"output_prefix", com::options::global["output_dir"].as_str() + "/%N"},
            {"output_suffix", "_last.qasm"}
        }
    );

    // CC-light code generation.
}

} // namespace cc_light
} // namespace arch
} // namespace ql
