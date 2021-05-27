/** \file
 * Defines a class for tracking the state of a collection of initialized
 * resources.
 */

#include "ql/rmgr/state.h"

namespace ql {
namespace rmgr {

/**
 * Constructor for the initial state, called from Manager::build().
 */
State::State() : resources(), is_broken(false) {
}

/**
 * Copy constructor that clones the resource states.
 */
State::State(const State &src) {
    resources.resize(src.resources.size());
    for (utils::UInt i = 0; i < src.resources.size(); i++) {
        resources[i] = src.resources[i].clone();
    }
    is_broken = src.is_broken;
}

/**
 * Copy assignment operator that clones the resource states.
 */
State &State::operator=(const State &src) {
    resources.resize(src.resources.size());
    for (utils::UInt i = 0; i < src.resources.size(); i++) {
        resources[i] = src.resources[i].clone();
    }
    is_broken = src.is_broken;
    return *this;
}

/**
 * Checks whether the given gate can be scheduled at the given (start)
 * cycle.
 */
utils::Bool State::available(
    utils::UInt cycle,
    const ir::GateRef &gate
) const {
    if (is_broken) {
        throw utils::Exception("usage of resource state that was left in an undefined state");
    }
    for (auto &resource : resources) {
        if (!resource->gate(cycle, gate, false)) {
            return false;
        }
    }
    return true;
}

/**
 * Schedules the given gate at the given (start) cycle. Throws an exception
 * if this is not possible. When an exception is thrown, the resulting state
 * of the resources is undefined.
 */
void State::reserve(
    utils::UInt cycle,
    const ir::GateRef &gate
) {
    if (is_broken) {
        throw utils::Exception("usage of resource state that was left in an undefined state");
    }
    for (auto &resource : resources) {
        if (!resource->gate(cycle, gate, true)) {
            is_broken = true;
            utils::StrStrm ss;
            ss << "failed to reserve " << gate->qasm();
            ss << " for cycle " << cycle;
            ss << " with resource " << resource->get_name();
            ss << " of type " << resource->get_type();
            throw utils::Exception(ss.str());
        }
    }
}

/**
 * Dumps a debug representation of the current resource state.
 */
void State::dump(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    for (const auto &resource : resources) {
        os << line_prefix << "Resource " << resource->get_name();
        os << " of type " << resource->get_type() << ":\n";
        resource->dump_state(os, line_prefix + "    ");
        os << "\n";
    }
    os.flush();
}

} // namespace rmgr
} // namespacq ql
