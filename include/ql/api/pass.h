/** \file
 * API header for modifying compiler pass parameters.
 */

#pragma once

#include "ql/config.h"
#include "ql/pmgr/pass_types/base.h"
#include "ql/api/declarations.h"

//============================================================================//
//                               W A R N I N G                                //
//----------------------------------------------------------------------------//
//         Docstrings in this file must manually be kept in sync with         //
//       pass.i! This should be automated at some point, but isn't yet.       //
//============================================================================//

namespace ql {
namespace api {

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
    std::string dump_pass_documentation() const;

    /**
     * Prints the current state of the options. If only_set is set to true, only
     * the options that were explicitly configured are dumped.
     */
    void print_options(bool only_set = false) const;

    /**
     * Returns the string printed by print_options().
     */
    std::string dump_options(bool only_set = false) const;

#ifdef QL_HIERARCHICAL_PASS_MANAGEMENT
    /**
     * Prints the entire compilation strategy including configured options of
     * this pass and all sub-passes.
     */
    void print_strategy() const;
#endif

#ifdef QL_HIERARCHICAL_PASS_MANAGEMENT
    /**
     * Returns the string printed by print_strategy().
     */
    std::string dump_strategy() const;
#endif

#ifdef QL_HIERARCHICAL_PASS_MANAGEMENT
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
#else
    /**
     * Sets an option.
     */
    void set_option(
        const std::string &option,
        const std::string &value
    );
#endif

#ifdef QL_HIERARCHICAL_PASS_MANAGEMENT
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
#endif

#ifdef QL_HIERARCHICAL_PASS_MANAGEMENT
    /**
     * Returns the current value of an option. Periods may be used as hierarchy
     * separators to get options from sub-passes (if any).
     */
#else
    /**
     * Returns the current value of an option.
     */
#endif
    std::string get_option(const std::string &option) const;

#ifdef QL_HIERARCHICAL_PASS_MANAGEMENT
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
#endif

};

} // namespace api
} // namespace ql
