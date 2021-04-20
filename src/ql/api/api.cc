/** \file
 * Implementation for Python interface classes.
 */

#include "ql/api/api.h"

#include "ql/version.h"
#include "ql/com/interaction_matrix.h"
#include "ql/pass/io/sweep_points/annotation.h"
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
static void ensure_initialized() {
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

/**
 * Constructor used internally to build a pass object that belongs to
 * a compiler.
 */
Pass::Pass(const ql::pmgr::PassRef &pass) : pass(pass) {
}

/**
 * Returns the full, desugared type name that this pass was constructed
 * with.
 */
const std::string &Pass::get_type() const {
    return pass->get_type();
}

/**
 * Returns the instance name of the pass within the surrounding group.
 */
const std::string &Pass::get_name() const {
    return pass->get_name();
}

/**
 * Prints the documentation for this pass.
 */
void Pass::print_pass_documentation() const {
    pass->dump_help();
}

/**
 * Returns the documentation for this pass as a string.
 */
std::string Pass::get_pass_documentation() const {
    std::ostringstream ss;
    pass->dump_help(ss);
    return ss.str();
}

/**
 * Prints the current state of the options. If only_set is set to true, only
 * the options that were explicitly configured are dumped.
 */
void Pass::print_options(bool only_set) const {
    pass->dump_options(only_set);
}

/**
 * Returns the string printed by print_options().
 */
std::string Pass::get_options(bool only_set) const {
    std::ostringstream ss;
    pass->dump_help(ss);
    return ss.str();
}

/**
 * Prints the entire compilation strategy including configured options of
 * this pass and all sub-passes.
 */
void Pass::print_strategy() const {
    pass->dump_strategy();
}

/**
 * Returns the string printed by print_strategy().
 */
std::string Pass::dump_strategy() const {
    std::ostringstream ss;
    pass->dump_strategy(ss);
    return ss.str();
}

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
size_t Pass::set_option(
    const std::string &option,
    const std::string &value,
    bool must_exist
) {
    return pass->set_option(option, value, must_exist);
}

/**
 * Sets an option for all sub-passes recursively. The return value is the
 * number of passes that were affected; passes are only affected when they
 * have an option with the specified name. If must_exist is set an exception
 * will be thrown if none of the passes were affected, otherwise 0 will be
 * returned.
 */
size_t Pass::set_option_recursively(
    const std::string &option,
    const std::string &value,
    bool must_exist
) {
    return pass->set_option_recursively(option, value, must_exist);
}

/**
 * Returns the current value of an option. Periods may be used as hierarchy
 * separators to get options from sub-passes (if any).
 */
std::string Pass::get_option(const std::string &option) const {
    return pass->get_option(option).as_str();
}

/**
 * Constructs this pass. During construction, the pass implementation may
 * decide, based on its options, to become a group of passes or a normal
 * pass. If it decides to become a group, the group may be introspected or
 * modified by the user. The options are frozen after this, so set_option()
 * will start throwing exceptions when called. construct() may be called any
 * number of times, but becomes no-op after the first call.
 */
void Pass::construct() {
    return pass->construct();
}

/**
 * Returns whether this pass has been constructed yet.
 */
bool Pass::is_constructed() const {
    return pass->is_constructed();
}

/**
 * Returns whether this pass has configurable sub-passes.
 */
bool Pass::is_group() const {
    return pass->is_group();
}

/**
 * Returns whether this pass is a simple group of which the sub-passes can
 * be collapsed into the parent pass group without affecting the strategy.
 */
bool Pass::is_collapsible() const {
    return pass->is_collapsible();
}

/**
 * Returns whether this is the root pass group in a pass manager.
 */
bool Pass::is_root() const {
    return pass->is_root();
}

/**
 * Returns whether this pass transforms the platform tree.
 */
bool Pass::is_platform_transformer() const {
    return pass->is_platform_transformer();
}

/**
 * Returns whether this pass contains a conditionally-executed group.
 */
bool Pass::is_conditional() const {
    return pass->is_conditional();
}

/**
 * If this pass constructed into a group of passes, appends a pass to the
 * end of its pass list. Otherwise, an exception is thrown. If type_name is
 * empty or unspecified, a generic subgroup is added. Returns a reference to
 * the constructed pass.
 */
Pass Pass::append_sub_pass(
    const std::string &type_name,
    const std::string &instance_name,
    const std::map<std::string, std::string> &options
) {
    return Pass(pass->append_sub_pass(type_name, instance_name, options));
}

/**
 * If this pass constructed into a group of passes, appends a pass to the
 * beginning of its pass list. Otherwise, an exception is thrown. If
 * type_name is empty or unspecified, a generic subgroup is added. Returns a
 * reference to the constructed pass.
 */
Pass Pass::prefix_sub_pass(
    const std::string &type_name,
    const std::string &instance_name,
    const std::map<std::string, std::string> &options
) {
    return Pass(pass->prefix_sub_pass(type_name, instance_name, options));
}

/**
 * If this pass constructed into a group of passes, inserts a pass
 * immediately after the target pass (named by instance). If target does not
 * exist or this pass is not a group of sub-passes, an exception is thrown.
 * If type_name is empty or unspecified, a generic subgroup is added.
 * Returns a reference to the constructed pass. Periods may be used in
 * target to traverse deeper into the pass hierarchy.
 */
Pass Pass::insert_sub_pass_after(
    const std::string &target,
    const std::string &type_name,
    const std::string &instance_name,
    const std::map<std::string, std::string> &options
) {
    return Pass(pass->insert_sub_pass_after(target, type_name, instance_name, options));
}

/**
 * If this pass constructed into a group of passes, inserts a pass
 * immediately before the target pass (named by instance). If target does
 * not exist or this pass is not a group of sub-passes, an exception is
 * thrown. If type_name is empty or unspecified, a generic subgroup is
 * added. Returns a reference to the constructed pass. Periods may be used
 * in target to traverse deeper into the pass hierarchy.
 */
Pass Pass::insert_sub_pass_before(
    const std::string &target,
    const std::string &type_name,
    const std::string &instance_name,
    const std::map<std::string, std::string> &options
) {
    return Pass(pass->insert_sub_pass_before(target, type_name, instance_name, options));
}

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
Pass Pass::group_sub_pass(
    const std::string &target,
    const std::string &sub_name
) {
    return Pass(pass->group_sub_pass(target, sub_name));
}

/**
 * Like group_sub_pass(), but groups an inclusive range of passes into a
 * group with the given name, leaving the original pass names unchanged.
 * Periods may be used in from/to to traverse deeper into the pass
 * hierarchy, but the hierarchy prefix must be the same for from and to.
 */
Pass Pass::group_sub_passes(
    const std::string &from,
    const std::string &to,
    const std::string &group_name
) {
    return Pass(pass->group_sub_passes(from, to, group_name));
}

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
void Pass::flatten_subgroup(
    const std::string &target,
    const std::string &name_prefix
) {
    pass->flatten_subgroup(target, name_prefix);
}

/**
 * If this pass constructed into a group of passes, returns a reference to
 * the pass with the given instance name. If target does not exist or this
 * pass is not a group of sub-passes, an exception is thrown. Periods may be
 * used as hierarchy separators to get nested sub-passes.
 */
Pass Pass::get_sub_pass(const std::string &target) const {
    return Pass(pass->get_sub_pass(target));
}

/**
 * If this pass constructed into a group of passes, returns whether a
 * sub-pass with the target instance name exists. Otherwise, an exception is
 * thrown. Periods may be used in target to traverse deeper into the pass
 * hierarchy.
 */
bool Pass::does_sub_pass_exist(const std::string &target) const {
    return pass->does_sub_pass_exist(target);
}

/**
 * If this pass constructed into a group of passes, returns the total number
 * of immediate sub-passes. Otherwise, an exception is thrown.
 */
size_t Pass::get_num_sub_passes() const {
    return pass->get_num_sub_passes();
}

/**
 * If this pass constructed into a group of passes, returns a reference to
 * the list containing all the sub-passes. Otherwise, an exception is
 * thrown.
 */
std::vector<Pass> Pass::get_sub_passes() const {
    std::vector<Pass> retval;
    for (const auto &p : pass->get_sub_passes()) {
        retval.push_back(Pass(p));
    }
    return retval;
}

/**
 * If this pass constructed into a group of passes, returns an indexable
 * list of references to all immediate sub-passes with the given type.
 * Otherwise, an exception is thrown.
 */
std::vector<Pass> Pass::get_sub_passes_by_type(const std::string &target) const {
    std::vector<Pass> retval;
    for (const auto &p : pass->get_sub_passes_by_type(target)) {
        retval.push_back(Pass(p));
    }
    return retval;
}

/**
 * If this pass constructed into a group of passes, removes the sub-pass
 * with the target instance name. If target does not exist or this pass is
 * not a group of sub-passes, an exception is thrown. Periods may be used in
 * target to traverse deeper into the pass hierarchy.
 */
void Pass::remove_sub_pass(const std::string &target) {
    pass->remove_sub_pass(target);
}

/**
 * If this pass constructed into a group of passes, removes all sub-passes.
 * Otherwise, an exception is thrown.
 */
void Pass::clear_sub_passes() {
    pass->clear_sub_passes();
}

/**
 * Constructor used internally to build a compiler object that belongs to
 * a platform.
 */
Compiler::Compiler(
    const ql::pmgr::Ref &pass_manager
) :
    pass_manager(pass_manager)
{ }

/**
 * Creates an empty compiler, with no specified architecture.
 */
Compiler::Compiler(
) :
    pass_manager(ql::utils::Ptr<ql::pmgr::Manager>::make())
{ }

/**
 * Creates a compiler configuration from the given JSON file.
 */
Compiler::Compiler(
    const std::string &fname
) :
    pass_manager(ql::pmgr::Manager::from_json(ql::utils::load_json(fname)))
{ }

/**
 * Creates a default compiler for the given platform.
 */
Compiler::Compiler(
    const Platform &platform
) :
    pass_manager(ql::pmgr::Manager::from_defaults(platform.platform))
{ }

/**
 * Prints documentation for all available pass types, as well as the option
 * documentation for the passes.
 */
void Compiler::print_pass_types() const {
    pass_manager->dump_pass_types();
}

/**
 * Returns documentation for all available pass types, as well as the option
 * documentation for the passes.
 */
std::string Compiler::get_pass_types() const {
    std::ostringstream ss;
    pass_manager->dump_pass_types(ss);
    return ss.str();
}

/**
 * Prints the currently configured compilation strategy.
 */
void Compiler::print_strategy() const {
    pass_manager->dump_strategy();
}

/**
 * Returns the currently configured compilation strategy as a string.
 */
std::string Compiler::get_strategy() const {
    std::ostringstream ss;
    pass_manager->dump_strategy(ss);
    return ss.str();
}

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
size_t Compiler::set_option(
    const std::string &path,
    const std::string &value,
    bool must_exist
) {
    return pass_manager->set_option(path, value, must_exist);
}

/**
 * Sets an option for all passes recursively. The return value is the number
 * of passes that were affected; passes are only affected when they have an
 * option with the specified name. If must_exist is set an exception will be
 * thrown if none of the passes were affected, otherwise 0 will be returned.
 */
size_t Compiler::set_option_recursively(
    const std::string &option,
    const std::string &value,
    bool must_exist
) {
    return pass_manager->set_option_recursively(option, value, must_exist);
}

/**
 * Returns the current value of an option. Periods are used as hierarchy
 * separators; the last element will be the option name, and the preceding
 * elements represent pass instance names.
 */
std::string Compiler::get_option(const std::string &path) const {
    return pass_manager->get_option(path).as_str();
}

/**
 * Appends a pass to the end of the pass list. If type_name is empty
 * or unspecified, a generic subgroup is added. Returns a reference to the
 * constructed pass.
 */
Pass Compiler::append_pass(
    const std::string &type_name,
    const std::string &instance_name,
    const std::map<std::string, std::string> &options
) {
    return Pass(pass_manager->append_pass(type_name, instance_name, options));
}

/**
 * Appends a pass to the beginning of the pass list. If type_name is empty
 * or unspecified, a generic subgroup is added. Returns a reference to the
 * constructed pass.
 */
Pass Compiler::prefix_pass(
    const std::string &type_name,
    const std::string &instance_name,
    const std::map<std::string, std::string> &options
) {
    return Pass(pass_manager->prefix_pass(type_name, instance_name, options));
}

/**
 * Inserts a pass immediately after the target pass (named by instance). If
 * target does not exist, an exception is thrown. If type_name is empty or
 * unspecified, a generic subgroup is added. Returns a reference to the
 * constructed pass. Periods may be used in target to traverse deeper into
 * the pass hierarchy.
 */
Pass Compiler::insert_pass_after(
    const std::string &target,
    const std::string &type_name,
    const std::string &instance_name,
    const std::map<std::string, std::string> &options
) {
    return Pass(pass_manager->insert_pass_after(target, type_name, instance_name, options));
}

/**
 * Inserts a pass immediately before the target pass (named by instance). If
 * target does not exist, an exception is thrown. If type_name is empty or
 * unspecified, a generic subgroup is added. Returns a reference to the
 * constructed pass. Periods may be used in target to traverse deeper into
 * the pass hierarchy.
 */
Pass Compiler::insert_pass_before(
    const std::string &target,
    const std::string &type_name,
    const std::string &instance_name,
    const std::map<std::string, std::string> &options
) {
    return Pass(pass_manager->insert_pass_before(target, type_name, instance_name, options));
}

/**
 * Looks for the pass with the target instance name, and embeds it into a
 * newly generated group. The group will assume the name of the original
 * pass, while the original pass will be renamed as specified by sub_name.
 * Note that this ultimately does not modify the pass order. If target does
 * not exist or this pass is not a group of sub-passes, an exception is
 * thrown. Returns a reference to the constructed group. Periods may be used
 * in target to traverse deeper into the pass hierarchy.
 */
Pass Compiler::group_pass(
    const std::string &target,
    const std::string &sub_name
) {
    return Pass(pass_manager->group_pass(target, sub_name));
}

/**
 * Like group_pass(), but groups an inclusive range of passes into a
 * group with the given name, leaving the original pass names unchanged.
 * Periods may be used in from/to to traverse deeper into the pass
 * hierarchy, but the hierarchy prefix must be the same for from and to.
 */
Pass Compiler::group_passes(
    const std::string &from,
    const std::string &to,
    const std::string &group_name
) {
    return Pass(pass_manager->group_passes(from, to, group_name));
}

/**
 * Looks for an unconditional pass group with the target instance name and
 * flattens its contained passes into its parent group. The names of the
 * passes found in the collapsed group are prefixed with name_prefix before
 * they are added to the parent group. Note that this ultimately does not
 * modify the pass order. If the target instance name does not exist or is
 * not an unconditional group, an exception is thrown. Periods may be used
 * in target to traverse deeper into the pass hierarchy.
 */
void Compiler::flatten_subgroup(
    const std::string &target,
    const std::string &name_prefix
) {
    pass_manager->flatten_subgroup(target, name_prefix);
}

/**
 * Returns a reference to the pass with the given instance name. If no such
 * pass exists, an exception is thrown. Periods may be used as hierarchy
 * separators to get nested sub-passes.
 */
Pass Compiler::get_pass(const std::string &target) const {
    return Pass(pass_manager->get_pass(target));
}

/**
 * Returns whether a pass with the target instance name exists. Periods may
 * be used in target to traverse deeper into the pass hierarchy.
 */
bool Compiler::does_pass_exist(const std::string &target) const {
    return pass_manager->does_pass_exist(target);
}

/**
 * Returns the total number of passes in the root hierarchy.
 */
size_t Compiler::get_num_passes() const {
    return pass_manager->get_num_passes();
}

/**
 * If this pass constructed into a group of passes, returns a reference to
 * the list containing all the sub-passes. Otherwise, an exception is
 * thrown.
 */
std::vector<Pass> Compiler::get_passes() const {
    std::vector<Pass> retval;
    for (const auto &pass : pass_manager->get_passes()) {
        retval.push_back(Pass(pass));
    }
    return retval;
}

/**
 * Returns an indexable list of references to all passes with the given
 * type within the root hierarchy.
 */
std::vector<Pass> Compiler::get_sub_passes_by_type(const std::string &target) const {
    std::vector<Pass> retval;
    for (const auto &pass : pass_manager->get_sub_passes_by_type(target)) {
        retval.push_back(Pass(pass));
    }
    return retval;
}

/**
 * Removes the pass with the given target instance name, or throws an
 * exception if no such pass exists.
 */
void Compiler::remove_pass(const std::string &target) {
    pass_manager->remove_pass(target);
}

/**
 * Clears the entire pass list.
 */
void Compiler::clear_passes() {
    pass_manager->clear_passes();
}

/**
 * Constructs all passes recursively. This freezes the pass options, but
 * allows subtrees to be modified.
 */
void Compiler::construct() {
    pass_manager->construct();
}

/**
 * Ensures that all passes have been constructed, and then runs the passes
 * on the given program. This is the same as Program.compile() when the
 * program's platform is referencing the same compiler
 */
void Compiler::compile(const Program &program) {
    pass_manager->compile(program.program);
}

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
 * Whether a custom compiler configuration has been attached to this
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

/**
 * Creates a register with the given index.
 */
CReg::CReg(size_t id) {
    creg.emplace(id);
}

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
Operation::Operation(const CReg &lop, const std::string &op, const CReg &rop) {
    operation.emplace(*(lop.creg), op, *(rop.creg));
}

/**
 * Creates a classical unary operation on a register. The operation is
 * specified as a string, of which currently only "~" (bitwise NOT) is
 * supported.
 */
Operation::Operation(const std::string &op, const CReg &rop) {
    operation.emplace(op, *(rop.creg));
}

/**
 * Creates a classical "operation" that just returns the value of the given
 * register.
 */
Operation::Operation(const CReg &lop) {
    operation.emplace(*(lop.creg));
}

/**
 * Creates a classical "operation" that just returns the given integer
 * value.
 */
Operation::Operation(int val) {
    operation.emplace(val);
}

/**
 * Creates a unitary matrix from the given row-major, square, unitary
 * matrix.
 */
Unitary::Unitary(
    const std::string &name,
    const std::vector<std::complex<double>> &matrix
) :
    name(name)
{
    unitary.emplace(name, ql::utils::Vec<ql::utils::Complex>(matrix.begin(), matrix.end()));
}

/**
 * Explicitly decomposes the gate. Does not need to be called; it will be
 * called automatically when the gate is added to the kernel.
 */
void Unitary::decompose() {
    unitary->decompose();
}

/**
 * Returns whether OpenQL was built with unitary decomposition support
 * enabled.
 */
bool Unitary::is_decompose_support_enabled() {
    return ql::com::Unitary::is_decompose_support_enabled();
}

/**
 * Creates a new kernel with the given name, using the given platform.
 * The third, fourth, and fifth arguments optionally specify the desired
 * number of qubits, classical integer registers, and classical bit
 * registers. If not specified, the number of qubits is taken from the
 * platform, and no classical or bit registers will be allocated.
 */
Kernel::Kernel(
    const std::string &name,
    const Platform &platform,
    size_t qubit_count,
    size_t creg_count,
    size_t breg_count
) :
    name(name),
    platform(platform),
    qubit_count(qubit_count),
    creg_count(creg_count),
    breg_count(breg_count)
{
    if (!qubit_count) qubit_count = platform.platform->qubit_count;
    kernel.emplace(name, platform.platform, qubit_count, creg_count, breg_count);
}

/**
 * Shorthand for an "identity" gate with a single qubit.
 */
void Kernel::identity(size_t q0) {
    kernel->identity(q0);
}

/**
 * Shorthand for a "hadamard" gate with a single qubit.
 */
void Kernel::hadamard(size_t q0) {
    kernel->hadamard(q0);
}

/**
 * Shorthand for a "s" gate with a single qubit.
 */
void Kernel::s(size_t q0) {
    kernel->s(q0);
}

/**
 * Shorthand for a "sdag" gate with a single qubit.
 */
void Kernel::sdag(size_t q0) {
    kernel->sdag(q0);
}

/**
 * Shorthand for a "t" gate with a single qubit.
 */
void Kernel::t(size_t q0) {
    kernel->t(q0);
}

/**
 * Shorthand for a "tdag" gate with a single qubit.
 */
void Kernel::tdag(size_t q0) {
    kernel->tdag(q0);
}

/**
 * Shorthand for a "x" gate with a single qubit.
 */
void Kernel::x(size_t q0) {
    kernel->x(q0);
}

/**
 * Shorthand for a "y" gate with a single qubit.
 */
void Kernel::y(size_t q0) {
    kernel->y(q0);
}

/**
 * Shorthand for a "z" gate with a single qubit.
 */
void Kernel::z(size_t q0) {
    kernel->z(q0);
}

/**
 * Shorthand for an "rx90" gate with a single qubit.
 */
void Kernel::rx90(size_t q0) {
    kernel->rx90(q0);
}

/**
 * Shorthand for an "mrx90" gate with a single qubit.
 */
void Kernel::mrx90(size_t q0) {
    kernel->mrx90(q0);
}

/**
 * Shorthand for an "rx180" gate with a single qubit.
 */
void Kernel::rx180(size_t q0) {
    kernel->rx180(q0);
}

/**
 * Shorthand for an "ry90" gate with a single qubit.
 */
void Kernel::ry90(size_t q0) {
    kernel->ry90(q0);
}

/**
 * Shorthand for an "mry90" gate with a single qubit.
 */
void Kernel::mry90(size_t q0) {
    kernel->mry90(q0);
}

/**
 * Shorthand for an "ry180" gate with a single qubit.
 */
void Kernel::ry180(size_t q0) {
    kernel->ry180(q0);
}

/**
 * Shorthand for an "rx" gate with a single qubit and the given rotation in
 * radians.
 */
void Kernel::rx(size_t q0, double angle) {
    kernel->rx(q0, angle);
}

/**
 * Shorthand for an "ry" gate with a single qubit and the given rotation in
 * radians.
 */
void Kernel::ry(size_t q0, double angle) {
    kernel->ry(q0, angle);
}

/**
 * Shorthand for an "rz" gate with a single qubit and the given rotation in
 * radians.
 */
void Kernel::rz(size_t q0, double angle) {
    kernel->rz(q0, angle);
}

/**
 * Shorthand for a "measure" gate with a single qubit and implicit result
 * bit register.
 */
void Kernel::measure(size_t q0) {
    QL_DOUT("Python k.measure([" << q0 << "])");
    kernel->measure(q0);
}

/**
 * Shorthand for a "measure" gate with a single qubit and explicit result
 * bit register.
 */
void Kernel::measure(size_t q0, size_t b0) {
    QL_DOUT("Python k.measure([" << q0 << "], [" << b0 << "])");
    kernel->measure(q0, b0);
}

/**
 * Shorthand for a "prepz" gate with a single qubit.
 */
void Kernel::prepz(size_t q0) {
    kernel->prepz(q0);
}

/**
 * Shorthand for a "cnot" gate with two qubits.
 */
void Kernel::cnot(size_t q0, size_t q1) {
    kernel->cnot(q0,q1);
}

/**
 * Shorthand for a "cphase" gate with two qubits.
 */
void Kernel::cphase(size_t q0, size_t q1) {
    kernel->cphase(q0,q1);
}

/**
 * Shorthand for a "cz" gate with two qubits.
 */
void Kernel::cz(size_t q0, size_t q1) {
    kernel->cz(q0,q1);
}

/**
 * Shorthand for a "toffoli" gate with three qubits.
 */
void Kernel::toffoli(size_t q0, size_t q1, size_t q2) {
    kernel->toffoli(q0,q1,q2);
}

/**
 * Shorthand for the Clifford gate with the specific number using the
 * minimal number of rx90, rx180, mrx90, ry90, ry180, mry90 and Y gates.
 */
void Kernel::clifford(int id, size_t q0) {
    kernel->clifford(id, q0);
}

/**
 * Shorthand for a "wait" gate with the specified qubits and duration in
 * nanoseconds. If no qubits are specified, the wait applies to all qubits
 * instead (a wait with no qubits is meaningless). Note that the duration
 * will usually end up being rounded up to multiples of the platform's cycle
 * time.
 */
void Kernel::wait(const std::vector<size_t> &qubits, size_t duration) {
    kernel->wait({qubits.begin(), qubits.end()}, duration);
}

/**
 * Shorthand for a "wait" gate with the specified qubits and duration 0. If
 * no qubits are specified, the wait applies to all qubits instead (a wait
 * with no qubits is meaningless).
 */
void Kernel::barrier(const std::vector<size_t> &qubits) {
    kernel->wait({qubits.begin(), qubits.end()}, 0);
}

/**
 * Returns a newline-separated list of all custom gates supported by the
 * platform.
 */
std::string Kernel::get_custom_instructions() const {
    return kernel->get_gates_definition();
}

/**
 * Shorthand for a "display" gate with no qubits.
 */
void Kernel::display() {
    kernel->display();
}

/**
 * Shorthand for the given gate name with a single qubit.
 */
void Kernel::gate(const std::string &gname, size_t q0) {
    kernel->gate(gname, q0);
}

/**
 * Shorthand for the given gate name with two qubits.
 */
void Kernel::gate(const std::string &gname, size_t q0, size_t q1) {
    kernel->gate(gname, q0, q1);
}

/**
 * Main function for adding arbitrary quantum gates.
 */
void Kernel::gate(
    const std::string &name,
    const std::vector<size_t> &qubits,
    size_t duration,
    double angle,
    const std::vector<size_t> &bregs,
    const std::string &condstring,
    const std::vector<size_t> &condregs
) {
    QL_DOUT(
        "Python k.gate("
        << name
        << ", "
        << ql::utils::Vec<size_t>(qubits.begin(), qubits.end())
        << ", "
        << duration
        << ", "
        << angle
        << ", "
        << ql::utils::Vec<size_t>(bregs.begin(), bregs.end())
        << ", "
        << condstring
        << ", "
        << ql::utils::Vec<size_t>(condregs.begin(), condregs.end())
        << ")"
    );
    auto condvalue = kernel->condstr2condvalue(condstring);

    kernel->gate(
        name,
        {qubits.begin(), qubits.end()},
        {},
        duration,
        angle,
        {bregs.begin(), bregs.end()},
        condvalue,
        {condregs.begin(), condregs.end()}
    );
}

/**
 * Alternative function for adding normal conditional quantum gates. Avoids
 * having to specify duration, angle, and bregs.
 */
void Kernel::condgate(
    const std::string &name,
    const std::vector<size_t> &qubits,
    const std::string &condstring,
    const std::vector<size_t> &condregs
) {
    QL_DOUT(
        "Python k.condgate("
        << name
        << ", "
        << ql::utils::Vec<size_t>(qubits.begin(), qubits.end())
        << ", "
        << condstring
        << ", "
        << ql::utils::Vec<size_t>(condregs.begin(), condregs.end())
        << ")"
    );
    kernel->condgate(
        name,
        {qubits.begin(), qubits.end()},
        kernel->condstr2condvalue(condstring),
        {condregs.begin(), condregs.end()}
    );
}

/**
 * Main function for mixed quantum-classical gates involving integer
 * registers.
 */
void Kernel::gate(
    const std::string &name,
    const std::vector<size_t> &qubits,
    const CReg &destination
) {
    QL_DOUT(
        "Python k.gate("
        << name
        << ", "
        << ql::utils::Vec<size_t>(qubits.begin(), qubits.end())
        << ",  "
        << (destination.creg)->id
        << ") # (name,qubits,creg-destination)"
    );
    kernel->gate(name, {qubits.begin(), qubits.end()}, {(destination.creg)->id} );
}

/**
 * Adds a unitary gate to the circuit. The size of the unitary gate must of
 * course align with the number of qubits presented.
 */
void Kernel::gate(const Unitary &u, const std::vector<size_t> &qubits) {
    kernel->gate(*(u.unitary), {qubits.begin(), qubits.end()});
}

/**
 * Adds a classical assignment gate to the circuit. The classical integer
 * register is assigned to the result of the given operation.
 */
void Kernel::classical(const CReg &destination, const Operation &operation) {
    kernel->classical(*(destination.creg), *(operation.operation));
}

/**
 * Adds a classical gate without operands. Only "nop" is currently (more or
 * less) supported.
 */
void Kernel::classical(const std::string &operation) {
    kernel->classical(operation);
}

/**
 * Sets the condition for all gates subsequently added to this kernel.
 * Thus, essentially shorthand notation. Reset with gate_clear_condition().
 */
void Kernel::gate_preset_condition(
    const std::string &condstring,
    const std::vector<size_t> &condregs
) {
    QL_DOUT("Python k.gate_preset_condition("<<condstring<<", condregs)");
    kernel->gate_preset_condition(
        kernel->condstr2condvalue(condstring),
        {condregs.begin(), condregs.end()}
    );
}

/**
 * Clears a condition previously set via gate_preset_condition().
 */
void Kernel::gate_clear_condition() {
    QL_DOUT("Python k.gate_clear_condition()");
    kernel->gate_clear_condition();
}

/**
 * Adds a controlled kernel. The number of control and ancilla qubits must
 * be equal.
 *
 * NOTE: this high-level functionality is poorly/not maintained, and relies
 * on default gates, which are on the list for removal.
 */
void Kernel::controlled(
    const Kernel &k,
    const std::vector<size_t> &control_qubits,
    const std::vector<size_t> &ancilla_qubits
) {
    kernel->controlled(*k.kernel, {control_qubits.begin(), control_qubits.end()}, {ancilla_qubits.begin(), ancilla_qubits.end()});
}

/**
 * Adds the conjugate of the given kernel to this kernel.
 *
 * NOTE: this high-level functionality is poorly/not maintained, and relies
 * on default gates, which are on the list for removal.
 */
void Kernel::conjugate(const Kernel &k) {
    kernel->conjugate(*k.kernel);
}

/**
 * Creates a new program with the given name, using the given platform.
 * The third, fourth, and fifth arguments optionally specify the desired
 * number of qubits, classical integer registers, and classical bit
 * registers. If not specified, the number of qubits is taken from the
 * platform, and no classical or bit registers will be allocated.
 */
Program::Program(
    const std::string &name,
    const Platform &platform,
    size_t qubit_count,
    size_t creg_count,
    size_t breg_count
) :
    name(name),
    platform(platform),
    qubit_count(qubit_count),
    creg_count(creg_count),
    breg_count(breg_count)
{
    if (!qubit_count) qubit_count = platform.platform->qubit_count;
    program.emplace(name, platform.platform, qubit_count, creg_count, breg_count);
}

/**
 * Adds an unconditionally-executed kernel to the end of the program.
 */
void Program::add_kernel(const Kernel &k) {
    program->add(k.kernel);
}

/**
 * Adds an unconditionally-executed subprogram to the end of the program.
 */
void Program::add_program(const Program &p) {
    program->add_program(p.program);
}

/**
 * Adds a conditionally-executed kernel to the end of the program. The
 * kernel will be executed if the given classical condition is true.
 */
void Program::add_if(const Kernel &k, const Operation &operation) {
    program->add_if(k.kernel, *operation.operation);
}

/**
 * Adds a conditionally-executed subprogram to the end of the program. The
 * kernel will be executed if the given classical condition evaluates to
 * true.
 */
void Program::add_if(const Program &p, const Operation &operation) {
    program->add_if(p.program, *operation.operation);
}

/**
 * Adds two conditionally-executed kernels with inverted conditions to the
 * end of the program. The first kernel will be executed if the given
 * classical condition evaluates to true; the second kernel will be executed
 * if it evaluates to false.
 */
void Program::add_if_else(const Kernel &k_if, const Kernel &k_else, const Operation &operation) {
    program->add_if_else(k_if.kernel, k_else.kernel, *(operation.operation));
}

/**
 * Adds two conditionally-executed subprograms with inverted conditions to
 * the end of the program. The first kernel will be executed if the given
 * classical condition evaluates to true; the second kernel will be executed
 * if it evaluates to false.
 */
void Program::add_if_else(const Program &p_if, const Program &p_else, const Operation &operation) {
    program->add_if_else(p_if.program, p_else.program, *(operation.operation));
}

/**
 * Adds a kernel that will be repeated until the given classical condition
 * evaluates to true. The kernel is executed at least once, since the
 * condition is evaluated at the end of the loop body.
 */
void Program::add_do_while(const Kernel &k, const Operation &operation) {
    program->add_do_while(k.kernel, *operation.operation);
}

/**
 * Adds a subprogram that will be repeated until the given classical
 * condition evaluates to true. The subprogram is executed at least once,
 * since the condition is evaluated at the end of the loop body.
 */
void Program::add_do_while(const Program &p, const Operation &operation) {
    program->add_do_while(p.program, *operation.operation);
}

/**
 * Adds an unconditionally-executed kernel that will loop for the given
 * number of iterations.
 */
void Program::add_for(const Kernel &k, size_t iterations) {
    program->add_for(k.kernel, iterations);
}

/**
 * Adds an unconditionally-executed subprogram that will loop for the given
 * number of iterations.
 */
void Program::add_for(const Program &p, size_t iterations) {
    program->add_for(p.program, iterations);
}

/**
 * Sets sweep point information for the program.
 */
void Program::set_sweep_points(const std::vector<double> &sweep_points) {
    QL_WOUT("The sweep points system is deprecated and may be removed at any time");
    using Annotation = ql::pass::io::sweep_points::Annotation;
    if (!program->has_annotation<Annotation>()) {
        program->set_annotation<Annotation>({});
    }
    program->get_annotation<Annotation>().data = sweep_points;
}

/**
 * Returns the configured sweep point information for the program.
 */
std::vector<double> Program::get_sweep_points() const {
    QL_WOUT("The sweep points system is deprecated and may be removed at any time");
    using Annotation = ql::pass::io::sweep_points::Annotation;
    auto annot = program->get_annotation_ptr<Annotation>();
    if (annot == nullptr) {
        return {};
    } else {
        return {annot->data.begin(), annot->data.end()};
    }
}

/**
 * Sets the name of the file that the sweep points will be written to.
 */
void Program::set_config_file(const std::string &config_file_name) {
    QL_WOUT("The sweep points system is deprecated and may be removed at any time");
    using Annotation = ql::pass::io::sweep_points::Annotation;
    if (!program->has_annotation<Annotation>()) {
        program->set_annotation<Annotation>({});
    }
    program->get_annotation<Annotation>().config_file_name
        = get_option("output_dir") + "/" + config_file_name;
}

/**
 * Whether a custom compiler configuration has been attached to this
 * program. When this is the case, it will be used to implement compile(),
 * rather than generating the compiler in-place from defaults and global
 * options during the call.
 */
bool Program::has_compiler() {
    return pass_manager.has_value();
}

/**
 * Returns the custom compiler configuration associated with this program.
 * If no such configuration exists yet, the default one is created,
 * attached, and returned.
 */
Compiler Program::get_compiler() {
    if (!pass_manager.has_value()) {
        pass_manager.emplace(ql::pmgr::Manager::from_defaults(program->platform));
    }
    return Compiler(pass_manager);
}

/**
 * Sets the compiler associated with this program. It will then be used for
 * compile().
 */
void Program::set_compiler(const Compiler &compiler) {
    pass_manager = compiler.pass_manager;
}

/**
 * Compiles the program.
 */
void Program::compile() {
    QL_IOUT("compiling " << name << " ...");
    QL_WOUT("compiling " << name << " ...");
    if (program->kernels.empty()) {
        QL_FATAL("compiling a program with no kernels !");
    }
    ql::pmgr::Manager::from_defaults(program->platform).compile(program);
}

/**
 * Prints the interaction matrix for each kernel in the program.
 */
void Program::print_interaction_matrix() const {
    QL_IOUT("printing interaction matrix...");

    ql::com::InteractionMatrix::dump_for_program(program);
}

/**
 * Writes the interaction matrix for each kernel in the program to a file.
 * This is one of the few functions that still uses the global output_dir
 * option.
 */
void Program::write_interaction_matrix() const {
    ql::com::InteractionMatrix::write_for_program(
        ql::com::options::get("output_dir") + "/",
        program
    );
}

/**
 * Builds a cQASM reader for the given platform and program, optionally
 * using a custom instruction set configuration file. This is an old
 * interface; the platform argument is redundant.
 */
cQasmReader::cQasmReader(
    const Platform &platform,
    const Program &program,
    const std::string &gateset_fname
) :
    platform(platform),
    program(program)
{
    if (platform.platform.get_ptr() != program.program->platform.get_ptr()) {
        throw ql::utils::Exception(
            "Mismatch between the given platform and the platform "
            "associated with the given program"
        );
    }
    if (gateset_fname.empty()) {
        cqasm_reader.emplace(platform.platform, program.program);
    } else {
        cqasm_reader.emplace(platform.platform, program.program, gateset_fname);
    }
}

/**
 * Builds a cQASM reader for the given program, optionally using a custom
 * instruction set configuration file.
 */
cQasmReader::cQasmReader(
    const Program &program,
    const std::string &gateset_fname
) :
    platform(program.platform),
    program(program)
{
    if (gateset_fname.empty()) {
        cqasm_reader.emplace(platform.platform, program.program);
    } else {
        cqasm_reader.emplace(platform.platform, program.program, gateset_fname);
    }
}

/**
 * Interprets a string as cQASM file and adds its contents to the program
 * associated with this reader.
 */
void cQasmReader::string2circuit(const std::string &cqasm_str) {
    cqasm_reader->string2circuit(cqasm_str);
}

/**
 * Interprets a cQASM file and adds its contents to the program associated
 * with this reader.
 */
void cQasmReader::file2circuit(const std::string &cqasm_file_path) {
    cqasm_reader->file2circuit(cqasm_file_path);
}

} // namespace api
} // namespace ql
