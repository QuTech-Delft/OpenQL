/** \file
 * Defines miscellaneous API functions.
 */

#include "ql/api/misc.h"

#include "ql/version.h"
#include "ql/utils/logger.h"
#include "ql/com/options.h"

//============================================================================//
//                               W A R N I N G                                //
//----------------------------------------------------------------------------//
//         Docstrings in this file must manually be kept in sync with         //
//   python/openql.i! This should be automated at some point, but isn't yet.  //
//============================================================================//

namespace ql {
namespace api {

/**
 * Records whether initialized has been called yet.
 */
static bool initialized = false;

/**
 * Initializes the OpenQL library, for as far as this must be done. This should
 * be called by the user (in Python) before anything else.
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
 * Make sure initialize() has been called.
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
void set_option(const std::string &option_name, const std::string &option_value) {
    ensure_initialized();
    ql::com::options::global[option_name] = option_value;
}

/**
 * Returns the current value for a global option. Use print_options() to get a
 * list of all available options.
 */
std::string get_option(const std::string &option_name) {
    return ql::com::options::global[option_name].as_str();
}

/**
 * Prints a list of all available options.
 */
void print_options() {
    ql::com::options::global.help();
}

} // namespace api
} // namespace ql
