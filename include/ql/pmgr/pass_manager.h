/** \file
 * Pass management.
 */

#pragma once

#include <functional>
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
#include "ql/pmgr/pass_types.h"

namespace ql {
namespace pmgr {

/**
 * A generic group of passes, with no special functionality or default set of
 * passes.
 */
class PassGroup : public pass_types::Group {
public:

    /**
     * Constructs the pass group. No error checking here; this is up to the
     * parent pass group. Note that the type name is missing, and that
     * instance_name defaults to the empty string; generic passes always have
     * an empty type name, and the root group has an empty instance name as
     * well.
     */
    PassGroup(
        const utils::Ptr<const PassFactory> &pass_factory,
        const utils::Str &instance_name
    );

protected:

    /**
     * Implementation for the initial pass list. This is no-op for a generic
     * pass group.
     */
    void get_passes(
        const utils::Ptr<const PassFactory> &factory,
        utils::List<PassRef> &passes
    ) final;

    /**
     * Writes the documentation for a basic pass group to the given stream.
     */
    void dump_docs(
        std::ostream &os,
        const utils::Str &line_prefix
    ) const override;

};

class PassFactory;
using PassFactoryRef = utils::Ptr<PassFactory>;
using CPassFactoryRef = utils::Ptr<const PassFactory>;

/**
 * Factory class for constructing passes.
 */
class PassFactory {
private:

    /**
     * Function pointer object type that is used to construct pass class
     * instances.
     */
    using ConstructorFn = utils::Ptr<
        std::function<
            PassRef(
                const CPassFactoryRef &pass_factory,
                const utils::Str &instance_name
            )
        >
    >;

    /**
     * Map from (desugared) pass type name to a constructor function for that
     * particular pass type.
     */
    utils::Map<utils::Str, ConstructorFn> pass_types;

    /**
     * List of analysis pass type name & instance name suffix pairs that are
     * inserted before and after passes with debugging enabled.
     */
    utils::List<utils::Pair<utils::Str, utils::Str>> debug_dumpers;

public:

    /**
     * Constructs a default pass factory for OpenQL.
     */
    PassFactory();

    /**
     * Registers a pass class with the given type name.
     */
    template <class PassType>
    void register_pass(const utils::Str &type_name) {
        ConstructorFn fn;
        fn.template emplace([type_name](
            const CPassFactoryRef &pass_factory,
            const utils::Str &instance_name
        ) {
            PassRef pass;
            pass.template emplace<PassType>(pass_factory, type_name, instance_name);
            return pass;
        });
        pass_types.set(type_name) = fn;
    }

    /**
     * Returns a copy of this pass factory with the following modifications made
     * to the map.
     *
     *  - Entries with a `dnu` path component in them are removed. If the type
     *    of the removed entry exists in dnu however, it will be reinserted with
     *    the `dnu` path component removed.
     *  - A copy is made of entries that include an `arch.<architecture>`
     *    component pair, with that pair stripped.
     *
     * Furthermore, the debug_dumpers member is modified accordingly. The
     * original factory is not modified.
     */
    CPassFactoryRef configure(
        const utils::Str &architecture,
        const utils::Set<utils::Str> &dnu,
        const utils::List<utils::Pair<utils::Str, utils::Str>> &debug_dumpers
    ) const;

    /**
     * Builds a pass instance.
     */
    static PassRef build_pass(
        const CPassFactoryRef &pass_factory,
        const utils::Str &type_name,
        const utils::Str &instance_name
    );

    /**
     * Prefixes and suffixes the given pass list with the debug dumpers
     * configured for this factory.
     */
    static void add_debug_dumpers(
        const CPassFactoryRef &pass_factory,
        utils::List<PassRef> &passes
    );

    /**
     * Dumps documentation for all pass types known by this factory, as well as
     * the option documentation for each pass.
     */
    static void dump_pass_types(
        const CPassFactoryRef &pass_factory,
        std::ostream &os = std::cout,
        const utils::Str &line_prefix = ""
    );

};

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
class PassManager {
private:

    /**
     * The pass factory we're using.
     */
    CPassFactoryRef pass_factory;

    /**
     * The root pass group.
     */
    PassRef root;

public:

    /**
     * Constructs a new pass manager.
     */
    explicit PassManager(
        const utils::Str &architecture = "",
        const utils::Set<utils::Str> &dnu = {},
        const utils::List<utils::Pair<utils::Str, utils::Str>> &debug_dumpers = {
            {"debug_cqasm", "io.WriteCQasm"}
        },
        const PassFactory &factory = {}
    );

    /**
     * Constructs a pass manager based on the given JSON configuration.
     *
     * Structure:
     *
     * ```json
     * {
     *     "strategy": {
     *         "architecture": <optional string, default "">,
     *         "dnu": <optional list of strings, default []>,
     *         "passes": [
     *             <pass description>
     *         ]
     *     },
     *     <any other keys are ignored>
     * }
     * ```
     *
     * The optional "architecture" key may be used to make shorthands for
     * architecture- specific passes, normally prefixed with "arch.<architecture>.".
     * If it's not specified or an empty string, no shorthand aliases are made.
     *
     * The optional "dnu" key may be used to specify a list of do-not-use pass types
     * (experimental passes, deprecated passes, or any other pass that's considered
     * unfit for "production" use) that you explicitly want to use, including the
     * "dnu" namespace they are defined in. Once specified, you'll be able to use
     * the pass type without the "dnu" namespace element. For example, if you would
     * include "dnu.whatever" in the list, the pass type "whatever" may be used to
     * add the pass.
     *
     * Pass descriptions can either be strings (in which case the string is
     * interpreted as a pass type alias and everything else is inferred/default),
     * or an object with the following structure.
     *
     * ```json
     * {
     *     "type": <optional string, default "">,
     *     "name": <optional string, default "">,
     *     "options": <optional object, default {}>
     *     "group": [
     *         <optional list of pass descriptions>
     *     ]
     * }
     * ```
     *
     * The "type" key, if specified, must identify a pass type that OpenQL knows
     * about. If it's not specified or empty, a group is made instead, and "group"
     * must be specified for the group to do anything.
     *
     * The "name" key, if specified, is a user-defined name for the pass, that must
     * match `[a-zA-Z0-9_\-]+` and be unique within the surrounding pass list. If
     * not specified, a name that complies with these requirements is generated
     * automatically, but the actual generated name should not be relied upon to be
     * consistent between OpenQL versions. The name may be used to programmatically
     * refer to passes after construction, and passes may use it for logging or
     * unique output filenames. However, passes should not use the name for anything
     * that affects the behavior of the pass.
     *
     * The "options" key, if specified, may be an object that maps option names to
     * option values. The values may be booleans, integers, or strings, but nothing
     * else. The option names and values must be supported by the particular pass
     * type.
     *
     * The "group" key must only be used when "type" is set to an empty string or
     * left unspecified, turning the pass into a basic group. The list then
     * specifies the sub-passes for the group.
     */
    static PassManager from_json(
        const utils::Json &json,
        const PassFactory &factory = {}
    );

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
     * constructed pass.
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
     * constructed pass.
     */
    PassRef insert_pass_before(
        const utils::Str &target,
        const utils::Str &type_name = "",
        const utils::Str &instance_name = "",
        const utils::Map<utils::Str, utils::Str> &options = {}
    );

    /**
     * Looks for the pass with the target instance name, and embeds it into a
     * newly generated group. The newly created group will assume the name of
     * the original pass, while the original pass will be renamed as specified
     * by sub_name. Note that this ultimately does not modify the pass order.
     * If the target
     */
    PassRef group_pass(
        const utils::Str &target,
        const utils::Str &sub_name = "main"
    );

    /**
     * Like group_pass(), but groups an inclusive range of passes into a group
     * with the given name, leaving the original pass names unchanged.
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
     * not an unconditional group, an exception is thrown.
     */
    void flatten_subgroup(
        const utils::Str &target,
        const utils::Str &name_prefix = ""
    );

    /**
     * Returns a reference to the pass with the given instance name. If no such
     * pass exists, an exception is thrown.
     */
    PassRef get_pass(const utils::Str &target) const;

    /**
     * Returns whether a pass with the target instance name exists.
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
     * on the given platform and program.
     */
    void compile(
        const plat::PlatformRef &platform,
        const ir::ProgramRef &program
    );

};

} // namespace pmgr
} // namespace ql
