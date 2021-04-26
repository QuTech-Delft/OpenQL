/** \file
 * Resource factory implementation.
 */

#include "ql/arch/factory.h"

#include "ql/arch/cc/info.h"
#include "ql/arch/cc_light/info.h"
#include "ql/arch/none/info.h"

namespace ql {
namespace arch {

/**
 * Constructs a default architecture factory for OpenQL.
 */
Factory::Factory() {
    register_architecture<cc::Info>();
    register_architecture<cc_light::Info>();
    register_architecture<none::Info>();
}

/**
 * Implementation of build_from_namespace() and build_from_eqasm_compiler(),
 * using the given map for the lookup.
 */
CArchitectureRef Factory::build_from_map(
    const utils::Map<utils::Str, InfoRef> &map,
    const utils::Str &str
) const {
    auto pos = str.find('.');
    auto name = str.substr(0, pos);
    utils::Str variant;
    if (pos != utils::Str::npos) {
        variant = str.substr(pos + 1);
    }
    auto it = map.find(name);
    if (it == map.end()) {
        return {};
    }
    auto architecture = it->second.as_const();
    if (variant.empty()) {
        variant = architecture->get_variant_names().front();
    } else {
        utils::Bool ok = false;
        for (const auto &existing_variant : architecture->get_variant_names()) {
            if (variant == existing_variant) {
                ok = true;
                break;
            }
        }
        if (!ok) {
            return {};
        }
    }
    return Architecture(architecture, variant);
}

/**
 * Builds an architecture from an "eqasm_compiler" name. Returns a reference
 * to the architecture variant object if one was found. Otherwise, an empty
 * reference is returned.
 */
CArchitectureRef Factory::build_from_namespace(
    const utils::Str &namspace
) const {
    return build_from_map(namespace_names, namspace);
}

/**
 * Builds an architecture from an "eqasm_compiler" name. Returns a reference
 * to the architecture variant object if one was found. Otherwise, an empty
 * reference is returned.
 */
CArchitectureRef Factory::build_from_eqasm_compiler(
    const utils::Str &eqasm_compiler
) const {
    return build_from_map(eqasm_compiler_names, eqasm_compiler);
}

/**
 * Dumps documentation for all architectures known by this factory.
 */
void Factory::dump_architectures(std::ostream &os, const utils::Str &line_prefix) const {

    for (const auto &it : namespace_names) {
        const auto arch = it.second.as_const();

        os << line_prefix << "* " << arch->get_friendly_name() << " *\n";
        os << line_prefix << "  \n";
        os << line_prefix << "   - Pass/resource/C++ namespace: `arch."
                          << arch->get_namespace_name() << "`\n";
        os << line_prefix << "   - Acceptable `\"eqasm_compiler\"` values: "
                          << arch->get_eqasm_compiler_names().to_string(
                              "`\"", "\"`, `\"", "\"`", "\"`, or `\"", "\"` or `\"") << "\n";
        os << line_prefix << "  \n";
        arch->dump_docs(os, line_prefix + "  ");
        os << line_prefix << "    \n";
        os << line_prefix << "  * Default pass list *\n";
        os << line_prefix << "    \n";
        pmgr::Manager manager;
        auto variants = arch->get_variant_names();
        arch->populate_backend_passes(manager, variants.front());
        if (manager.get_num_passes() > 0) {
            os << line_prefix << "    For the current/default global option values "
                              << "and the default variant (`" << variants.front() << "`), "
                              << "the following backend passes are used by default.\n";
            os << line_prefix << "    \n";
            manager.dump_strategy(os, line_prefix + "        ");
        } else {
            os << line_prefix << "    For the current/default global option values, "
                              << "this architecture does not insert any backend passes.\n";
        }
        if (variants.size() <= 1) {
            os << line_prefix << "    \n";
            os << line_prefix << "  * Default configuration file *\n";
            os << line_prefix << "    \n";
            os << line_prefix << "    When no platform configuration file is specified, "
                              << "    the following default file is used instead.\n";
            os << line_prefix << "    \n";
            os << line_prefix << "    ```json\n";
            utils::dump_str(os, line_prefix + "    ", arch->get_default_platform(variants.front()));
            os << line_prefix << "    ```\n";
        } else {
            for (const auto &variant : variants) {
                os << line_prefix << "    \n";
                os << line_prefix << "  * `" << variant << "` variant *\n";
                os << line_prefix << "    \n";
                arch->dump_variant_docs(variant, os, line_prefix + "    ");
                os << line_prefix << "    \n";
                os << line_prefix << "    When no platform configuration file is specified, "
                                  << "    the following default file is used instead.\n";
                os << line_prefix << "    \n";
                os << line_prefix << "    ```json\n";
                utils::dump_str(os, line_prefix + "    ", arch->get_default_platform(variant));
                os << line_prefix << "    ```\n";
            }
        }
        os << line_prefix << "\n";

    }
}

} // namespace arch
} // namespacq ql
