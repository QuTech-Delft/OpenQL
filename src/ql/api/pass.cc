/** \file
 * API header for modifying compiler pass parameters.
 */

#include "ql/api/pass.h"

//============================================================================//
//                               W A R N I N G                                //
//----------------------------------------------------------------------------//
//         Docstrings in this file must manually be kept in sync with         //
//       pass.i! This should be automated at some point, but isn't yet.       //
//============================================================================//

namespace ql {
namespace api {

/**
 * Constructor used internally to build a pass object that belongs to
 * a compiler.
 */
Pass::Pass(const ql::pmgr::PassRef &pass, bool dummy) : pass(pass) {
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
std::string Pass::get_strategy() const {
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
    return Pass(pass->append_sub_pass(type_name, instance_name, options), false);
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
    return Pass(pass->prefix_sub_pass(type_name, instance_name, options), false);
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
    return Pass(pass->insert_sub_pass_after(target, type_name, instance_name, options), false);
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
    return Pass(pass->insert_sub_pass_before(target, type_name, instance_name, options), false);
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
    return Pass(pass->group_sub_pass(target, sub_name), false);
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
    return Pass(pass->group_sub_passes(from, to, group_name), false);
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
    return Pass(pass->get_sub_pass(target), false);
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
        retval.push_back(Pass(p, false));
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
        retval.push_back(Pass(p, false));
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

} // namespace api
} // namespace ql
