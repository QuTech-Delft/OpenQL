/** \file
 * Pass management.
 */

#pragma once

#include <functional>
#include "ql/config.h"
#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/ptr.h"
#include "ql/utils/list.h"
#include "ql/utils/vec.h"
#include "ql/utils/set.h"
#include "ql/utils/pair.h"
#include "ql/utils/options.h"
#include "ql/utils/compat.h"
#include "ql/ir/ir.h"
#include "ql/pmgr/declarations.h"
#include "ql/pmgr/pass_types/base.h"
#include "ql/pmgr/factory.h"

namespace ql {
namespace pmgr {

/**
 * The top-level pass manager class that drives compilation.
 *
 * Internally, this contains a tree structure with compiler passes at the nodes.
 * This structure represents a compilation strategy. Usually, the strategy is
 * just "run the following passes in sequence," but it's also possible to run
 * groups of passes conditionally of in a loop, for instance based on some
 * analysis pass that tries to estimate how much potential for optimization
 * remains in a given program.
 *
 * Passes are configured based on a pass type and pass options. The available
 * pass type names depend on the PassFactory that the PassManager is
 * constructed with, the selected target architecture, and the list of
 * "do-not-use" passes that are explicitly enabled. As for the options; some
 * options exist for all passes, while others only exist for a particular pass
 * type. Pass options can be (re)configured until construct() is called, at
 * which point the pass may expand into a number of sub-passes based on its
 * configuration, which then become configurable. The complete list of passes
 * and their options
 *
 * Ultimately, the compile() method applies the configured compilation strategy
 * to a program and platform, reducing the abstraction level of the program and
 * constraining it to the platform as per the strategy.
 *
 * Constructed passes are usually referred to by instance names. You're free to
 * choose these names, as long as they don't contain any special symbols
 * (the names must match `[a-zA-Z0-9_\-]+`); the pass should not do anything
 * with this name other than use it to name log files and such. Periods are used
 * for hierarchy separation, so `a.b` refers to sub-pass `b` of pass `a`.
 */
class Manager {
public:

    /**
     * Dumps the documentation for the pass JSON configuration structure.
     */
    static void dump_docs(std::ostream &os = std::cout, const utils::Str &line_prefix = "");

private:

    /**
     * The pass factory we're using.
     */
    CFactoryRef pass_factory;

    /**
     * The root pass group.
     */
    PassRef root;

public:

    /**
     * Constructs a new pass manager.
     */
    explicit Manager(
        const utils::Str &architecture = "",
        const utils::Set<utils::Str> &dnu = {},
        const Factory &factory = {}
    );

    /**
     * Constructs a pass manager based on the given JSON configuration. Refer
     * to dump_docs() for details.
     */
    static Manager from_json(
        const utils::Json &json,
        const Factory &factory = {}
    );

    /**
     * Generate a pass manager with a strategy that aims to mimic the flow of
     * the OpenQL compiler as it was before pass management as closely as
     * possible. The actual pass list is derived from the eqasm_compiler key
     * in the configuration file and from the global options (similar to the
     * "compatibility-mode" key in the JSON strategy definition format).
     */
    static Manager from_defaults(const ir::compat::PlatformRef &platform);

    /**
     * Returns a reference to the root pass group.
     */
    const PassRef &get_root();

    /**
     * Returns a reference to the root pass group.
     */
    CPassRef get_root() const;

    /**
     * Dumps documentation for all available pass types, as well as the option
     * documentation for the passes.
     */
    void dump_pass_types(
        std::ostream &os = std::cout,
        const utils::Str &line_prefix = ""
    ) const;

    /**
     * Dumps the currently configured compilation strategy to the given stream.
     */
    void dump_strategy(
        std::ostream &os = std::cout,
        const utils::Str &line_prefix = ""
    ) const;

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
    utils::UInt set_option(
        const utils::Str &path,
        const utils::Str &value,
        utils::Bool must_exist = true
    );

    /**
     * Sets an option for all passes recursively. The return value is the number
     * of passes that were affected; passes are only affected when they have an
     * option with the specified name. If must_exist is set an exception will be
     * thrown if none of the passes were affected, otherwise 0 will be returned.
     */
    utils::UInt set_option_recursively(
        const utils::Str &option,
        const utils::Str &value,
        utils::Bool must_exist = true
    );

    /**
     * Returns the current value of an option. Periods are used as hierarchy
     * separators; the last element will be the option name, and the preceding
     * elements represent pass instance names.
     */
    const utils::Option &get_option(const utils::Str &path) const;

    /**
     * Appends a pass to the end of the pass list. If type_name is empty
     * or unspecified, a generic subgroup is added. Returns a reference to the
     * constructed pass.
     */
    PassRef append_pass(
        const utils::Str &type_name = "",
        const utils::Str &instance_name = "",
        const utils::Map<utils::Str, utils::Str> &options = {}
    );

    /**
     * Appends a pass to the beginning of the pass list. If type_name is empty
     * or unspecified, a generic subgroup is added. Returns a reference to the
     * constructed pass.
     */
    PassRef prefix_pass(
        const utils::Str &type_name = "",
        const utils::Str &instance_name = "",
        const utils::Map<utils::Str, utils::Str> &options = {}
    );

    /**
     * Inserts a pass immediately after the target pass (named by instance). If
     * target does not exist, an exception is thrown. If type_name is empty or
     * unspecified, a generic subgroup is added. Returns a reference to the
     * constructed pass. Periods may be used in target to traverse deeper into
     * the pass hierarchy.
     */
    PassRef insert_pass_after(
        const utils::Str &target,
        const utils::Str &type_name = "",
        const utils::Str &instance_name = "",
        const utils::Map<utils::Str, utils::Str> &options = {}
    );

    /**
     * Inserts a pass immediately before the target pass (named by instance). If
     * target does not exist, an exception is thrown. If type_name is empty or
     * unspecified, a generic subgroup is added. Returns a reference to the
     * constructed pass. Periods may be used in target to traverse deeper into
     * the pass hierarchy.
     */
    PassRef insert_pass_before(
        const utils::Str &target,
        const utils::Str &type_name = "",
        const utils::Str &instance_name = "",
        const utils::Map<utils::Str, utils::Str> &options = {}
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
    PassRef group_pass(
        const utils::Str &target,
        const utils::Str &sub_name = "main"
    );

    /**
     * Like group_pass(), but groups an inclusive range of passes into a
     * group with the given name, leaving the original pass names unchanged.
     * Periods may be used in from/to to traverse deeper into the pass
     * hierarchy, but the hierarchy prefix must be the same for from and to.
     */
    PassRef group_passes(
        const utils::Str &from,
        const utils::Str &to,
        const utils::Str &group_name
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
        const utils::Str &target,
        const utils::Str &name_prefix = ""
    );

    /**
     * Returns a reference to the pass with the given instance name. If no such
     * pass exists, an exception is thrown. Periods may be used as hierarchy
     * separators to get nested sub-passes.
     */
    PassRef get_pass(const utils::Str &target) const;

    /**
     * Returns whether a pass with the target instance name exists. Periods may
     * be used in target to traverse deeper into the pass hierarchy.
     */
    utils::Bool does_pass_exist(const utils::Str &target) const;

    /**
     * Returns the total number of passes in the root hierarchy.
     */
    utils::UInt get_num_passes() const;

    /**
     * If this pass constructed into a group of passes, returns a reference to
     * the list containing all the sub-passes. Otherwise, an exception is
     * thrown.
     */
    const utils::List<PassRef> &get_passes() const;

    /**
     * Returns an indexable list of references to all passes with the given
     * type within the root hierarchy.
     */
    utils::Vec<PassRef> get_sub_passes_by_type(const utils::Str &target) const;

    /**
     * Removes the pass with the given target instance name, or throws an
     * exception if no such pass exists.
     */
    void remove_pass(const utils::Str &target);

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
     * on the given program.
     */
    void compile(const ir::Ref &ir);

};

/**
 * A shared pointer reference to a pass manager.
 */
using Ref = utils::Ptr<Manager>;

} // namespace pmgr
} // namespace ql
