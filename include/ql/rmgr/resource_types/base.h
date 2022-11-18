/** \file
 * Defines the base class for scheduler resources.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/ir/compat/compat.h"
#include "ql/ir/ir.h"
#include "ql/rmgr/types.h"

namespace ql {
namespace rmgr {
namespace resource_types {

/**
 * Record containing information about a gate/statement being fed to the
 * resource. This is supposed to be a temporary construct, needed because the
 * resource manager currently has to work with two kinds of IRs. This simply
 * acts as the least common denominator between them. When the old IR is phased
 * out, this structure can be removed, and gate()/on_gate() can be updated to
 * accept an ir::StatementRef directly.
 */
struct GateData {

    /**
     * The complete gate reference when operating on the old IR. This is empty
     * when operating on the new IR.
     */
    ir::compat::GateRef gate;

    /**
     * The complete statement reference when operating on the new IR. This is
     * empty when operating on the old IR.
     */
    ir::StatementRef statement;

    /**
     * Reference to the name of the gate, valid for either IR type.
     */
    utils::Str name;

    /**
     * Reference to the duration of the gate in cycles, valid for either IR
     * type.
     */
    utils::UInt duration_cycles;

    /**
     * If the old IR is used or the new IR statement is a quantum gate operating
     * on the main qubit register, this is populated with the qubit indices.
     * Otherwise, it will be empty.
     */
    utils::Vec<utils::UInt> qubits;

    /**
     * JSON data from the instruction definition in the platform configuration
     * file.
     */
    utils::RawPtr<const utils::Json> data;

};

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
    utils::Int prev_cycle;

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
        utils::Int cycle,
        const GateData &gate,
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
     * gate data structure and (start) cycle number. Note that the cycle number
     * may be negative. The state is only updated if the gate is schedulable for
     * the given cycle and commit is set.
     */
    utils::Bool gate(
        utils::Int cycle,
        const GateData &data,
        utils::Bool commit
    );

    /**
     * Checks and optionally updates the resource manager state for the given
     * old-IR gate and (start) cycle number. The state is only updated if the
     * gate is schedulable for the given cycle and commit is set.
     */
    utils::Bool gate(
        utils::Int cycle,
        const ir::compat::GateRef &gate,
        utils::Bool commit
    );

    /**
     * Checks and optionally updates the resource manager state for the given
     * new-IR statement and (start) cycle number. Note that cycles may be
     * negative in the new IR during scheduling. The state is only updated if
     * the gate is schedulable for the given cycle and commit is set.
     */
    utils::Bool gate(
        utils::Int cycle,
        const ir::StatementRef &statement,
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
using Ref = utils::CloneablePtr<Base>;

/**
 * An immutable reference to a resource.
 */
using CRef = utils::CloneablePtr<const Base>;

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
