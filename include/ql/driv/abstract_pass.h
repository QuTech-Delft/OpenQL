/** \file
 * AbstractPass implementation. Base class for all pass types usable within the
 * driver.
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

namespace ql {
namespace driv {

// Forward declaration for the pass factory.
class PassFactory;

/**
 * Base class for all passes, including groups of passes.
 */
class AbstractPass {
public:

    /**
     * A reference to a pass.
     */
    using Ref = utils::Ptr<AbstractPass>;

private:

    /**
     * Reference to the pass factory that was used to construct this pass,
     * allowing this pass to construct sub-passes.
     */
    const utils::Ptr<const PassFactory> &pass_factory;

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
     * The option set for this pass. The available options should be registered
     * in the constructor of the derived pass types. It becomes illegal to
     * change options once construct() is called.
     */
    utils::Options options;

    /**
     * Whether this pass has been constructed yet, see construct().
     */
    utils::Bool constructed = false;

    /**
     * Whether this pass constructed to a group of sub-passes or an actual pass.
     * If this is false after construct(), this pass behaves like a normal pass
     * (i.e. on_run() is called). Otherwise, on_run() is not called, but instead
     * the sub-passes are run in the specified order.
     */
    utils::Bool group = false;

    /**
     * List of sub-passes, used only when is_group is set to true.
     */
    utils::List<Ref> sub_pass_order;

    /**
     * List of sub-passes, used only when is_group is set to true.
     */
    utils::Map<utils::Str, Ref> sub_pass_names;

protected:

    /**
     * Constructs the abstract pass. No error checking here; this is up to the
     * parent pass group.
     */
    AbstractPass(
        const utils::Ptr<const PassFactory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Writes the documentation for this pass to the given output stream. May
     * depend on type_name, but should not depend on anything else. The
     * automatically-generated documentation for the options should not be
     * added here; it is added by dump_help().
     */
    virtual void dump_docs(std::ostream &os) const = 0;

    /**
     * Overridable implementation of construct(). If this abstract pass is to
     * become a pass group, this must return true and passes must be populated
     * with a list of sub-passes constructed using the given pass factory.
     * Name uniqueness and regex matching is done by the caller. on_run() will
     * not be called in this case. If this abstract pass is to behave like a
     * normal pass, false must be returned.
     */
    virtual utils::Bool on_construct(
        const utils::Ptr<const PassFactory> &factory,
        utils::List<Ref> &passes
    ) = 0;

    /**
     * Overridable implementation of compile(). Called within compile() when
     * this is not a pass group.
     */
    virtual void on_compile(
        const plat::PlatformRef &platform,
        const ir::ProgramRef &program
    ) const = 0;

public:

    /**
     * Default virtual destructor.
     */
    virtual ~AbstractPass() = default;

    /**
     * Returns the full, desugared type name that this pass was constructed
     * with.
     */
    const utils::Str &get_type() const;

    /**
     * Returns the full, desugared type name that this pass was constructed
     * with.
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
     * Dumps the entire compilation plan including configured options of this
     * pass and all sub-passes.
     */
    void dump_plan(
        std::ostream &os = std::cout,
        const utils::Str &line_prefix = ""
    ) const;

    /**
     * Sets an option. This is allowed only until construct() is called.
     */
    void set_option(const utils::Str &option, const utils::Str &value);

    /**
     * Returns the current value of an option.
     */
    const utils::Option &get_option(const utils::Str &option) const;

    /**
     * Returns the embedded options object.
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

    /**
     * Returns whether this pass has been constructed yet.
     */
    utils::Bool is_constructed();

    /**
     * Returns whether this pass is a group (true) or a normal pass (false).
     */
    utils::Bool is_group();

    /**
     * Returns whether this is the root pass group in a driver.
     */
    utils::Bool is_root();

    /**
     * If this pass constructed into a group of passes, appends a pass to the
     * end of its pass list. Otherwise, an exception is thrown. If type_name is
     * empty or unspecified, a generic subgroup is added. Returns a reference to
     * the constructed pass.
     */
    Ref append_sub_pass(
        const utils::Str &type_name = "",
        const utils::Str instance_name = "",
        const utils::Map<utils::Str, utils::Str> options = {}
    );

    /**
     * If this pass constructed into a group of passes, appends a pass to the
     * beginning of its pass list. Otherwise, an exception is thrown. If
     * type_name is empty or unspecified, a generic subgroup is added. Returns a
     * reference to the constructed pass.
     */
    Ref prefix_sub_pass(
        const utils::Str &type_name = "",
        const utils::Str instance_name = "",
        const utils::Map<utils::Str, utils::Str> options = {}
    );

    /**
     * If this pass constructed into a group of passes, inserts a pass
     * immediately after the target pass (named by instance). If target does not
     * exist or this pass is not a group of sub-passes, an exception is thrown.
     * If type_name is empty or unspecified, a generic subgroup is added.
     * Returns a reference to the constructed pass.
     */
    Ref insert_sub_pass_after(
        const utils::Str &target,
        const utils::Str &type_name = "",
        const utils::Str instance_name = "",
        const utils::Map<utils::Str, utils::Str> options = {}
    );

    /**
     * If this pass constructed into a group of passes, inserts a pass
     * immediately before the target pass (named by instance). If target does
     * not exist or this pass is not a group of sub-passes, an exception is
     * thrown. If type_name is empty or unspecified, a generic subgroup is
     * added. Returns a reference to the constructed pass.
     */
    Ref insert_sub_pass_before(
        const utils::Str &target,
        const utils::Str &type_name = "",
        const utils::Str instance_name = "",
        const utils::Map<utils::Str, utils::Str> options = {}
    );

    /**
     * If this pass constructed into a group of passes, looks for the pass with
     * the target instance name, and embeds it into a newly generated group. The
     * group will assume the name of the original pass, while the original pass
     * will be renamed as specified by sub_name. Note that this ultimately does
     * not modify the pass order. If target does not exist or this pass is not a
     * group of sub-passes, an exception is thrown. Returns a reference to the
     * constructed group.
     */
    Ref group_sub_pass(
        const utils::Str &target,
        const utils::Str &sub_name = "main"
    );

    /**
     * Like group_sub_pass(), but groups an inclusive range of passes.
     */
    Ref group_sub_passes(
        const utils::Str &from,
        const utils::Str &to,
        const utils::Str &sub_name = "main"
    );

    /**
     * If this pass constructed into a group of passes, looks for the pass with
     * the target instance name, treats it as a generic group, and flattens its
     * contained passes into the list of sub-passes of this group. The names of
     * the passes found in the collapsed subgroup are prefixed with name_prefix
     * before they are added to the parent group. Note that this ultimately does
     * not modify the pass order. If target does not exist, does not construct
     * into a group of passes (construct() is called automatically), or this
     * pass is not a group of sub-passes, an exception is thrown.
     */
    void flatten_subgroup(
        const utils::Str &target,
        const utils::Str &name_prefix = ""
    );

    /**
     * If this pass constructed into a group of passes, returns a reference to
     * the pass with the given instance name. If target does not exist or this
     * pass is not a group of sub-passes, an exception is thrown.
     */
    Ref get_sub_pass(const utils::Str &target) const;

    /**
     * If this pass constructed into a group of passes, returns whether a
     * sub-pass with the target instance name exists. Otherwise, an exception is
     * thrown.
     */
    utils::Bool does_sub_pass_exist(const utils::Str &target) const;

    /**
     * If this pass constructed into a group of passes, returns the total number
     * of sub-passes. Otherwise, an exception is thrown.
     */
    utils::UInt get_num_sub_passes() const;

    /**
     * If this pass constructed into a group of passes, returns an indexable
     * list of references to all passes with the given type. Otherwise, an
     * exception is thrown.
     */
    utils::Vec<Ref> get_sub_passes_by_type(const utils::Str &target) const;

    /**
     * If this pass constructed into a group of passes, returns a reference to
     * the list containing all the sub-passes. Otherwise, an exception is
     * thrown.
     */
    const utils::List<Ref> &get_sub_passes(const utils::Str &target) const;

    /**
     * If this pass constructed into a group of passes, removes the sub-pass
     * with the target instance name. If target does not exist or this pass is
     * not a group of sub-passes, an exception is thrown.
     */
    void remove_sub_pass(const utils::Str &target);

    /**
     * If this pass constructed into a group of passes, removes all sub-passes.
     * Otherwise, an exception is thrown.
     */
    void clear_sub_passes();

    /**
     * Executes this pass or pass group on the given platform and program.
     */
    void compile(
        const plat::PlatformRef &platform,
        const ir::ProgramRef &program
    ) const;

};

/**
 * A pass type for passes that always construct into a group. For example, a
 * generic optimizer pass with an option-configured set of optimization passes
 * would derive from this.
 */
class AbstractPassGroup : public AbstractPass {
protected:

    /**
     * Constructs the abstract pass group. No error checking here; this is up to
     * the parent pass group.
     */
    AbstractPassGroup(
        const utils::Ptr<const PassFactory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Simple implementation for on_construct() that always returns true and
     * defers to get_passes() for the initial pass list.
     */
    utils::Bool on_construct(
        const utils::Ptr<const PassFactory> &factory,
        utils::List<Ref> &passes
    ) final;

    /**
     * Dummy implementation for compilation. Should never be called, as this
     * pass always behaves as a group. Thus, it just throws an exception.
     */
    void on_compile(
        const plat::PlatformRef &platform,
        const ir::ProgramRef &program
    ) const final;

    /**
     * Overridable implementation that returns the initial pass list for this
     * pass group. The default implementation is no-op.
     */
    virtual void get_passes(
        const utils::Ptr<const PassFactory> &factory,
        utils::List<Ref> &passes
    ) = 0;

};

/**
 * A generic group of passes, with no special functionality or default set of
 * passes.
 */
class PassGroup : public AbstractPassGroup {
protected:

    /**
     * Constructs the pass group. No error checking here; this is up to the
     * parent pass group. Note that the type name is missing, and that
     * instance_name defaults to the empty string; generic passes always have
     * an empty type name, and the root group has an empty instance name as
     * well.
     */
    PassGroup(
        const utils::Ptr<const PassFactory> &pass_factory,
        const utils::Str &instance_name = ""
    );

    /**
     * Implementation for the initial pass list. This is no-op for a generic
     * pass group.
     */
    virtual void get_passes(
        const utils::Ptr<const PassFactory> &factory,
        utils::List<Ref> &passes
    ) final;

};

/**
 * A pass type for regular passes that normally don't construct into a group
 * (although this is still possible). Just provides a default implementation for
 * on_construct().
 */
class AbstractNormalPass : public AbstractPass {
protected:
    
    /**
     * Constructs the normal pass. No error checking here; this is up to the
     * parent pass group.
     */
    AbstractNormalPass(
        const utils::Ptr<const PassFactory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Default implementation for on_construct() that makes this a normal pass.
     * May be overridden to allow the pass to generate into a group as well,
     * based on its options.
     */
    utils::Bool on_construct(
        const utils::Ptr<const PassFactory> &factory,
        utils::List<Ref> &passes
    ) override;

};

/**
 * A pass type for passes that apply a program-wide transformation.
 */
class AbstractProgramTransformationPass : public AbstractNormalPass {
protected:
    
    /**
     * Constructs the pass. No error checking here; this is up to the parent
     * pass group.
     */
    AbstractProgramTransformationPass(
        const utils::Ptr<const PassFactory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Implementation for on_compile() that calls run() appropriately.
     */
    void on_compile(
        const plat::PlatformRef &platform,
        const ir::ProgramRef &program
    ) const final;

    /**
     * The virtual implementation for this pass.
     */
    virtual void run(
        const plat::PlatformRef &platform,
        const ir::ProgramRef &program
    ) const = 0;

};

/**
 * A pass type for passes that apply a transformation per kernel/basic block.
 */
class AbstractKernelTransformationPass : public AbstractNormalPass {
protected:
    
    /**
     * Constructs the pass. No error checking here; this is up to the parent
     * pass group.
     */
    AbstractKernelTransformationPass(
        const utils::Ptr<const PassFactory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Implementation for on_compile() that calls run() appropriately.
     */
    void on_compile(
        const plat::PlatformRef &platform,
        const ir::ProgramRef &program
    ) const final;

    /**
     * The virtual implementation for this pass.
     */
    virtual void run(
        const plat::PlatformRef &platform,
        const ir::ProgramRef &program,
        const ir::KernelRef &kernel
    ) const = 0;

};

/**
 * A pass type for passes that analyze a program without modifying it.
 */
class AbstractProgramAnalysisPass : public AbstractNormalPass {
protected:
    
    /**
     * Constructs the pass. No error checking here; this is up to the parent
     * pass group.
     */
    AbstractProgramAnalysisPass(
        const utils::Ptr<const PassFactory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Implementation for on_compile() that calls run() appropriately.
     */
    void on_compile(
        const plat::PlatformRef &platform,
        const ir::ProgramRef &program
    ) const final;

    /**
     * The virtual implementation for this pass.
     */
    virtual void run(
        const plat::PlatformRef &platform,
        const ir::ProgramRef &program
    ) const = 0;

};

/**
 * A pass type for passes that apply a transformation per kernel/basic block.
 */
class AbstractKernelAnalysisPass : public AbstractNormalPass {
protected:
    
    /**
     * Constructs the pass. No error checking here; this is up to the parent
     * pass group.
     */
    AbstractKernelAnalysisPass(
        const utils::Ptr<const PassFactory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Implementation for on_compile() that calls run() appropriately.
     */
    void on_compile(
        const plat::PlatformRef &platform,
        const ir::ProgramRef &program
    ) const final;

    /**
     * The virtual implementation for this pass.
     */
    virtual void run(
        const plat::PlatformRef &platform,
        const ir::ProgramRef &program,
        const ir::KernelRef &kernel
    ) const = 0;

};

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
            AbstractPass::Ref(
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
     * Registers a pass class with the given type name.
     */
    template <class PassType>
    void register_pass(const utils::Str &type_name);

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
    PassFactory configure(
        const utils::Str architecture,
        const utils::Set<utils::Str> dnu,
        const utils::List<utils::Str> debug_dumpers
    ) const;

    /**
     * Builds a pass instance.
     */
    AbstractPass::Ref build_pass(
        const utils::Str &type_name,
        const utils::Str &instance_name
    ) const;

    /**
     * Prefixes and suffixes the given pass list with the debug dumpers
     * configured for this factory.
     */
    void add_debug_dumpers(
        utils::List<AbstractPass::Ref> &passes
    ) const;

};

/**
 * Global pass registry. The passes provided by OpenQL must register themselves
 * with this.
 */
QL_GLOBAL extern PassFactory pass_registry;

} // namespace driv
} // namespace ql
