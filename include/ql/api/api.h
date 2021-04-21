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
private:
    friend class Compiler;

    /**
     * The linked pass.
     */
    ql::pmgr::PassRef pass;

    /**
     * Constructor used internally to build a pass object that belongs to
     * a compiler.
     *
     * NOTE: the dummy boolean is because the SWIG wrapper otherwise generates
     *  some inane ambiguity error with the copy/move constructor (even though
     *  this is private and a different type).
     */
    explicit Pass(const ql::pmgr::PassRef &pass, bool dummy);

public:

    /**
     * Default constructor, only exists because the SWIG wrapper breaks
     * otherwise. Pass objects constructed this way cannot be used! You can only
     * use Pass objects returned by Compiler.
     */
    Pass() = default;

    /**
     * Returns the full, desugared type name that this pass was constructed
     * with.
     */
    const std::string &get_type() const;

    /**
     * Returns the instance name of the pass within the surrounding group.
     */
    const std::string &get_name() const;

    /**
     * Prints the documentation for this pass.
     */
    void print_pass_documentation() const;

    /**
     * Returns the documentation for this pass as a string.
     */
    std::string get_pass_documentation() const;

    /**
     * Prints the current state of the options. If only_set is set to true, only
     * the options that were explicitly configured are dumped.
     */
    void print_options(bool only_set = false) const;

    /**
     * Returns the string printed by print_options().
     */
    std::string get_options(bool only_set = false) const;

    /**
     * Prints the entire compilation strategy including configured options of
     * this pass and all sub-passes.
     */
    void print_strategy() const;

    /**
     * Returns the string printed by print_strategy().
     */
    std::string dump_strategy() const;

    /**
     * Sets an option. Periods may be used as hierarchy separators to set
     * options for sub-passes; the last element will be the option name, and the
     * preceding elements represent pass instance names. Furthermore, wildcards
     * may be used for the pass name elements (asterisks for zero or more
     * characters and a question mark for a single character) to select multiple
     * or all immediate sub-passes of that group, and a double asterisk may be
     * used for the element before the option name to chain to
     * set_option_recursively() instead. The return value is the number of
     * passes that were affected; passes are only affected when they are
     * selected by the option path AND have an option with the specified name.
     * If must_exist is set an exception will be thrown if none of the passes
     * were affected, otherwise 0 will be returned.
     */
    size_t set_option(
        const std::string &option,
        const std::string &value,
        bool must_exist = true
    );

    /**
     * Sets an option for all sub-passes recursively. The return value is the
     * number of passes that were affected; passes are only affected when they
     * have an option with the specified name. If must_exist is set an exception
     * will be thrown if none of the passes were affected, otherwise 0 will be
     * returned.
     */
    size_t set_option_recursively(
        const std::string &option,
        const std::string &value,
        bool must_exist = true
    );

    /**
     * Returns the current value of an option. Periods may be used as hierarchy
     * separators to get options from sub-passes (if any).
     */
    std::string get_option(const std::string &option) const;

    /**
     * Constructs this pass. During construction, the pass implementation may
     * decide, based on its options, to become a group of passes or a normal
     * pass. If it decides to become a group, the group may be introspected or
     * modified by the user. The options are frozen after this, so set_option()
     * will start throwing exceptions when called. construct() may be called any
     * number of times, but becomes no-op after the first call.
     */
    void construct();

    /**
     * Returns whether this pass has been constructed yet.
     */
    bool is_constructed() const;

    /**
     * Returns whether this pass has configurable sub-passes.
     */
    bool is_group() const;

    /**
     * Returns whether this pass is a simple group of which the sub-passes can
     * be collapsed into the parent pass group without affecting the strategy.
     */
    bool is_collapsible() const;

    /**
     * Returns whether this is the root pass group in a pass manager.
     */
    bool is_root() const;

    /**
     * Returns whether this pass transforms the platform tree.
     */
    bool is_platform_transformer() const;

    /**
     * Returns whether this pass contains a conditionally-executed group.
     */
    bool is_conditional() const;

    /**
     * If this pass constructed into a group of passes, appends a pass to the
     * end of its pass list. Otherwise, an exception is thrown. If type_name is
     * empty or unspecified, a generic subgroup is added. Returns a reference to
     * the constructed pass.
     */
    Pass append_sub_pass(
        const std::string &type_name = "",
        const std::string &instance_name = "",
        const std::map<std::string, std::string> &options = {}
    );

    /**
     * If this pass constructed into a group of passes, appends a pass to the
     * beginning of its pass list. Otherwise, an exception is thrown. If
     * type_name is empty or unspecified, a generic subgroup is added. Returns a
     * reference to the constructed pass.
     */
    Pass prefix_sub_pass(
        const std::string &type_name = "",
        const std::string &instance_name = "",
        const std::map<std::string, std::string> &options = {}
    );

    /**
     * If this pass constructed into a group of passes, inserts a pass
     * immediately after the target pass (named by instance). If target does not
     * exist or this pass is not a group of sub-passes, an exception is thrown.
     * If type_name is empty or unspecified, a generic subgroup is added.
     * Returns a reference to the constructed pass. Periods may be used in
     * target to traverse deeper into the pass hierarchy.
     */
    Pass insert_sub_pass_after(
        const std::string &target,
        const std::string &type_name = "",
        const std::string &instance_name = "",
        const std::map<std::string, std::string> &options = {}
    );

    /**
     * If this pass constructed into a group of passes, inserts a pass
     * immediately before the target pass (named by instance). If target does
     * not exist or this pass is not a group of sub-passes, an exception is
     * thrown. If type_name is empty or unspecified, a generic subgroup is
     * added. Returns a reference to the constructed pass. Periods may be used
     * in target to traverse deeper into the pass hierarchy.
     */
    Pass insert_sub_pass_before(
        const std::string &target,
        const std::string &type_name = "",
        const std::string &instance_name = "",
        const std::map<std::string, std::string> &options = {}
    );

    /**
     * If this pass constructed into a group of passes, looks for the pass with
     * the target instance name, and embeds it into a newly generated group. The
     * group will assume the name of the original pass, while the original pass
     * will be renamed as specified by sub_name. Note that this ultimately does
     * not modify the pass order. If target does not exist or this pass is not a
     * group of sub-passes, an exception is thrown. Returns a reference to the
     * constructed group. Periods may be used in target to traverse deeper into
     * the pass hierarchy.
     */
    Pass group_sub_pass(
        const std::string &target,
        const std::string &sub_name = "main"
    );

    /**
     * Like group_sub_pass(), but groups an inclusive range of passes into a
     * group with the given name, leaving the original pass names unchanged.
     * Periods may be used in from/to to traverse deeper into the pass
     * hierarchy, but the hierarchy prefix must be the same for from and to.
     */
    Pass group_sub_passes(
        const std::string &from,
        const std::string &to,
        const std::string &group_name
    );

    /**
     * If this pass constructed into a group of passes, looks for the pass with
     * the target instance name, treats it as a generic group, and flattens its
     * contained passes into the list of sub-passes of its parent. The names of
     * the passes found in the collapsed subgroup are prefixed with name_prefix
     * before they are added to the parent group. Note that this ultimately does
     * not modify the pass order. If target does not exist, does not construct
     * into a group of passes (construct() is called automatically), or this
     * pass is not a group of sub-passes, an exception is thrown. Periods may be
     * used in target to traverse deeper into the pass hierarchy.
     */
    void flatten_subgroup(
        const std::string &target,
        const std::string &name_prefix = ""
    );

    /**
     * If this pass constructed into a group of passes, returns a reference to
     * the pass with the given instance name. If target does not exist or this
     * pass is not a group of sub-passes, an exception is thrown. Periods may be
     * used as hierarchy separators to get nested sub-passes.
     */
    Pass get_sub_pass(const std::string &target) const;

    /**
     * If this pass constructed into a group of passes, returns whether a
     * sub-pass with the target instance name exists. Otherwise, an exception is
     * thrown. Periods may be used in target to traverse deeper into the pass
     * hierarchy.
     */
    bool does_sub_pass_exist(const std::string &target) const;

    /**
     * If this pass constructed into a group of passes, returns the total number
     * of immediate sub-passes. Otherwise, an exception is thrown.
     */
    size_t get_num_sub_passes() const;

    /**
     * If this pass constructed into a group of passes, returns a reference to
     * the list containing all the sub-passes. Otherwise, an exception is
     * thrown.
     */
    std::vector<Pass> get_sub_passes() const;

    /**
     * If this pass constructed into a group of passes, returns an indexable
     * list of references to all immediate sub-passes with the given type.
     * Otherwise, an exception is thrown.
     */
    std::vector<Pass> get_sub_passes_by_type(const std::string &target) const;

    /**
     * If this pass constructed into a group of passes, removes the sub-pass
     * with the target instance name. If target does not exist or this pass is
     * not a group of sub-passes, an exception is thrown. Periods may be used in
     * target to traverse deeper into the pass hierarchy.
     */
    void remove_sub_pass(const std::string &target);

    /**
     * If this pass constructed into a group of passes, removes all sub-passes.
     * Otherwise, an exception is thrown.
     */
    void clear_sub_passes();

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
     * User-given name for this compiler.
     *
     * NOTE: not actually used for anything. It's only here for consistency with
     * the rest of the API objects.
     */
    std::string name;

    /**
     * Creates an empty compiler, with no specified architecture.
     */
    explicit Compiler(const std::string &name="");

    /**
     * Creates a compiler configuration from the given JSON file.
     */
    explicit Compiler(const std::string &name, const std::string &filename);

    /**
     * Creates a default compiler for the given platform.
     */
    explicit Compiler(const std::string &name, const Platform &platform);

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

/**
 * Represents a classical 32-bit integer register.
 */
class CReg {
private:
    friend class Operation;
    friend class Kernel;

    /**
     * The wrapped control register object.
     */
    ql::utils::Ptr<ql::ir::ClassicalRegister> creg;

public:

    /**
     * Creates a register with the given index.
     */
    explicit CReg(size_t id);

};

/**
 * Represents a classical operation.
 */
class Operation {
private:
    friend class Kernel;
    friend class Program;

    /**
     * The wrapped classical operation object.
     */
    ql::utils::Ptr<ql::ir::ClassicalOperation> operation;

public:

    /**
     * Creates a classical binary operation between two classical registers. The
     * operation is specified as a string, of which the following are supported:
     *  - "+": addition.
     *  - "-": subtraction.
     *  - "&": bitwise AND.
     *  - "|": bitwise OR.
     *  - "^": bitwise XOR.
     *  - "==": equality.
     *  - "!=": inequality.
     *  - ">": greater-than.
     *  - ">=": greater-or-equal.
     *  - "<": less-than.
     *  - "<=": less-or-equal.
     */
    Operation(const CReg &lop, const std::string &op, const CReg &rop);

    /**
     * Creates a classical unary operation on a register. The operation is
     * specified as a string, of which currently only "~" (bitwise NOT) is
     * supported.
     */
    Operation(const std::string &op, const CReg &rop);

    /**
     * Creates a classical "operation" that just returns the value of the given
     * register.
     */
    explicit Operation(const CReg &lop);

    /**
     * Creates a classical "operation" that just returns the given integer
     * value.
     */
    explicit Operation(int val);

};

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

    /**
     * The name given to the unitary gate.
     */
    const std::string name;

    /**
     * Creates a unitary matrix from the given row-major, square, unitary
     * matrix.
     */
    Unitary(const std::string &name, const std::vector<std::complex<double>> &matrix);

    /**
     * Explicitly decomposes the gate. Does not need to be called; it will be
     * called automatically when the gate is added to the kernel.
     */
    void decompose();

    /**
     * Returns whether OpenQL was built with unitary decomposition support
     * enabled.
     */
    static bool is_decompose_support_enabled();

};

/**
 * Represents a kernel of a quantum program, a.k.a. a basic block. Kernels are
 * just sequences of gates with no classical control-flow in between.
 */
class Kernel {
private:
    friend class Program;

    /**
     * The wrapped kernel object.
     */
    ql::ir::KernelRef kernel;

public:

    /**
     * The name of the kernel as given by the user.
     */
    const std::string name;

    /**
     * The platform that the kernel was built for.
     */
    const Platform platform;

    /**
     * The number of (virtual) qubits allocated for the kernel.
     */
    const size_t qubit_count;

    /**
     * The number of classical integer registers allocated for the kernel.
     */
    const size_t creg_count;

    /**
     * The number of classical bit registers allocated for the kernel.
     */
    const size_t breg_count;

    /**
     * Creates a new kernel with the given name, using the given platform.
     * The third, fourth, and fifth arguments optionally specify the desired
     * number of qubits, classical integer registers, and classical bit
     * registers. If not specified, the number of qubits is taken from the
     * platform, and no classical or bit registers will be allocated.
     */
    Kernel(
        const std::string &name,
        const Platform &platform,
        size_t qubit_count = 0,
        size_t creg_count = 0,
        size_t breg_count = 0
    );

    /**
     * Shorthand for an "identity" gate with a single qubit.
     */
    void identity(size_t q0);

    /**
     * Shorthand for a "hadamard" gate with a single qubit.
     */
    void hadamard(size_t q0);

    /**
     * Shorthand for a "s" gate with a single qubit.
     */
    void s(size_t q0);

    /**
     * Shorthand for a "sdag" gate with a single qubit.
     */
    void sdag(size_t q0);

    /**
     * Shorthand for a "t" gate with a single qubit.
     */
    void t(size_t q0);

    /**
     * Shorthand for a "tdag" gate with a single qubit.
     */
    void tdag(size_t q0);

    /**
     * Shorthand for a "x" gate with a single qubit.
     */
    void x(size_t q0);

    /**
     * Shorthand for a "y" gate with a single qubit.
     */
    void y(size_t q0);

    /**
     * Shorthand for a "z" gate with a single qubit.
     */
    void z(size_t q0);

    /**
     * Shorthand for an "rx90" gate with a single qubit.
     */
    void rx90(size_t q0);

    /**
     * Shorthand for an "mrx90" gate with a single qubit.
     */
    void mrx90(size_t q0);

    /**
     * Shorthand for an "rx180" gate with a single qubit.
     */
    void rx180(size_t q0);

    /**
     * Shorthand for an "ry90" gate with a single qubit.
     */
    void ry90(size_t q0);

    /**
     * Shorthand for an "mry90" gate with a single qubit.
     */
    void mry90(size_t q0);

    /**
     * Shorthand for an "ry180" gate with a single qubit.
     */
    void ry180(size_t q0);

    /**
     * Shorthand for an "rx" gate with a single qubit and the given rotation in
     * radians.
     */
    void rx(size_t q0, double angle);

    /**
     * Shorthand for an "ry" gate with a single qubit and the given rotation in
     * radians.
     */
    void ry(size_t q0, double angle);

    /**
     * Shorthand for an "rz" gate with a single qubit and the given rotation in
     * radians.
     */
    void rz(size_t q0, double angle);

    /**
     * Shorthand for a "measure" gate with a single qubit and implicit result
     * bit register.
     */
    void measure(size_t q0);

    /**
     * Shorthand for a "measure" gate with a single qubit and explicit result
     * bit register.
     */
    void measure(size_t q0, size_t b0);

    /**
     * Shorthand for a "prepz" gate with a single qubit.
     */
    void prepz(size_t q0);

    /**
     * Shorthand for a "cnot" gate with two qubits.
     */
    void cnot(size_t q0, size_t q1);

    /**
     * Shorthand for a "cphase" gate with two qubits.
     */
    void cphase(size_t q0, size_t q1);

    /**
     * Shorthand for a "cz" gate with two qubits.
     */
    void cz(size_t q0, size_t q1);

    /**
     * Shorthand for a "toffoli" gate with three qubits.
     */
    void toffoli(size_t q0, size_t q1, size_t q2);

    /**
     * Shorthand for the Clifford gate with the specific number using the
     * minimal number of rx90, rx180, mrx90, ry90, ry180, mry90 and Y gates.
     * These are as follows:
     *
     *  - 0: no gates inserted.
     *  - 1: ry90; rx90
     *  - 2: mrx90, mry90
     *  - 3: rx180
     *  - 4: mry90, mrx90
     *  - 5: rx90, mry90
     *  - 6: ry180
     *  - 7: mry90, rx90
     *  - 8: rx90, ry90
     *  - 9: rx180, ry180
     *  - 10: ry90, mrx90
     *  - 11: mrx90, ry90
     *  - 12: ry90, rx180
     *  - 13: mrx90
     *  - 14: rx90, mry90, mrx90
     *  - 15: mry90
     *  - 16: rx90
     *  - 17: rx90, ry90, rx90
     *  - 18: mry90, rx180
     *  - 19: rx90, ry180
     *  - 20: rx90, mry90, rx90
     *  - 21: ry90
     *  - 22: mrx90, ry180
     *  - 23: rx90, ry90, mrx90
     */
    void clifford(int id, size_t q0);

    /**
     * Shorthand for a "wait" gate with the specified qubits and duration in
     * nanoseconds. If no qubits are specified, the wait applies to all qubits
     * instead (a wait with no qubits is meaningless). Note that the duration
     * will usually end up being rounded up to multiples of the platform's cycle
     * time.
     */
    void wait(const std::vector<size_t> &qubits, size_t duration);

    /**
     * Shorthand for a "wait" gate with the specified qubits and duration 0. If
     * no qubits are specified, the wait applies to all qubits instead (a wait
     * with no qubits is meaningless).
     */
    void barrier(const std::vector<size_t> &qubits = std::vector<size_t>());

    /**
     * Returns a newline-separated list of all custom gates supported by the
     * platform.
     */
    std::string get_custom_instructions() const;

    /**
     * Shorthand for a "display" gate with no qubits.
     */
    void display();

    /**
     * Shorthand for the given gate name with a single qubit.
     */
    void gate(const std::string &gname, size_t q0);

    /**
     * Shorthand for the given gate name with two qubits.
     */
    void gate(const std::string &gname, size_t q0, size_t q1);

    /**
     * Main function for adding arbitrary quantum gates.
     *
     * Note that OpenQL currently uses string comparisons with gate names all
     * over the place to derive functionality, and to derive what the actual
     * arguments do. This is inherently a bad idea and something we want to
     * move away from, so documenting it all would not be worthwhile.
     *
     * For conditional gates, the following condition strings are supported:
     *
     *  - "COND_ALWAYS" or "1": no condition; gate is always executed.
     *  - "COND_NEVER" or "0": no condition; gate is never executed.
     *  - "COND_UNARY" or "" (empty): gate is executed if the single bit
     *    specified via condregs is 1.
     *  - "COND_NOT" or "!": gate is executed if the single bit specified via
     *    condregs is 0.
     *  - "COND_AND" or "&": gate is executed if the two bits specified via
     *    condregs are both 1.
     *  - "COND_NAND" or "!&": gate is executed if either of the two bits
     *    specified via condregs is zero.
     *  - "COND_OR" or "|": gate is executed if either of the two bits specified
     *    via condregs is one.
     *  - "COND_NOR" or "1": no condition; gate is always executed.
     */
    void gate(
        const std::string &name,
        const std::vector<size_t> &qubits,
        size_t duration = 0,
        double angle = 0.0,
        const std::vector<size_t> &bregs = {},
        const std::string &condstring = "COND_ALWAYS",
        const std::vector<size_t> &condregs = {}
    );

    /**
     * Alternative function for adding normal conditional quantum gates. Avoids
     * having to specify duration, angle, and bregs.
     */
    void condgate(
        const std::string &name,
        const std::vector<size_t> &qubits,
        const std::string &condstring,
        const std::vector<size_t> &condregs
    );

    /**
     * Main function for mixed quantum-classical gates involving integer
     * registers.
     */
    void gate(
        const std::string &name,
        const std::vector<size_t> &qubits,
        const CReg &destination
    );

    /**
     * Adds a unitary gate to the circuit. The size of the unitary gate must of
     * course align with the number of qubits presented.
     */
    void gate(const Unitary &u, const std::vector<size_t> &qubits);

    /**
     * Adds a classical assignment gate to the circuit. The classical integer
     * register is assigned to the result of the given operation.
     */
    void classical(const CReg &destination, const Operation &operation);

    /**
     * Adds a classical gate without operands. Only "nop" is currently (more or
     * less) supported.
     */
    void classical(const std::string &operation);

    /**
     * Sets the condition for all gates subsequently added to this kernel.
     * Thus, essentially shorthand notation. Reset with gate_clear_condition().
     */
    void gate_preset_condition(
        const std::string &condstring,
        const std::vector<size_t> &condregs
    );

    /**
     * Clears a condition previously set via gate_preset_condition().
     */
    void gate_clear_condition();

    /**
     * Adds a controlled kernel. The number of control and ancilla qubits must
     * be equal.
     *
     * NOTE: this high-level functionality is poorly/not maintained, and relies
     * on default gates, which are on the list for removal.
     */
    void controlled(
        const Kernel &k,
        const std::vector<size_t> &control_qubits,
        const std::vector<size_t> &ancilla_qubits
    );

    /**
     * Adds the conjugate of the given kernel to this kernel.
     *
     * NOTE: this high-level functionality is poorly/not maintained, and relies
     * on default gates, which are on the list for removal.
     */
    void conjugate(const Kernel &k);
};

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

/**
 * cQASM reader interface.
 */
class cQasmReader {
private:

    /**
     * The wrapped cQASM reader.
     */
    ql::utils::Ptr<ql::pass::io::cqasm::read::Reader> cqasm_reader;

public:

    /**
     * The platform associated with the reader.
     */
    const Platform platform;

    /**
     * The program that the cQASM circuits will be added to.
     */
    const Program program;

    /**
     * Builds a cQASM reader for the given platform and program, optionally
     * using a custom instruction set configuration file. This is an old
     * interface; the platform argument is redundant.
     */
    cQasmReader(
        const Platform &platform,
        const Program &program,
        const std::string &gateset_fname = ""
    );

    /**
     * Builds a cQASM reader for the given program, optionally using a custom
     * instruction set configuration file.
     */
    cQasmReader(
        const Program &program,
        const std::string &gateset_fname = ""
    );

    /**
     * Interprets a string as cQASM file and adds its contents to the program
     * associated with this reader.
     */
    void string2circuit(const std::string &cqasm_str);

    /**
     * Interprets a cQASM file and adds its contents to the program associated
     * with this reader.
     */
    void file2circuit(const std::string &cqasm_file_path);

};

} // namespace api
} // namespace ql
