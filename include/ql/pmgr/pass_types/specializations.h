/** \file
 * Defines specialized abstract classes for passes. These are all abstract, to
 * be implemented by actual passes; only common functionality is provided.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/ptr.h"
#include "ql/ir/compat/compat.h"
#include "ql/ir/ir.h"
#include "ql/pmgr/condition.h"
#include "ql/pmgr/pass_types/base.h"

namespace ql {
namespace pmgr {
namespace pass_types {

/**
 * A pass type for passes that always construct into a simple group. For
 * example, a generic optimizer pass with an option-configured set of
 * optimization passes would derive from this.
 */
class Group : public Base {
protected:

    /**
     * Constructs the abstract pass group. No error checking here; this is up to
     * the parent pass group.
     */
    Group(
        const utils::Ptr<const Factory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Simple implementation for on_construct() that always returns true and
     * defers to get_passes() for the initial pass list.
     */
    NodeType on_construct(
        const utils::Ptr<const Factory> &factory,
        utils::List<Ref> &passes,
        condition::Ref &condition
    ) final;

    /**
     * Dummy implementation for compilation. Should never be called, as this
     * pass always behaves as an unconditional group. Thus, it just throws an
     * exception.
     */
    utils::Int run_internal(
        const ir::Ref &ir,
        const Context &context
    ) const final;

    /**
     * Overridable implementation that returns the initial pass list for this
     * pass group. The default implementation is no-op.
     */
    virtual void get_passes(
        const utils::Ptr<const Factory> &factory,
        utils::List<Ref> &passes
    ) = 0;

};

/**
 * A pass type for regular passes that normally don't construct into a group
 * (although this is still possible). Just provides a default implementation for
 * on_construct().
 */
class Normal : public Base {
protected:

    /**
     * Constructs the normal pass. No error checking here; this is up to the
     * parent pass group.
     */
    Normal(
        const utils::Ptr<const Factory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Default implementation for on_construct() that makes this a normal pass.
     * May be overridden to allow the pass to generate into a group as well,
     * based on its options.
     */
    NodeType on_construct(
        const utils::Ptr<const Factory> &factory,
        utils::List<Ref> &passes,
        condition::Ref &condition
    ) override;

};

/**
 * A pass type for passes that transform the IR.
 */
class Transformation : public Normal {
protected:

    /**
     * Constructs the pass. No error checking here; this is up to the parent
     * pass group.
     */
    Transformation(
        const utils::Ptr<const Factory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Implementation for on_compile() that calls run() appropriately.
     */
    utils::Int run_internal(
        const ir::Ref &ir,
        const Context &context
    ) const final;

    /**
     * The virtual implementation for this pass.
     */
    virtual utils::Int run(
        const ir::Ref &ir,
        const Context &context
    ) const = 0;

};

/**
 * A pass type for passes that apply a program-wide transformation using the
 * old IR.
 */
class ProgramTransformation : public Normal {
protected:

    /**
     * Constructs the pass. No error checking here; this is up to the parent
     * pass group.
     */
    ProgramTransformation(
        const utils::Ptr<const Factory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Implementation for on_compile() that calls run() appropriately.
     */
    utils::Int run_internal(
        const ir::Ref &ir,
        const Context &context
    ) const final;

    /**
     * The virtual implementation for this pass.
     */
    virtual utils::Int run(
        const ir::compat::ProgramRef &program,
        const Context &context
    ) const = 0;

    /**
     * Returns that this is a legacy pass.
     */
    utils::Bool is_legacy() const override;

};

/**
 * A pass type for passes that apply a transformation per kernel/basic block
 * using the old IR.
 */
class KernelTransformation : public Normal {
protected:

    /**
     * Constructs the pass. No error checking here; this is up to the parent
     * pass group.
     */
    KernelTransformation(
        const utils::Ptr<const Factory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Implementation for on_compile() that calls run() appropriately.
     */
    utils::Int run_internal(
        const ir::Ref &ir,
        const Context &context
    ) const final;

    /**
     * Initial accumulator value for the return value. Defaults to zero.
     */
    virtual utils::Int retval_initialize() const;

    /**
     * Return value reduction operator. Defaults to addition.
     */
    virtual utils::Int retval_accumulate(utils::Int state, utils::Int kernel) const;

    /**
     * The virtual implementation for this pass.
     */
    virtual utils::Int run(
        const ir::compat::ProgramRef &program,
        const ir::compat::KernelRef &kernel,
        const Context &context
    ) const = 0;

    /**
     * Returns that this is a legacy pass.
     */
    utils::Bool is_legacy() const override;

};

/**
 * A pass type for passes that analyze the IR without modifying it.
 */
class Analysis : public Normal {
protected:

    /**
     * Constructs the pass. No error checking here; this is up to the parent
     * pass group.
     */
    Analysis(
        const utils::Ptr<const Factory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Implementation for on_compile() that calls run() appropriately.
     */
    utils::Int run_internal(
        const ir::Ref &ir,
        const Context &context
    ) const final;

    /**
     * The virtual implementation for this pass.
     */
    virtual utils::Int run(
        const ir::Ref &ir,
        const Context &context
    ) const = 0;

};

/**
 * A pass type for passes that analyze the complete program using the old IR
 * without modifying it.
 */
class ProgramAnalysis : public Normal {
protected:

    /**
     * Constructs the pass. No error checking here; this is up to the parent
     * pass group.
     */
    ProgramAnalysis(
        const utils::Ptr<const Factory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Implementation for on_compile() that calls run() appropriately.
     */
    utils::Int run_internal(
        const ir::Ref &ir,
        const Context &context
    ) const final;

    /**
     * The virtual implementation for this pass. The contents of platform and
     * program must not be modified.
     */
    virtual utils::Int run(
        const ir::compat::ProgramRef &program,
        const Context &context
    ) const = 0;

    /**
     * Returns that this is a legacy pass.
     */
    utils::Bool is_legacy() const override;

};

} // namespace pass_types
} // namespace pmgr
} // namespace ql
