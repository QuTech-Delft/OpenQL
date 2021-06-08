/** \file
 * Defines the base class for scheduler resources.
 */

#include "ql/rmgr/resource_types/base.h"

namespace ql {
namespace rmgr {
namespace resource_types {

/**
 * Constructs the abstract resource. No error checking here; this is up to
 * the resource manager.
 */
Base::Base(const Context &context) :
    context(context),
    initialized(false),
    direction(Direction::UNDEFINED),
    prev_cycle(0)
{
}

/**
 * Abstract implementation for initialize(). This is where the JSON
 * structure should be parsed and the resource state should be initialized.
 * This will only be called once during the lifetime of this resource. The
 * default implementation is no-op.
 */
void Base::on_initialize(Direction direction) {
    (void)direction;
}

/**
 * Returns the type name for this resource.
 */
const utils::Str &Base::get_type() const {
    return context->type_name;
}

/**
 * Returns the user-specified or generated unique instance name for this
 * resource.
 */
const utils::Str &Base::get_name() const {
    return context->instance_name;
}

/**
 * Writes the documentation for this resource to the given output stream.
 * May depend on type_name, but should not depend on anything else. The help
 * information should end in a newline, and every line printed should start
 * with line_prefix.
 */
void Base::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    on_dump_docs(os, line_prefix);
}

/**
 * Writes information about the configuration of this resource. This is
 * called before initialize(). The printed information should end in a
 * newline, and every line printed should start with line_prefix.
 */
void Base::dump_config(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    on_dump_config(os, line_prefix);
}

/**
 * Initializes the state for this resource for a particular scheduling
 * direction.
 */
void Base::initialize(Direction direction) {
    if (initialized) {
        throw utils::Exception("resource initialize() called twice");
    }
    this->direction = direction;
    if (direction == Direction::FORWARD) {
        prev_cycle = 0;
    } else {
        prev_cycle = utils::UMAX;
    }
    on_initialize(direction);
    initialized = true;
}

/**
 * Checks and optionally updates the resource manager state for the given
 * gate and (start) cycle number. The state is only updated if the gate is
 * schedulable for the given cycle and commit is set.
 */
utils::Bool Base::gate(
    utils::UInt cycle,
    const ir::compat::GateRef &gate,
    utils::Bool commit
) {
    if (!initialized) {
        throw utils::Exception("resource gate() called before initialization");
    }

    // Verify that the scheduling direction (if any) is respected. If not,
    // simply return false. The behavior of the old resources was basically
    // undefined, which we're not going to emulate...
    QL_DOUT("commit = " << commit << ", cycle = " << cycle << ", prev = " << prev_cycle);
    utils::Bool out_of_order = false;
    switch (direction) {
        case Direction::FORWARD: out_of_order = cycle < prev_cycle; break;
        case Direction::BACKWARD: out_of_order = cycle > prev_cycle; break;
        default: void();
    }
    if (out_of_order) {
        return false;
    }

    // Run the resource implementation.
    utils::Bool retval = on_gate(cycle, gate, commit);

    // If the above committed a gate, update prev_cycle.
    if (retval && commit) {
        prev_cycle = cycle;
    }

    return retval;
}

/**
 * Dumps a debug representation of the current resource state.
 */
void Base::dump_state(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    if (!initialized) {
        throw utils::Exception("resource dump_state() called before initialization");
    }
    on_dump_state(os, line_prefix);
}

} // namespace resource_types
} // namespace rmgr
} // namespace ql
