/** \file
 * Defines specialized abstract classes for passes. These are all abstract, to
 * be implemented by actual passes; only common functionality is provided.
 */

#include "ql/pmgr/pass_types/specializations.h"

#include "ql/ir/new_to_old.h"
#include "ql/ir/old_to_new.h"

namespace ql {
namespace pmgr {
namespace pass_types {

/**
 * Constructs the abstract pass group. No error checking here; this is up to
 * the parent pass group.
 */
Group::Group(
    const utils::Ptr<const Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : Base(pass_factory, type_name, instance_name) {
}

/**
 * Simple implementation for on_construct() that always returns true and
 * defers to get_passes() for the initial pass list.
 */
NodeType Group::on_construct(
    const utils::Ptr<const Factory> &factory,
    utils::List<Ref> &passes,
    condition::Ref &condition
) {
    get_passes(factory, passes);
    return NodeType::GROUP;
}

/**
 * Dummy implementation for compilation. Should never be called, as this
 * pass always behaves as an unconditional group. Thus, it just throws an
 * exception.
 */
utils::Int Group::run_internal(
    const ir::Ref &ir,
    const Context &context
) const {
    QL_ASSERT(false);
}

/**
 * Constructs the normal pass. No error checking here; this is up to the
 * parent pass group.
 */
Normal::Normal(
    const utils::Ptr<const Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : Base(pass_factory, instance_name, type_name) {
}

/**
 * Default implementation for on_construct() that makes this a normal pass.
 * May be overridden to allow the pass to generate into a group as well,
 * based on its options.
 */
NodeType Normal::on_construct(
    const utils::Ptr<const Factory> &factory,
    utils::List<Ref> &passes,
    condition::Ref &condition
) {
    return NodeType::NORMAL;
}

/**
 * Constructs the pass. No error checking here; this is up to the parent
 * pass group.
 */
Transformation::Transformation(
    const utils::Ptr<const Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : Normal(pass_factory, instance_name, type_name) {
}

/**
 * Implementation for on_compile() that calls run() appropriately.
 */
utils::Int Transformation::run_internal(
    const ir::Ref &ir,
    const Context &context
) const {
    return run(ir, context);
}

/**
 * Constructs the pass. No error checking here; this is up to the parent
 * pass group.
 */
ProgramTransformation::ProgramTransformation(
    const utils::Ptr<const Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : Normal(pass_factory, instance_name, type_name) {
}

/**
 * Implementation for on_compile() that calls run() appropriately.
 */
utils::Int ProgramTransformation::run_internal(
    const ir::Ref &ir,
    const Context &context
) const {
    auto program = ir::convert_new_to_old(ir);
    auto retval = run(program, context);
    auto new_ir = ir::convert_old_to_new(program);
    ir->program = new_ir->program;
    ir->platform = new_ir->platform;
    ir->copy_annotations(*new_ir);
    return retval;
}

/**
 * Constructs the pass. No error checking here; this is up to the parent
 * pass group.
 */
KernelTransformation::KernelTransformation(
    const utils::Ptr<const Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : Normal(pass_factory, instance_name, type_name) {
}

/**
 * Initial accumulator value for the return value. Defaults to zero.
 */
utils::Int KernelTransformation::retval_initialize() const {
    return 0;
}

/**
 * Return value reduction operator. Defaults to addition.
 */
utils::Int KernelTransformation::retval_accumulate(
    utils::Int state,
    utils::Int kernel
) const {
    return state + kernel;
}

/**
 * Implementation for on_compile() that calls run() appropriately.
 */
utils::Int KernelTransformation::run_internal(
    const ir::Ref &ir,
    const Context &context
) const {
    auto program = ir::convert_new_to_old(ir);
    utils::Int accumulator = retval_initialize();
    for (const auto &kernel : program->kernels) {
        accumulator = retval_accumulate(accumulator, run(program, kernel, context));
    }
    auto new_ir = ir::convert_old_to_new(program);
    ir->program = new_ir->program;
    ir->platform = new_ir->platform;
    ir->copy_annotations(*new_ir);
    return accumulator;
}

/**
 * Constructs the pass. No error checking here; this is up to the parent
 * pass group.
 */
Analysis::Analysis(
    const utils::Ptr<const Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : Normal(pass_factory, instance_name, type_name) {
}

/**
 * Implementation for on_compile() that calls run() appropriately.
 */
utils::Int Analysis::run_internal(
    const ir::Ref &ir,
    const Context &context
) const {
    return run(ir, context);
}

/**
 * Constructs the pass. No error checking here; this is up to the parent
 * pass group.
 */
ProgramAnalysis::ProgramAnalysis(
    const utils::Ptr<const Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : Normal(pass_factory, instance_name, type_name) {
}

/**
 * Implementation for on_compile() that calls run() appropriately.
 */
utils::Int ProgramAnalysis::run_internal(
    const ir::Ref &ir,
    const Context &context
) const {
    auto program = ir::convert_new_to_old(ir);
    auto retval = run(program, context);
    auto new_ir = ir::convert_old_to_new(program);
    ir->program = new_ir->program;
    ir->platform = new_ir->platform;
    ir->copy_annotations(*new_ir);
    return retval;
}

/**
 * Constructs the pass. No error checking here; this is up to the parent
 * pass group.
 */
KernelAnalysis::KernelAnalysis(
    const utils::Ptr<const Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : Normal(pass_factory, instance_name, type_name) {
}

/**
 * Initial accumulator value for the return value. Defaults to zero.
 */
utils::Int KernelAnalysis::retval_initialize() const {
    return 0;
}

/**
 * Return value reduction operator. Defaults to addition.
 */
utils::Int KernelAnalysis::retval_accumulate(
    utils::Int state,
    utils::Int kernel
) const {
    return state + kernel;
}

/**
 * Implementation for on_compile() that calls run() appropriately.
 */
utils::Int KernelAnalysis::run_internal(
    const ir::Ref &ir,
    const Context &context
) const {
    auto program = ir::convert_new_to_old(ir);
    utils::Int accumulator = retval_initialize();
    for (const auto &kernel : program->kernels) {
        accumulator = retval_accumulate(accumulator, run(program, kernel, context));
    }
    auto new_ir = ir::convert_old_to_new(program);
    ir->program = new_ir->program;
    ir->platform = new_ir->platform;
    ir->copy_annotations(*new_ir);
    return accumulator;
}

} // namespace pass_types
} // namespace pmgr
} // namespace ql
