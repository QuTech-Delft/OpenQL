/** \file
 * Defines information about the CC architecture.
 */

#include "ql/arch/cc/info.h"

#include "ql/com/options.h"
#include "ql/arch/cc/pass/gen/vq1asm/detail/options.h"
#include "ql/arch/cc/resources/hwconf_default.inc"

namespace ql {
namespace arch {
namespace cc {

/**
 * Writes the documentation for this architecture to the given output
 * stream.
 */
void Info::dump_docs(std::ostream &os, const utils::Str &line_prefix) const {
    utils::dump_str(os, line_prefix, R"(
    TODO: port from docs/platform_cc.rst
    )");
}

/**
 * Returns a user-friendly type name for this architecture. Used for
 * documentation generation.
 */
utils::Str Info::get_friendly_name() const {
    return "QuTech Central Controller";
}

/**
 * Returns the name of the namespace for this architecture.
 */
utils::Str Info::get_namespace_name() const {
    return "cc";
}

/**
 * Returns a list of strings accepted for the "eqasm_compiler" key in the
 * platform configuration file. This can be more than one, to support both
 * legacy (inconsistent) names and the new namespace names. The returned
 * set must include at least the name of the namespace.
 */
utils::List<utils::Str> Info::get_eqasm_compiler_names() const {
    return {"cc", "eqasm_backend_cc"};
}

/**
 * Should generate a sane default platform JSON file for the given variant
 * of this architecture. This JSON data will still be preprocessed by
 * preprocess_platform().
 */
utils::Str Info::get_default_platform(const utils::Str &variant) const {

    // NOTE: based on tests/cc/cc_s5_direct_iq.json at the time of writing.
    return HWCONF_DEFAULT_DATA;

}

/**
 * Preprocessing logic for the platform JSON configuration file. May be used
 * to generate/expand certain things that are always the same for that
 * platform, to save typing in the configuration file (and reduce the amount
 * of mistakes made).
 */
void Info::preprocess_platform(utils::Json &data, const utils::Str &variant) const {

    // TODO Wouter: any CC-specific platform configuration file preprocessing
    //  you might want to do for the resources can go here!

}

/**
 * Adds the default "backend passes" for this platform. Called by
 * pmgr::Manager::from_defaults() when no compiler configuration file is
 * specified. This typically includes at least the architecture-specific
 * code generation pass, but anything after prescheduling and optimization
 * is considered a backend pass.
 */
void Info::populate_backend_passes(pmgr::Manager &manager, const utils::Str &variant) const {

    // Mimic original CC backend.
    manager.append_pass(
        "sch.Schedule",
        "scheduler",
        {
#if OPT_CC_SCHEDULE_RC
            {"resource_constraints", "yes"}
#else
            {"resource_constraints", "no"}
#endif
        }
    );
    manager.append_pass(
        "arch.cc.gen.VQ1Asm",
        "codegen",
        {
            {"output_prefix", com::options::global["output_dir"].as_str() + "/%N"}
        }
    );

}

} // namespace cc
} // namespace arch
} // namespace ql
