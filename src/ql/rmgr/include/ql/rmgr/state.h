/** \file
 * Defines a class for tracking the state of a collection of initialized
 * resources.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/vec.h"
#include "ql/ir/compat/compat.h"
#include "ql/rmgr/declarations.h"
#include "ql/rmgr/resource_types/base.h"
#include "ql/rmgr/factory.h"

namespace ql {
namespace rmgr {

// Forward declaration for the manager, so we can declare it as a friend.
class Manager;

/**
 * Maintains the state of a collection of scheduling resources.
 */
class State {
private:
    friend class Manager;

    /**
     * The list of resources and their state.
     */
    utils::Vec<ResourceRef> resources;

    /**
     * Set when reserve() returned an error, implying that the resources are in
     * an inconsistent state. When set, further calls to available() and
     * reserve() will immediately throw an exception.
     */
    utils::Bool is_broken;

    /**
     * Constructor for the initial state, called from Manager::build().
     */
    State();

public:

    /**
     * Copy constructor that clones the resource states.
     */
    State(const State &src);

    /**
     * Move constructor.
     */
    State(State &&src) = default;

    /**
     * Copy assignment operator that clones the resource states.
     */
    State &operator=(const State &src);

    /**
     * Move assignment operator.
     */
    State &operator=(State &&src) = default;

    /**
     * Checks whether the given old-IR gate can be scheduled at the given
     * (start) cycle.
     */
    utils::Bool available(
        utils::UInt cycle,
        const ir::compat::GateRef &gate
    ) const;

    /**
     * Checks whether the given new-IR statement can be scheduled at the given
     * (start) cycle. Note that the cycle number may be negative.
     */
    utils::Bool available(
        utils::Int cycle,
        const ir::StatementRef &statement
    ) const;

    /**
     * Schedules the given old-IR gate at the given (start) cycle. Throws an
     * exception if this is not possible. When an exception is thrown, the
     * resulting state of the resources is undefined.
     */
    void reserve(
        utils::UInt cycle,
        const ir::compat::GateRef &gate
    );

    /**
     * Schedules the given new-IR statement at the given (start) cycle. Throws
     * an exception if this is not possible. When an exception is thrown, the
     * resulting state of the resources is undefined. Note that the cycle number
     * may be negative.
     */
    void reserve(
        utils::Int cycle,
        const ir::StatementRef &statement
    );

    /**
     * Dumps a debug representation of the current resource state.
     */
    void dump(
        std::ostream &os = std::cout,
        const utils::Str &line_prefix = ""
    ) const;

};

} // namespace rmgr
} // namespacq ql
