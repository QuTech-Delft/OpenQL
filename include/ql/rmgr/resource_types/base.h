/** \file
 * Defines the base class for scheduler resources.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/ir/ir.h"
#include "ql/rmgr/types.h"

namespace ql {
namespace rmgr {
namespace resource_types {

/**
 * Base class for scheduling resources. Scheduling resources are used to
 * represent constraints on when gates can be executed in a schedule, within
 * context of other gates.
 */
class Base {
protected:

    /**
     * The context information that the resource was constructed with. This is
     * wrapped in a Ptr so it doesn't need to be cloned every time the resource
     * state is cloned.
     */
    const utils::Ptr<const Context> context;

private:

    /**
     * Whether our state has been initialized yet.
     */
    utils::Bool initialized;

    /**
     * The scheduling direction.
     */
    Direction direction;

    /**
     * Used to verify that gates are added in the order specified by direction.
     */
    utils::UInt prev_cycle;

protected:

    /**
     * Constructs the abstract resource. No error checking here; this is up to
     * the resource manager.
     */
    explicit Base(const Context &context);

    /**
     * Abstract implementation for initialize(). This is where the JSON
     * structure should be parsed and the resource state should be initialized.
     * This will only be called once during the lifetime of this resource. The
     * default implementation is no-op.
     */
    virtual void on_initialize(Direction direction);

    /**
     * Abstract implementation for gate().
     */
    virtual utils::Bool on_gate(
        utils::UInt cycle,
        const ir::GateRef &gate,
        utils::Bool commit
    ) = 0;

    /**
     * Abstract implementation for dump_docs().
     */
    virtual void on_dump_docs(
        std::ostream &os,
        const utils::Str &line_prefix
    ) const = 0;

    /**
     * Abstract implementation for dump_config().
     */
    virtual void on_dump_config(
        std::ostream &os,
        const utils::Str &line_prefix
    ) const = 0;

    /**
     * Abstract implementation for dump_state().
     */
    virtual void on_dump_state(
        std::ostream &os,
        const utils::Str &line_prefix
    ) const = 0;

public:

    /**
     * Virtual destructor for proper polymorphism.
     */
    virtual ~Base() = default;

    /**
     * Returns a user-friendly type name for this resource.
     */
    virtual utils::Str get_friendly_type() const = 0;

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
    void dump_docs(
        std::ostream &os = std::cout,
        const utils::Str &line_prefix = ""
    ) const;

    /**
     * Writes information about the configuration of this resource. This is
     * called before initialize(). The printed information should end in a
     * newline, and every line printed should start with line_prefix.
     */
    void dump_config(
        std::ostream &os = std::cout,
        const utils::Str &line_prefix = ""
    ) const;

    /**
     * Initializes the state for this resource for a particular scheduling
     * direction.
     */
    void initialize(Direction direction);

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
     * Dumps a debug representation of the current resource state.
     */
    void dump_state(
        std::ostream &os = std::cout,
        const utils::Str &line_prefix = ""
    ) const;

};

/**
 * A mutable reference to a resource.
 */
using Ref = utils::ClonablePtr<Base>;

/**
 * An immutable reference to a resource.
 */
using CRef = utils::ClonablePtr<const Base>;

} // namespace resource_types

/**
 * Shorthand for a reference to any resource type.
 */
using ResourceRef = resource_types::Ref;

/**
 * Shorthand for an immutable reference to any resource type.
 */
using CResourceRef = resource_types::CRef;

} // namespace rmgr
} // namespacq ql
