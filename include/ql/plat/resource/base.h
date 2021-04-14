/** \file
 * Defines the base class for scheduler resources.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/plat/platform.h"
#include "ql/ir/ir.h"

namespace ql {
namespace plat {
namespace resource {

/**
 * The direction in which gates are presented to a resource, allowing the
 * resource to optimize its state.
 */
enum class Direction {

    /**
     * Gates are only reserved with non-decreasing cycle numbers.
     */
    FORWARD,

    /**
     * Gates are only reserved with non-increasing cycle numbers.
     */
    BACKWARD,

    /**
     * available() and reserve() may be called with any cycle number.
     */
    UNDEFINED

};

/**
 * Stream operator for Direction.
 */
std::ostream &operator<<(std::ostream &os, Direction dir);

/**
 * Base class for scheduling resources. Scheduling resources are used to
 * represent constraints on when gates can be executed in a schedule, within
 * context of other gates.
 */
class Base {
protected:

    /**
     * The full type name for this resource. This is the full name that was used
     * when the resource was registered with the resource factory. The same
     * resource class may be registered with multiple type names, in which case
     * the pass implementation may use this to differentiate.
     */
    const utils::Str type_name;

    /**
     * The instance name for this resource, i.e. the name that the user assigned
     * to it or the name that was assigned to it automatically. Must match
     * `[a-zA-Z0-9_\-]+`, and must be unique within a resource manager.
     * Instance names should NOT have a semantic meaning; they are only intended
     * for logging.
     */
    const utils::Str instance_name;

    /**
     * Reference to the platform for contextual information.
     */
    const plat::PlatformRef platform;

    /**
     * The directing in which gates are scheduled, if any.
     */
    const Direction direction;

private:

    /**
     * Used to verify that gates are added in the order specified by direction.
     */
    utils::UInt prev_cycle;

protected:

    /**
     * Constructs the abstract resource. No error checking here; this is up to
     * the resource manager.
     */
    Base(
        const utils::Str &type_name,
        const utils::Str &instance_name,
        const plat::PlatformRef &platform,
        Direction direction
    );

    /**
     * Abstract implementation for gate().
     */
    virtual utils::Bool on_gate(
        utils::UInt cycle,
        const ir::GateRef &gate,
        utils::Bool commit
    ) = 0;

public:

    /**
     * Virtual destructor for proper polymorphism.
     */
    virtual ~Base() = default;

    /**
     * Returns the type name for this resource.
     */
    const utils::Str &get_type() const;

    /**
     * Returns the user-specified or generated unique instance name for this
     * resource.
     */
    const utils::Str &get_name() const;

    /**
     * Writes the documentation for this resource to the given output stream.
     * May depend on type_name, but should not depend on anything else. The help
     * information should end in a newline, and every line printed should start
     * with line_prefix.
     */
    virtual void dump_docs(
        std::ostream &os = std::cout,
        const utils::Str &line_prefix = ""
    ) const = 0;

    /**
     * Dumps a debug representation of the current resource state.
     */
    virtual void dump_state(
        std::ostream &os = std::cout,
        const utils::Str &line_prefix = ""
    ) const = 0;

    /**
     * Checks and optionally updates the resource manager state for the given
     * gate and (start) cycle number. The state is only updated if the gate is
     * schedulable for the given cycle and commit is set.
     */
    utils::Bool gate(
        utils::UInt cycle,
        const ir::GateRef &gate,
        utils::Bool commit
    );

    /**
     * Shorthand for gate() with commit set to false.
     */
    utils::Bool available(
        utils::UInt cycle,
        const ir::GateRef &gate
    );

    /**
     * Shorthand for gate() with commit set to true and an exception on failure.
     */
    void reserve(
        utils::UInt cycle,
        const ir::GateRef &gate
    );

};

/**
 * A reference to a resource.
 */
using Ref = utils::ClonablePtr<Base>;

} // namespace resource
} // namespace plat
} // namespacq ql
