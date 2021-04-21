/** \file
 * API header for loading and managing quantum platform information.
 */

#pragma once

#include "ql/plat/platform.h"
#include "ql/pmgr/manager.h"
#include "ql/api/declarations.h"
#include "ql/api/compiler.h"

//============================================================================//
//                               W A R N I N G                                //
//----------------------------------------------------------------------------//
//         Docstrings in this file must manually be kept in sync with         //
//   python/openql.i! This should be automated at some point, but isn't yet.  //
//============================================================================//

namespace ql {
namespace api {

/**
 * Quantum platform description. Describes everything that the compiler needs to
 * know about the target quantum chip, instruments, etc.
 */
class Platform {
private:
    friend class Compiler;
    friend class Kernel;
    friend class Program;
    friend class cQasmReader;

    /**
     * The wrapped platform.
     */
    ql::plat::PlatformRef platform;

    /**
     * Wrapped pass manager. If this is non-null, it will be used for
     * Program.compile for programs constructed using this platform.
     */
    ql::pmgr::Ref pass_manager;

public:

    /**
     * The user-given name of the platform.
     */
    const std::string name;

    /**
     * The configuration file that the platform was loaded from.
     */
    const std::string config_file;

    /**
     * Constructs a platform. name is any name the user wants to give to the
     * platform; it is only used for report messages. platform_config_file must
     * point to a JSON file that represents the platform. Optionally,
     * compiler_config_file can be specified to override the compiler
     * configuration specified by the platform (if any).
     */
    Platform(
        const std::string &name,
        const std::string &platform_config_file,
        const std::string &compiler_config_file = ""
    );

    /**
     * Returns the number of qubits in the platform.
     */
    size_t get_qubit_number() const;

    /**
     * Prints some basic information about the platform.
     */
    void print_info() const;

    /**
     * Returns the result of print_info() as a string.
     */
    std::string get_info() const;

    /**
     * Whether a custom compiler configuration has been attached to this
     * platform. When this is the case, programs constructed from this platform
     * will use it to implement Program.compile(), rather than generating the
     * compiler in-place from defaults and global options during the call.
     */
    bool has_compiler();

    /**
     * Returns the custom compiler configuration associated with this platform.
     * If no such configuration exists yet, the default one is created,
     * attached, and returned.
     */
    Compiler get_compiler();

    /**
     * Sets the compiler associated with this platform. Any programs constructed
     * from this platform after this call will use the given compiler.
     */
    void set_compiler(const Compiler &compiler);

};

} // namespace api
} // namespace ql
