/** \file
 * Defines the base classes for all passes.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/ptr.h"
#include "ql/utils/list.h"
#include "ql/utils/vec.h"
#include "ql/utils/set.h"
#include "ql/utils/options.h"
#include "ql/ir/ir.h"
#include "ql/pmgr/declarations.h"
#include "ql/pmgr/condition.h"

namespace ql {
namespace pmgr {
namespace pass_types {

/**
 * Context information provided to the run function of a pass by the pass
 * management system.
 */
struct Context {

    /**
     * The fully-qualified pass name, using periods for hierarchy separation.
     */
    utils::Str full_pass_name;

    /**
     * The directory and filename prefix that should be used for all output
     * products of the pass.
     */
    utils::Str output_prefix;

    /**
     * Reference to the pass options.
     */
    const utils::Options &options;

};

// Forward declaration for the base type.
class Base;

/**
 * A reference to any pass type.
 */
using Ref = utils::Ptr<Base>;

/**
 * An immutable reference to any pass type.
 */
using CRef = utils::Ptr<const Base>;

/**
 * Type of a node within the pass instance tree that represents the compilation
 * strategy.
 */
enum class NodeType {

    /**
     * construct() has not been called yet, so the node type is still
     * undetermined.
     */
    UNKNOWN,

    /**
     * A normal pass that semantically doesn't contain a group of sub-passes.
     * The group is ignored by compile() and calls to functions that modify it
     * throw an exception. compile() only calls run_internal()/run().
     */
    NORMAL,

    /**
     * An unconditional group of passes. Such a group serves only as a
     * hierarchical level for logging/profiling and as an abstraction layer to
     * users; compile() will simply run the passes in sequence with no further
     * logic; run_internal()/run() is not called. Such groups can be collapsed
     * or created by the user at will.
     */
    GROUP,

    /**
     * A conditional group of passes. compile() will first call
     * run_internal()/run(), and then use its status return value and the
     * condition to determine whether to run the pass group as well.
     */
    GROUP_IF,

    /**
     * Like GROUP_IF, but loops back and re-evaluates the condition by calling
     * run_internal()/run() again after the group of passes finishes executing.
     */
    GROUP_WHILE,

    /**
     * Like GROUP_WHILE, but the condition is evaluated at the end of the loop
     * rather than at the beginning, so the group is evaluated at list once.
     */
    GROUP_REPEAT_UNTIL_NOT

};

/**
 * Base class for all passes.
 */
class Base {
private:

    /**
     * Reference to the pass factory that was used to construct this pass,
     * allowing this pass to construct sub-passes.
     */
    utils::Ptr<const Factory> pass_factory;

    /**
     * The full type name for this pass. This is the full name that was used
     * when the pass was registered with the pass factory. The same pass class
     * may be registered with multiple type names, in which case the pass
     * implementation may use this to differentiate. An empty type name is used
     * for generic groups.
     */
    const utils::Str type_name;

    /**
     * The instance name for this pass, i.e. the name that the user assigned to
     * it or the name that was assigned to it automatically. Must match
     * `[a-zA-Z0-9_\-]+` for normal passes or groups, and must be unique within
     * the group of passes it resides in. The root group uses an empty name.
     * Instance names should NOT have a semantic meaning besides possibly
     * uniquely naming output files; use options for any other functional
     * configuration.
     */
    utils::Str instance_name;

    /**
     * The type of node that this pass represents in the pass tree. Configured
     * by construct().
     */
    NodeType node_type = NodeType::UNKNOWN;

    /**
     * List of sub-passes, used only by group nodes.
     */
    utils::List<Ref> sub_pass_order;

    /**
     * Mapping from instance name to sub-pass, used only by group nodes.
     */
    utils::Map<utils::Str, Ref> sub_pass_names;

    /**
     * The condition used to turn the pass return value into a boolean, used
     * only for conditional group nodes.
     */
    condition::Ref condition;

    /**
     * Throws an exception if the given pass instance name is invalid.
     */
    static void check_pass_name(
        const utils::Str &instance_name,
        const utils::Map<utils::Str, Ref> &existing_pass_names
    );

    /**
     * Returns a unique name generated from the given type name.
     */
    utils::Str generate_valid_pass_name(const utils::Str &type_name) const;

    /**
     * Makes a new pass. Used by the various functions that add passes.
     */
    Ref make_pass(
        const utils::Str &type_name,
        const utils::Str &instance_name,
        const utils::Map<utils::Str, utils::Str> &options
    ) const;

    /**
     * Returns an iterator for the sub_pass_order list corresponding with the
     * given target instance name, or throws an exception if no such pass is
     * found.
     */
    utils::List<Ref>::iterator find_pass(const utils::Str &target);

    /**
     * Checks whether access to the sub-pass list is allowed. Throws an
     * exception if not.
     */
    void check_group_access_allowed() const;

    /**
     * Checks whether access to the condition is allowed. Throws an exception
     * if not.
     */
    void check_condition_access_allowed() const;

protected:

    /**
     * The option set for this pass. The available options should be registered
     * in the constructor of the derived pass types. It becomes illegal to
     * change options once construct() is called.
     */
    utils::Options options;

    /**
     * Constructs the abstract pass. No error checking here; this is up to the
     * parent pass group.
     */
    Base(
        const utils::Ptr<const Factory> &pass_factory,
        const utils::Str &type_name,
        const utils::Str &instance_name
    );

    /**
     * Writes the documentation for this pass to the given output stream. May
     * depend on type_name, but should not depend on anything else. The
     * automatically-generated documentation for the options should not be
     * added here; it is added by dump_help(). The help should end in a newline,
     * and every line printed should start with line_prefix.
     */
    virtual void dump_docs(
        std::ostream &os,
        const utils::Str &line_prefix
    ) const = 0;

    /**
     * Overridable implementation of construct(). Must return a non-unknown node
     * type for this pass to construct into, based on its options. If a group
     * type is returned, passes must be populated (it may be assumed to be empty
     * initially). If a conditional group type is returned, condition must also
     * be populated.
     */
    virtual NodeType on_construct(
        const utils::Ptr<const Factory> &factory,
        utils::List<Ref> &passes,
        condition::Ref &condition
    ) = 0;

    /**
     * Overridable implementation for calling the implementation of the pass.
     */
    virtual utils::Int run_internal(
        const ir::Ref &ir,
        const Context &context
    ) const = 0;

    /**
     * Returns whether this is a legacy pass, i.e. one that operates on the old
     * IR. Returns false unless overridden.
     */
    virtual utils::Bool is_legacy() const;

    /**
     * Returns `pass "<name>"` for normal passes and `root` for the root pass.
     * Used for error messages.
     */
    utils::Str describe() const;

public:

    /**
     * Default virtual destructor.
     */
    virtual ~Base() = default;

    /**
     * Returns a user-friendly type name for this pass. Used for documentation
     * generation.
     */
    virtual utils::Str get_friendly_type() const = 0;

    /**
     * Returns the full, desugared type name that this pass was constructed
     * with.
     */
    const utils::Str &get_type() const;

    /**
     * Returns the instance name of the pass within the surrounding group.
     */
    const utils::Str &get_name() const;

    /**
     * Dumps the documentation for this pass to the given stream.
     */
    void dump_help(
        std::ostream &os = std::cout,
        const utils::Str &line_prefix = ""
    ) const;

    /**
     * Dumps the current state of the options to the given stream. If only_set
     * is set to true, only the options that were explicitly configured are
     * dumped.
     */
    void dump_options(
        utils::Bool only_set = false,
        std::ostream &os = std::cout,
        const utils::Str &line_prefix = ""
    ) const;

    /**
     * Dumps the entire compilation strategy including configured options of
     * this pass and all sub-passes.
     */
    void dump_strategy(
        std::ostream &os = std::cout,
        const utils::Str &line_prefix = ""
    ) const;

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
    utils::UInt set_option(
        const utils::Str &option,
        const utils::Str &value,
        utils::Bool must_exist = true
    );

    /**
     * Sets an option for all sub-passes recursively. The return value is the
     * number of passes that were affected; passes are only affected when they
     * have an option with the specified name. If must_exist is set an exception
     * will be thrown if none of the passes were affected, otherwise 0 will be
     * returned.
     */
    utils::UInt set_option_recursively(
        const utils::Str &option,
        const utils::Str &value,
        utils::Bool must_exist = true
    );

    /**
     * Returns the current value of an option. Periods may be used as hierarchy
     * separators to get options from sub-passes (if any).
     */
    const utils::Option &get_option(const utils::Str &option) const;

    /**
     * Returns mutable access to the embedded options object. This is allowed
     * only until construct() is called.
     */
    utils::Options &get_options();

    /**
     * Returns read access to the embedded options object.
     */
    const utils::Options &get_options() const;

    /**
     * Constructs this pass. During construction, the pass implementation may
     * decide, based on its options, to become a group of passes or a normal
     * pass. If it decides to become a group, the group may be introspected or
     * modified by the user. The options are frozen after this, so set_option()
     * will start throwing exceptions when called. construct() may be called any
     * number of times, but becomes no-op after the first call.
     */
    void construct();

private:
    friend class ::ql::pmgr::Manager;

    /**
     * Recursively constructs this pass and all its sub-passes (if it constructs
     * or previously constructed into a group).
     */
    void construct_recursive(const utils::Str &pass_name_prefix = "");

public:

    /**
     * Returns whether this pass has been constructed yet.
     */
    utils::Bool is_constructed() const;

    /**
     * Returns whether this pass has configurable sub-passes.
     */
    utils::Bool is_group() const;

    /**
     * Returns whether this pass is a simple group of which the sub-passes can
     * be collapsed into the parent pass group without affecting the strategy.
     */
    utils::Bool is_collapsible() const;

    /**
     * Returns whether this is the root pass group in a pass manager.
     */
    utils::Bool is_root() const;

    /**
     * Returns whether this pass contains a conditionally-executed group.
     */
    utils::Bool is_conditional() const;

    /**
     * If this pass constructed into a group of passes, appends a pass to the
     * end of its pass list. Otherwise, an exception is thrown. If type_name is
     * empty or unspecified, a generic subgroup is added. Returns a reference to
     * the constructed pass.
     */
    Ref append_sub_pass(
        const utils::Str &type_name = "",
        const utils::Str &instance_name = "",
        const utils::Map<utils::Str, utils::Str> &options = {}
    );

    /**
     * If this pass constructed into a group of passes, appends a pass to the
     * beginning of its pass list. Otherwise, an exception is thrown. If
     * type_name is empty or unspecified, a generic subgroup is added. Returns a
     * reference to the constructed pass.
     */
    Ref prefix_sub_pass(
        const utils::Str &type_name = "",
        const utils::Str &instance_name = "",
        const utils::Map<utils::Str, utils::Str> &options = {}
    );

    /**
     * If this pass constructed into a group of passes, inserts a pass
     * immediately after the target pass (named by instance). If target does not
     * exist or this pass is not a group of sub-passes, an exception is thrown.
     * If type_name is empty or unspecified, a generic subgroup is added.
     * Returns a reference to the constructed pass. Periods may be used in
     * target to traverse deeper into the pass hierarchy.
     */
    Ref insert_sub_pass_after(
        const utils::Str &target,
        const utils::Str &type_name = "",
        const utils::Str &instance_name = "",
        const utils::Map<utils::Str, utils::Str> &options = {}
    );

    /**
     * If this pass constructed into a group of passes, inserts a pass
     * immediately before the target pass (named by instance). If target does
     * not exist or this pass is not a group of sub-passes, an exception is
     * thrown. If type_name is empty or unspecified, a generic subgroup is
     * added. Returns a reference to the constructed pass. Periods may be used
     * in target to traverse deeper into the pass hierarchy.
     */
    Ref insert_sub_pass_before(
        const utils::Str &target,
        const utils::Str &type_name = "",
        const utils::Str &instance_name = "",
        const utils::Map<utils::Str, utils::Str> &options = {}
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
    Ref group_sub_pass(
        const utils::Str &target,
        const utils::Str &sub_name = "main"
    );

    /**
     * Like group_sub_pass(), but groups an inclusive range of passes into a
     * group with the given name, leaving the original pass names unchanged.
     * Periods may be used in from/to to traverse deeper into the pass
     * hierarchy, but the hierarchy prefix must be the same for from and to.
     */
    Ref group_sub_passes(
        const utils::Str &from,
        const utils::Str &to,
        const utils::Str &group_name
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
        const utils::Str &target,
        const utils::Str &name_prefix = ""
    );

    /**
     * If this pass constructed into a group of passes, returns a reference to
     * the pass with the given instance name. If target does not exist or this
     * pass is not a group of sub-passes, an exception is thrown. Periods may be
     * used as hierarchy separators to get nested sub-passes.
     */
    Ref get_sub_pass(const utils::Str &target) const;

    /**
     * If this pass constructed into a group of passes, returns whether a
     * sub-pass with the target instance name exists. Otherwise, an exception is
     * thrown. Periods may be used in target to traverse deeper into the pass
     * hierarchy.
     */
    utils::Bool does_sub_pass_exist(const utils::Str &target) const;

    /**
     * If this pass constructed into a group of passes, returns the total number
     * of immediate sub-passes. Otherwise, an exception is thrown.
     */
    utils::UInt get_num_sub_passes() const;

    /**
     * If this pass constructed into a group of passes, returns a reference to
     * the list containing all the sub-passes. Otherwise, an exception is
     * thrown.
     */
    const utils::List<Ref> &get_sub_passes() const;

    /**
     * If this pass constructed into a group of passes, returns an indexable
     * list of references to all immediate sub-passes with the given type.
     * Otherwise, an exception is thrown.
     */
    utils::Vec<Ref> get_sub_passes_by_type(const utils::Str &target) const;

    /**
     * If this pass constructed into a group of passes, removes the sub-pass
     * with the target instance name. If target does not exist or this pass is
     * not a group of sub-passes, an exception is thrown. Periods may be used in
     * target to traverse deeper into the pass hierarchy.
     */
    void remove_sub_pass(const utils::Str &target);

    /**
     * If this pass constructed into a group of passes, removes all sub-passes.
     * Otherwise, an exception is thrown.
     */
    void clear_sub_passes();

    /**
     * If this pass constructed into a conditional pass group, returns a const
     * reference to the configured condition. Otherwise, an exception is thrown.
     */
    condition::CRef get_condition() const;

    /**
     * If this pass constructed into a conditional pass group, returns a mutable
     * reference to the configured condition. Otherwise, an exception is thrown.
     */
    condition::Ref get_condition();

private:

    /**
     * Handles the debug option. Called once before and once after compile().
     * after_pass is false when run before, and true when run after.
     */
    void handle_debugging(
        const ir::Ref &ir,
        const Context &context,
        utils::Bool after_pass
    );

    /**
     * Wrapper around running the main pass implementation for this pass, taking
     * care of logging, profiling, etc.
     */
    utils::Int run_main_pass(
        const ir::Ref &ir,
        const Context &context
    ) const;

    /**
     * Wrapper around running the sub-passes for this pass, taking care of logging,
     * profiling, etc.
     */
    void run_sub_passes(
        const ir::Ref &ir,
        const Context &context
    ) const;

public:

    /**
     * Executes this pass or pass group on the given program.
     */
    void compile(
        const ir::Ref &ir,
        const utils::Str &pass_name_prefix = ""
    );

};

} // namespace pass_types

/**
 * Shorthand for a reference to any pass type.
 */
using PassRef = pass_types::Ref;

/**
 * Shorthand for an immutable reference to any pass type.
 */
using CPassRef = pass_types::CRef;

} // namespace pmgr
} // namespace ql
