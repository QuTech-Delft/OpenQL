/** \file
 * API header for using quantum programs.
 */

#pragma once

#include "ql/ir/ir.h"
#include "ql/pmgr/manager.h"
#include "ql/api/declarations.h"
#include "ql/api/platform.h"
#include "ql/api/program.h"

//============================================================================//
//                               W A R N I N G                                //
//----------------------------------------------------------------------------//
//         Docstrings in this file must manually be kept in sync with         //
//   python/openql.i! This should be automated at some point, but isn't yet.  //
//============================================================================//

namespace ql {
namespace api {

/**
 * Represents a complete quantum program.
 */
class Program {
private:
    friend class Compiler;
    friend class cQasmReader;

    /**
     * The wrapped program.
     */
    ql::ir::ProgramRef program;

    /**
     * The pass manager that was associated with the platform when this program
     * was constructed, if any. If set, it must be used for compile().
     * Otherwise, compile() should generate it in-place.
     */
    ql::pmgr::Ref pass_manager;

public:

    /**
     * The name given to the program by the user.
     */
    const std::string name;

    /**
     * The platform associated with the program.
     */
    const Platform platform;

    /**
     * The number of (virtual) qubits allocated for the program.
     */
    const size_t qubit_count;

    /**
     * The number of classical integer registers allocated for the program.
     */
    const size_t creg_count;

    /**
     * The number of classical bit registers allocated for the program.
     */
    const size_t breg_count;

    /**
     * Creates a new program with the given name, using the given platform.
     * The third, fourth, and fifth arguments optionally specify the desired
     * number of qubits, classical integer registers, and classical bit
     * registers. If not specified, the number of qubits is taken from the
     * platform, and no classical or bit registers will be allocated.
     */
    Program(
        const std::string &name,
        const Platform &platform,
        size_t qubit_count = 0,
        size_t creg_count = 0,
        size_t breg_count = 0
    );

    /**
     * Adds an unconditionally-executed kernel to the end of the program.
     */
    void add_kernel(const Kernel &k);

    /**
     * Adds an unconditionally-executed subprogram to the end of the program.
     */
    void add_program(const Program &p);

    /**
     * Adds a conditionally-executed kernel to the end of the program. The
     * kernel will be executed if the given classical condition evaluates to
     * true.
     */
    void add_if(const Kernel &k, const Operation &operation);

    /**
     * Adds a conditionally-executed subprogram to the end of the program. The
     * kernel will be executed if the given classical condition evaluates to
     * true.
     */
    void add_if(const Program &p, const Operation &operation);

    /**
     * Adds two conditionally-executed kernels with inverted conditions to the
     * end of the program. The first kernel will be executed if the given
     * classical condition evaluates to true; the second kernel will be executed
     * if it evaluates to false.
     */
    void add_if_else(const Kernel &k_if, const Kernel &k_else, const Operation &operation);

    /**
     * Adds two conditionally-executed subprograms with inverted conditions to
     * the end of the program. The first kernel will be executed if the given
     * classical condition evaluates to true; the second kernel will be executed
     * if it evaluates to false.
     */
    void add_if_else(const Program &p_if, const Program &p_else, const Operation &operation);

    /**
     * Adds a kernel that will be repeated until the given classical condition
     * evaluates to true. The kernel is executed at least once, since the
     * condition is evaluated at the end of the loop body.
     */
    void add_do_while(const Kernel &k, const Operation &operation);

    /**
     * Adds a subprogram that will be repeated until the given classical
     * condition evaluates to true. The subprogram is executed at least once,
     * since the condition is evaluated at the end of the loop body.
     */
    void add_do_while(const Program &p, const Operation &operation);

    /**
     * Adds an unconditionally-executed kernel that will loop for the given
     * number of iterations.
     */
    void add_for(const Kernel &k, size_t iterations);

    /**
     * Adds an unconditionally-executed subprogram that will loop for the given
     * number of iterations.
     */
    void add_for(const Program &p, size_t iterations);

    /**
     * Sets sweep point information for the program.
     */
    void set_sweep_points(const std::vector<double> &sweep_points);

    /**
     * Returns the configured sweep point information for the program.
     */
    std::vector<double> get_sweep_points() const;

    /**
     * Sets the name of the file that the sweep points will be written to.
     */
    void set_config_file(const std::string &config_file_name);

    /**
     * Whether a custom compiler configuration has been attached to this
     * program. When this is the case, it will be used to implement compile(),
     * rather than generating the compiler in-place from defaults and global
     * options during the call.
     */
    bool has_compiler();

    /**
     * Returns the custom compiler configuration associated with this program.
     * If no such configuration exists yet, the default one is created,
     * attached, and returned.
     */
    Compiler get_compiler();

    /**
     * Sets the compiler associated with this program. It will then be used for
     * compile().
     */
    void set_compiler(const Compiler &compiler);

    /**
     * Compiles the program.
     */
    void compile();

    /**
     * Prints the interaction matrix for each kernel in the program.
     */
    void print_interaction_matrix() const;

    /**
     * Writes the interaction matrix for each kernel in the program to a file.
     * This is one of the few functions that still uses the global output_dir
     * option.
     */
    void write_interaction_matrix() const;

};

} // namespace api
} // namespace ql
