/** \file
 * API header for loading and managing quantum platform information.
 */

#include "ql/api/platform.h"

#include "ql/api/misc.h"

//============================================================================//
//                               W A R N I N G                                //
//----------------------------------------------------------------------------//
//         Docstrings in this file must manually be kept in sync with         //
//     platform.i! This should be automated at some point, but isn't yet.     //
//============================================================================//

namespace ql {
namespace api {

/**
 * Constructs a platform. name is any name the user wants to give to the
 * platform; it is only used for report messages. platform_config_file must
 * point to a JSON file that represents the platform. Optionally,
 * compiler_config_file can be specified to override the compiler
 * configuration specified by the platform (if any).
 */
Platform::Platform(
    const std::string &name,
    const std::string &platform_config_file,
    const std::string &compiler_config_file
) :
    name(name),
    config_file(platform_config_file)
{
    ensure_initialized();
    platform.emplace(name, platform_config_file, compiler_config_file);
}

/**
 * Returns the number of qubits in the platform.
 */
size_t Platform::get_qubit_number() const {
    return platform->qubit_count;
}

/**
 * Prints some basic information about the platform.
 */
void Platform::print_info() const {
    platform->dump_info();
}

/**
 * Returns the result of print_info() as a string.
 */
std::string Platform::get_info() const {
    std::ostringstream ss;
    platform->dump_info(ss);
    return ss.str();
}

/**
 * Returns whether a custom compiler configuration has been attached to this
 * platform. When this is the case, programs constructed from this platform
 * will use it to implement Program.compile(), rather than generating the
 * compiler in-place from defaults and global options during the call.
 */
bool Platform::has_compiler() {
    return pass_manager.has_value();
}

/**
 * Returns the custom compiler configuration associated with this platform.
 * If no such configuration exists yet, the default one is created,
 * attached, and returned.
 */
Compiler Platform::get_compiler() {
    if (!pass_manager.has_value()) {
        pass_manager.emplace(ql::pmgr::Manager::from_defaults(platform));
    }
    return Compiler(pass_manager);
}

/**
 * Sets the compiler associated with this platform. Any programs constructed
 * from this platform after this call will use the given compiler.
 */
void Platform::set_compiler(const Compiler &compiler) {
    pass_manager = compiler.pass_manager;
}

} // namespace api
} // namespace ql
