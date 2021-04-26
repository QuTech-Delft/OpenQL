/** \file
 * API header for accessing the compiler's pass management logic.
 */

#include "ql/api/compiler.h"

#include "ql/api/misc.h"
#include "ql/api/platform.h"
#include "ql/api/program.h"
#include "ql/api/pass.h"

//============================================================================//
//                               W A R N I N G                                //
//----------------------------------------------------------------------------//
//         Docstrings in this file must manually be kept in sync with         //
//     compiler.i! This should be automated at some point, but isn't yet.     //
//============================================================================//

namespace ql {
namespace api {

/**
 * Constructor used internally to build a compiler object that belongs to
 * a platform.
 */
Compiler::Compiler(
    const ql::pmgr::Ref &pass_manager
) :
    pass_manager(pass_manager),
    name("default")
{ }

/**
 * Creates an empty compiler, with no specified architecture.
 */
Compiler::Compiler(
    const std::string &name
) :
    pass_manager(ql::utils::Ptr<ql::pmgr::Manager>::make()),
    name(name)
{
    ensure_initialized();
}

/**
 * Creates a compiler configuration from the given JSON file.
 */
Compiler::Compiler(
    const std::string &name,
    const std::string &filename
) :
    pass_manager(ql::pmgr::Manager::from_json(ql::utils::load_json(filename))),
    name(name)
{
    ensure_initialized();
}

/**
 * Creates a default compiler for the given platform.
 */
Compiler::Compiler(
    const std::string &name,
    const Platform &platform
) :
    pass_manager(ql::pmgr::Manager::from_defaults(platform.platform)),
    name(name)
{
    ensure_initialized();
}

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
    return Pass(pass_manager->append_pass(type_name, instance_name, options), false);
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
    return Pass(pass_manager->prefix_pass(type_name, instance_name, options), false);
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
    return Pass(pass_manager->insert_pass_after(target, type_name, instance_name, options), false);
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
    return Pass(pass_manager->insert_pass_before(target, type_name, instance_name, options), false);
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
    return Pass(pass_manager->group_pass(target, sub_name), false);
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
    return Pass(pass_manager->group_passes(from, to, group_name), false);
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
    return Pass(pass_manager->get_pass(target), false);
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
 * Returns a vector with references to all passes in the root hierarchy.
 */
std::vector<Pass> Compiler::get_passes() const {
    std::vector<Pass> retval;
    for (const auto &pass : pass_manager->get_passes()) {
        retval.push_back(Pass(pass, false));
    }
    return retval;
}

/**
 * Returns an indexable list of references to all passes with the given
 * type within the root hierarchy.
 */
std::vector<Pass> Compiler::get_passes_by_type(const std::string &target) const {
    std::vector<Pass> retval;
    for (const auto &pass : pass_manager->get_sub_passes_by_type(target)) {
        retval.push_back(Pass(pass, false));
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
 * program is referencing the same compiler.
 */
void Compiler::compile(const Program &program) {
    pass_manager->compile(program.program);
}

} // namespace api
} // namespace ql
