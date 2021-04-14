/** \file
 * Defines the base class for scheduler resources.
 */

#include "ql/plat/resource/base.h"

namespace ql {
namespace plat {
namespace resource {

/**
 * Stream operator for Direction.
 */
std::ostream &operator<<(std::ostream &os, Direction dir) {
    switch (dir) {
        case Direction::FORWARD: os << "forward"; break;
        case Direction::BACKWARD: os << "backward"; break;
        case Direction::UNDEFINED: os << "undefined"; break;
    }
    return os;
}

/**
 * Constructs the abstract resource. No error checking here; this is up to
 * the resource manager.
 */
Base::Base(
    const utils::Str &type_name,
    const utils::Str &instance_name,
    const plat::PlatformRef &platform,
    Direction direction
) :
    type_name(type_name),
    instance_name(instance_name),
    platform(platform),
    direction(direction),
    prev_cycle(direction == Direction::FORWARD ? 0 : utils::UMAX)
{
}

/**
 * Returns the type name for this resource.
 */
const utils::Str &Base::get_type() const {
    return type_name;
}

/**
 * Returns the user-specified or generated unique instance name for this
 * resource.
 */
const utils::Str &Base::get_name() const {
    return instance_name;
}

/**
 * Checks and optionally updates the resource manager state for the given
 * gate and (start) cycle number. The state is only updated if the gate is
 * schedulable for the given cycle and commit is set.
 */
utils::Bool Base::gate(
    utils::UInt cycle,
    const ir::GateRef &gate,
    utils::Bool commit
) {

    // Verify that the scheduling direction (if any) is respected.
    switch (direction) {
        case Direction::FORWARD: QL_ASSERT(cycle >= prev_cycle); break;
        case Direction::BACKWARD: QL_ASSERT(cycle <= prev_cycle); break;
        default: void();
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
 * Shorthand for gate() with commit set to false.
 */
utils::Bool Base::available(
    utils::UInt cycle,
    const ir::GateRef &gate
) {
    return this->gate(cycle, gate, false);
}

/**
 * Shorthand for gate() with commit set to true and an exception on failure.
 */
void Base::reserve(
    utils::UInt cycle,
    const ir::GateRef &gate
) {
    if (!this->gate(cycle, gate, true)) {
        utils::StrStrm ss;
        ss << "failed to reserve " << gate->qasm();
        ss << " for cycle " << cycle;
        ss << " with resource " << get_name();
        ss << " of type " << get_type();
        throw utils::Exception(ss.str());
    }
}

} // namespace resource
} // namespace plat
} // namespace ql
