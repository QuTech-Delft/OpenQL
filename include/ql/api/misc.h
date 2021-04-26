/** \file
 * Defines miscellaneous API functions.
 */

#pragma once

#include <string>

//============================================================================//
//                               W A R N I N G                                //
//----------------------------------------------------------------------------//
//         Docstrings in this file must manually be kept in sync with         //
//       misc.i! This should be automated at some point, but isn't yet.       //
//============================================================================//

namespace ql {
namespace api {

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
void initialize();

/**
 * Calls initialize() if it hasn't been called yet.
 */
void ensure_initialized();

/**
 * Returns the compiler's version string.
 */
std::string get_version();

/**
 * Sets a global option for the compiler. Use print_options() to get a list of
 * all available options.
 */
void set_option(const std::string &option, const std::string &value);

/**
 * Returns the current value for a global option. Use print_options() to get a
 * list of all available options.
 */
std::string get_option(const std::string &option);

/**
 * Prints the documentation for all available global options.
 */
void print_options();

/**
 * Returns the result of print_options() as a string.
 */
std::string get_options();

/**
 * Prints the documentation for all available target architectures.
 */
void print_architectures();

/**
 * Returns the result of print_architectures() as a string.
 */
std::string get_architectures();

/**
 * Prints the documentation for all available passes.
 */
void print_passes();

/**
 * Returns the result of print_passes() as a string.
 */
std::string get_passes();

/**
 * Prints the documentation for all available scheduler resources.
 */
void print_resources();

/**
 * Returns the result of print_resources() as a string.
 */
std::string get_resources();

/**
 * Prints the documentation for platform configuration files.
 */
void print_platform_docs();

/**
 * Returns the result of print_platform_docs() as a string.
 */
std::string get_platform_docs();

} // namespace api
} // namespace ql
