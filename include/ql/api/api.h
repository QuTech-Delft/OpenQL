/** \file
 * Header for Python interface.
 */

#pragma once

#include "ql/ir/ir.h"
#include "ql/com/options.h"
#include "ql/com/unitary.h"
#include "ql/pass/io/cqasm/read.h"
#include "ql/pmgr/manager.h"

//============================================================================//
//                               W A R N I N G                                //
//----------------------------------------------------------------------------//
//         Docstrings in this file must manually be kept in sync with         //
//   python/openql.i! This should be automated at some point, but isn't yet.  //
//============================================================================//

namespace ql {
namespace api {

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
void initialize();

/**
 * Returns the compiler's version string.
 */
std::string get_version();

/**
 * Sets a global option for the compiler. Use print_options() to get a list of
 * all available options.
 */
void set_option(const std::string &option_name, const std::string &option_value);

/**
 * Returns the current value for a global option. Use print_options() to get a
 * list of all available options.
 */
std::string get_option(const std::string &option_name);

/**
 * Prints a list of all available options.
 */
void print_options();

// Forward declarations for classes.
class Pass;
class Compiler;
class Platform;
class CReg;
class Operation;
class Unitary;
class Program;
class Kernel;
class cQasmReader;

/**
 * Wrapper for a pass that belongs to some pass manager.
 */
class Pass {
    friend class Compiler;

    /**
     * The linked pass.
     */
    ql::pmgr::PassRef pass;

    /**
     * Constructor used internally to build a pass object that belongs to
     * a compiler.
     */
    explicit Pass(const ql::pmgr::PassRef &pass);

public:

    // TODO
};

/**
 * Wrapper for the compiler/pass manager.
 */
class Compiler {
private:
    friend class Platform;
    friend class Program;

    /**
     * The linked pass manager.
     */
    ql::pmgr::Ref pass_manager;

    /**
     * Constructor used internally to build a compiler object that belongs to
     * a platform.
     */
    explicit Compiler(const ql::pmgr::Ref &pass_manager);

public:

    /**
     * Creates an empty compiler, with no specified architecture.
     */
    Compiler();

    /**
     * Creates a compiler configuration from the given JSON file.
     */
    explicit Compiler(const std::string &fname);

    /**
     * Creates a default compiler for the given platform.
     */
    explicit Compiler(const Platform &platform);

    /**
     * Prints documentation for all available pass types, as well as the option
     * documentation for the passes.
     */
    void print_pass_types() const;

    /**
     * Returns documentation for all available pass types, as well as the option
     * documentation for the passes.
     */
    std::string get_pass_types() const;

    /**
     * Prints the currently configured compilation strategy.
     */
    void print_strategy() const;

    /**
     * Returns the currently configured compilation strategy as a string.
     */
    std::string get_strategy() const;

    /**
     * Sets a pass option. Periods are used as hierarchy separators; the last
     * element will be the option name, and the preceding elements represent
     * pass instance names. Furthermore, wildcards may be used for the pass name
     * elements (asterisks for zero or more characters and a question mark for a
     * single character) to select multiple or all immediate sub-passes of that
     * group, and a double asterisk may be used for the element before the
     * option name to chain to set_option_recursively() instead. The return
     * value is the number of passes that were affected; passes are only
     * affected when they are selected by the option path AND have an option
     * with the specified name. If must_exist is set an exception will be thrown
     * if none of the passes were affected, otherwise 0 will be returned.
     */
    size_t set_option(
        const std::string &path,
        const std::string &value,
        bool must_exist = true
    );

    /**
     * Sets an option for all passes recursively. The return value is the number
     * of passes that were affected; passes are only affected when they have an
     * option with the specified name. If must_exist is set an exception will be
     * thrown if none of the passes were affected, otherwise 0 will be returned.
     */
    size_t set_option_recursively(
        const std::string &option,
        const std::string &value,
        bool must_exist = true
    );

    /**
     * Returns the current value of an option. Periods are used as hierarchy
     * separators; the last element will be the option name, and the preceding
     * elements represent pass instance names.
     */
    std::string get_option(const std::string &path) const;

    /**
     * Appends a pass to the end of the pass list. If type_name is empty
     * or unspecified, a generic subgroup is added. Returns a reference to the
     * constructed pass.
     */
    Pass append_pass(
        const std::string &type_name = "",
        const std::string &instance_name = "",
        const std::map<std::string, std::string> &options = {}
    );

    /**
     * Appends a pass to the beginning of the pass list. If type_name is empty
     * or unspecified, a generic subgroup is added. Returns a reference to the
     * constructed pass.
     */
    Pass prefix_pass(
        const std::string &type_name = "",
        const std::string &instance_name = "",
        const std::map<std::string, std::string> &options = {}
    );

    /**
     * Inserts a pass immediately after the target pass (named by instance). If
     * target does not exist, an exception is thrown. If type_name is empty or
     * unspecified, a generic subgroup is added. Returns a reference to the
     * constructed pass. Periods may be used in target to traverse deeper into
     * the pass hierarchy.
     */
    Pass insert_pass_after(
        const std::string &target,
        const std::string &type_name = "",
        const std::string &instance_name = "",
        const std::map<std::string, std::string> &options = {}
    );

    /**
     * Inserts a pass immediately before the target pass (named by instance). If
     * target does not exist, an exception is thrown. If type_name is empty or
     * unspecified, a generic subgroup is added. Returns a reference to the
     * constructed pass. Periods may be used in target to traverse deeper into
     * the pass hierarchy.
     */
    Pass insert_pass_before(
        const std::string &target,
        const std::string &type_name = "",
        const std::string &instance_name = "",
        const std::map<std::string, std::string> &options = {}
    );

    /**
     * Looks for the pass with the target instance name, and embeds it into a
     * newly generated group. The group will assume the name of the original
     * pass, while the original pass will be renamed as specified by sub_name.
     * Note that this ultimately does not modify the pass order. If target does
     * not exist or this pass is not a group of sub-passes, an exception is
     * thrown. Returns a reference to the constructed group. Periods may be used
     * in target to traverse deeper into the pass hierarchy.
     */
    Pass group_pass(
        const std::string &target,
        const std::string &sub_name = "main"
    );

    /**
     * Like group_pass(), but groups an inclusive range of passes into a
     * group with the given name, leaving the original pass names unchanged.
     * Periods may be used in from/to to traverse deeper into the pass
     * hierarchy, but the hierarchy prefix must be the same for from and to.
     */
    Pass group_passes(
        const std::string &from,
        const std::string &to,
        const std::string &group_name
    );

    /**
     * Looks for an unconditional pass group with the target instance name and
     * flattens its contained passes into its parent group. The names of the
     * passes found in the collapsed group are prefixed with name_prefix before
     * they are added to the parent group. Note that this ultimately does not
     * modify the pass order. If the target instance name does not exist or is
     * not an unconditional group, an exception is thrown. Periods may be used
     * in target to traverse deeper into the pass hierarchy.
     */
    void flatten_subgroup(
        const std::string &target,
        const std::string &name_prefix = ""
    );

    /**
     * Returns a reference to the pass with the given instance name. If no such
     * pass exists, an exception is thrown. Periods may be used as hierarchy
     * separators to get nested sub-passes.
     */
    Pass get_pass(const std::string &target) const;

    /**
     * Returns whether a pass with the target instance name exists. Periods may
     * be used in target to traverse deeper into the pass hierarchy.
     */
    bool does_pass_exist(const std::string &target) const;

    /**
     * Returns the total number of passes in the root hierarchy.
     */
    size_t get_num_passes() const;

    /**
     * If this pass constructed into a group of passes, returns a reference to
     * the list containing all the sub-passes. Otherwise, an exception is
     * thrown.
     */
    std::vector<Pass> get_passes() const;

    /**
     * Returns an indexable list of references to all passes with the given
     * type within the root hierarchy.
     */
    std::vector<Pass> get_sub_passes_by_type(const std::string &target) const;

    /**
     * Removes the pass with the given target instance name, or throws an
     * exception if no such pass exists.
     */
    void remove_pass(const std::string &target);

    /**
     * Clears the entire pass list.
     */
    void clear_passes();

    /**
     * Constructs all passes recursively. This freezes the pass options, but
     * allows subtrees to be modified.
     */
    void construct();

    /**
     * Ensures that all passes have been constructed, and then runs the passes
     * on the given program. This is the same as Program.compile() when the
     * program's platform is referencing the same compiler
     */
    void compile(const Program &program);

};

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

class CReg {
private:
    friend class Operation;
    friend class Kernel;

    ql::utils::Ptr<ql::ir::ClassicalRegister> creg;

public:
    explicit CReg(size_t id);
};

class Operation {
private:
    friend class Kernel;
    friend class Program;

    ql::utils::Ptr<ql::ir::ClassicalOperation> operation;

public:
    Operation(const CReg &lop, const std::string &op, const CReg &rop);
    Operation(const std::string &op, const CReg &rop);
    explicit Operation(const CReg &lop);
    explicit Operation(int val);
};

typedef std::complex<double> Complex;

/**
 * quantum unitary matrix interface
 */
class Unitary {
private:
    friend class Kernel;

    /**
     * The wrapped unitary gate.
     */
    ql::utils::Ptr<ql::com::Unitary> unitary;

public:
    const std::string name;

    Unitary(const std::string &name, const std::vector<std::complex<double>> &matrix);
    void decompose();
    static bool is_decompose_support_enabled();
};

/**
 * quantum kernel interface
 */
class Kernel {
private:
    friend class Program;

    ql::ir::KernelRef kernel;

public:
    const std::string name;
    const Platform platform;
    const size_t qubit_count;
    const size_t creg_count;
    const size_t breg_count;

    Kernel(
        const std::string &name,
        const Platform &platform,
        size_t qubit_count,
        size_t creg_count = 0,
        size_t breg_count = 0
    );

    void identity(size_t q0);
    void hadamard(size_t q0);
    void s(size_t q0);
    void sdag(size_t q0);
    void t(size_t q0);
    void tdag(size_t q0);
    void x(size_t q0);
    void y(size_t q0);
    void z(size_t q0);
    void rx90(size_t q0);
    void mrx90(size_t q0);
    void rx180(size_t q0);
    void ry90(size_t q0);
    void mry90(size_t q0);
    void ry180(size_t q0);
    void rx(size_t q0, double angle);
    void ry(size_t q0, double angle);
    void rz(size_t q0, double angle);
    void measure(size_t q0);
    void measure(size_t q0, size_t b0);
    void prepz(size_t q0);
    void cnot(size_t q0, size_t q1);
    void cphase(size_t q0, size_t q1);
    void cz(size_t q0, size_t q1);
    void toffoli(size_t q0, size_t q1, size_t q2);
    void clifford(int id, size_t q0);
    void wait(const std::vector<size_t> &qubits, size_t duration);
    void barrier(const std::vector<size_t> &qubits = std::vector<size_t>());
    std::string get_custom_instructions() const;
    void display();
    void gate(const std::string &gname, size_t q0);
    void gate(const std::string &gname, size_t q0, size_t q1);
    void gate(
        const std::string &name,
        const std::vector<size_t> &qubits,
        size_t duration = 0,
        double angle = 0.0,
        const std::vector<size_t> &bregs = {},
        const std::string &condstring = "COND_ALWAYS",
        const std::vector<size_t> &condregs = {}
    );
    void gate(
        const std::string &name,
        const std::vector<size_t> &qubits,
        const CReg &destination
    );
    void gate_preset_condition(
        const std::string &condstring,
        const std::vector<size_t> &condregs
    );
    void gate_clear_condition();
    void condgate(
        const std::string &name,
        const std::vector<size_t> &qubits,
        const std::string &condstring,
        const std::vector<size_t> &condregs
    );
    void gate(const Unitary &u, const std::vector<size_t> &qubits);
    void classical(const CReg &destination, const Operation &operation);
    void classical(const std::string &operation);
    void controlled(
        const Kernel &k,
        const std::vector<size_t> &control_qubits,
        const std::vector<size_t> &ancilla_qubits
    );
    void conjugate(const Kernel &k);
};


/**
 * quantum program interface
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
    const std::string name;
    const Platform platform;
    const size_t qubit_count;
    const size_t creg_count;
    const size_t breg_count;

    Program(
        const std::string &name,
        const Platform &platform,
        size_t qubit_count,
        size_t creg_count = 0,
        size_t breg_count = 0
    );

    void set_sweep_points(const std::vector<double> &sweep_points);
    std::vector<double> get_sweep_points() const;
    void set_config_file(const std::string &config_file_name);
    void add_kernel(Kernel &k);
    void add_program(Program &p);
    void add_if(Kernel &k, const Operation &operation);
    void add_if(Program &p, const Operation &operation);
    void add_if_else(Kernel &k_if, Kernel &k_else, const Operation &operation);
    void add_if_else(Program &p_if, Program &p_else, const Operation &operation);
    void add_do_while(Kernel &k, const Operation &operation);
    void add_do_while(Program &p, const Operation &operation);
    void add_for(Kernel &k, size_t iterations);
    void add_for(Program &p, size_t iterations);

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

/**
 * cqasm reader interface
 */
class cQasmReader {
private:
    ql::utils::Ptr<ql::pass::io::cqasm::read::Reader> cqasm_reader;

public:
    const Platform platform;
    const Program program;

    cQasmReader(const Platform &q_platform, const Program &q_program);
    cQasmReader(const Platform &q_platform, const Program &q_program, const std::string &gateset_fname);
    void string2circuit(const std::string &cqasm_str);
    void file2circuit(const std::string &cqasm_file_path);
};

} // namespace api
} // namespace ql
