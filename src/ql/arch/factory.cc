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
 * Builds an architecture from a namespace name. Returns an empty reference
 * if none was found.
 */
CInfoRef Factory::build_from_namespace(const utils::Str &namspace) const {
    auto it = namespace_names.find(namspace);
    if (it == namespace_names.end()) {
        return {};
    } else {
        return it->second.as_const();
    }
}

/**
 * Builds an architecture from an "eqasm_compiler" name. Returns an empty
 * reference if none was found.
 */
CInfoRef Factory::build_from_eqasm_compiler(const utils::Str &eqasm_compiler) const {
    auto it = eqasm_compiler_names.find(eqasm_compiler);
    if (it == eqasm_compiler_names.end()) {
        return {};
    } else {
        return it->second.as_const();
    }
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
        os << line_prefix << "    For the current/default global option values, "
                          << "the following backend passes are used by default.\n";
        os << line_prefix << "    \n";
        pmgr::Manager manager;
        arch->populate_backend_passes(manager);
        manager.dump_strategy(os, line_prefix + "        ");
        os << line_prefix << "    \n";
        os << line_prefix << "  * Default configuration file *\n";
        os << line_prefix << "    \n";
        os << line_prefix << "    When no platform configuration file is specified, "
                          << "    the following default file is used instead.\n";
        os << line_prefix << "    \n";
        os << line_prefix << "    ```json\n";
        utils::dump_str(os, line_prefix + "    ", arch->get_default_platform());
        os << line_prefix << "    ```\n";
        os << line_prefix << "\n";

    }
}

} // namespace arch
} // namespacq ql
