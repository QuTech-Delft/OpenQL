/** \file
 * Defines miscellaneous API functions.
 */

#include "ql/api/misc.h"

#include "ql/version.h"
#include "ql/utils/logger.h"
#include "ql/com/options.h"
#include "ql/arch/factory.h"
#include "ql/pmgr/factory.h"
#include "ql/pmgr/manager.h"
#include "ql/rmgr/factory.h"

//============================================================================//
//                               W A R N I N G                                //
//----------------------------------------------------------------------------//
//         Docstrings in this file must manually be kept in sync with         //
//       misc.i! This should be automated at some point, but isn't yet.       //
//============================================================================//

namespace ql {
namespace api {

/**
 * Records whether initialized has been called yet.
 */
static bool initialized = false;

/**
 * Initializes the OpenQL library, for as far as this must be done. This should
 * ideally be called by the user (in Python) before anything else, but
 * set_option() and the constructors of Compiler and Platform will automatically
 * call this when it hasn't been done yet as well.
 *
 * Currently this just resets the options to their default values to give the
 * user a clean slate to work with in terms of global variables (in case someone
 * else has used the library in the same interpreter before them, for instance,
 * as might happen with ipython/Jupyter in a shared notebook server, or during
 * test suites), but it may initialize more things in the future.
 */
void initialize() {
    if (initialized) {
        QL_IOUT("re-initializing OpenQL library");
    } else {
        QL_IOUT("initializing OpenQL library");
    }
    initialized = true;
    ql::com::options::global.reset();
}

/**
 * Calls initialize() if it hasn't been called yet.
 */
void ensure_initialized() {
    if (!initialized) {
        QL_WOUT("Calling initialize() implicitly! In the future, please call initialize() before anything else.");
        initialize();
    }
}

/**
 * Returns the compiler's version string.
 */
std::string get_version() {
    return OPENQL_VERSION_STRING;
}

/**
 * Sets a global option for the compiler. Use print_options() to get a list of
 * all available options.
 */
void set_option(const std::string &option, const std::string &value) {
    ensure_initialized();
    ql::com::options::global[option] = value;
}

/**
 * Returns the current value for a global option. Use print_options() to get a
 * list of all available options.
 */
std::string get_option(const std::string &option) {
    return ql::com::options::global[option].as_str();
}

/**
 * Prints the documentation for all available global options.
 */
void print_options() {
    ql::com::options::global.dump_help();
}

/**
 * Returns the result of print_options() as a string.
 */
std::string dump_options() {
    std::ostringstream ss;
    ql::com::options::global.dump_help(ss);
    return ss.str();
}

/**
 * Prints the documentation for all available target architectures.
 */
void print_architectures() {
    ql::arch::Factory().dump_architectures();
}

/**
 * Returns the result of print_architectures() as a string.
 */
std::string dump_architectures() {
    std::ostringstream ss;
    ql::arch::Factory().dump_architectures(ss);
    return ss.str();
}

/**
 * Prints the documentation for all available passes.
 */
void print_passes() {
    ql::pmgr::Factory::dump_pass_types(ql::pmgr::Factory());
}

/**
 * Returns the result of print_passes() as a string.
 */
std::string dump_passes() {
    std::ostringstream ss;
    ql::pmgr::Factory::dump_pass_types(ql::pmgr::Factory(), ss);
    return ss.str();
}

/**
 * Prints the documentation for all available scheduler resources.
 */
void print_resources() {
    ql::rmgr::Factory().dump_resource_types();
}

/**
 * Returns the result of print_resources() as a string.
 */
std::string dump_resources() {
    std::ostringstream ss;
    ql::rmgr::Factory().dump_resource_types(ss);
    return ss.str();
}

/**
 * Prints the documentation for platform configuration files.
 */
void print_platform_docs() {
    ql::plat::Platform::dump_docs();
}

/**
 * Returns the result of print_platform_docs() as a string.
 */
std::string dump_platform_docs() {
    std::ostringstream ss;
    ql::plat::Platform::dump_docs(ss);
    return ss.str();
}

/**
 * Prints the documentation for compiler configuration files.
 */
void print_compiler_docs() {
    ql::pmgr::Manager::dump_docs();
}

/**
 * Returns the result of print_compiler_docs() as a string.
 */
std::string dump_compiler_docs() {
    std::ostringstream ss;
    ql::pmgr::Manager::dump_docs(ss);
    return ss.str();
}

} // namespace api
} // namespace ql
