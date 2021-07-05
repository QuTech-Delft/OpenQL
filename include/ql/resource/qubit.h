/** \file
 * Defines the qubit resource. This resource prevents a qubit from being used
 * more than once in each cycle.
 */

#pragma once

#include "ql/utils/rangemap.h"
#include "ql/rmgr/resource_types/base.h"

namespace ql {
namespace resource {
namespace qubit {

/**
 * State per qubit.
 */
using State = utils::RangeSet<utils::UInt>;

/**
 * Qubit resource. This resource prevents a qubit from being used more than once
 * in each cycle.
 */
class QubitResource : public rmgr::resource_types::Base {
private:

    /**
     * The reservations for each qubit.
     */
    utils::Vec<State> state;

    /**
     * When set, there is a defined scheduling direction, which means it's
     * sufficient to only track the latest reservation for each qubit.
     */
    utils::Bool optimize;

    /**
     * Cycle time of the platform.
     */
    utils::UInt cycle_time;

protected:

    /**
     * Initializes this resource.
     */
    void on_initialize(rmgr::Direction direction) override;

    /**
     * Checks availability of and/or reserves a gate.
     */
    utils::Bool on_gate(
        utils::UInt cycle,
        const ir::GateRef &gate,
        utils::Bool commit
    ) override;

    /**
     * Dumps documentation for this resource.
     */
    void on_dump_docs(
        std::ostream &os,
        const utils::Str &line_prefix
    ) const override;

    /**
     * Dumps the configuration of this resource.
     */
    void on_dump_config(
        std::ostream &os,
        const utils::Str &line_prefix
    ) const override;

    /**
     * Dumps the state of this resource.
     */
    void on_dump_state(
        std::ostream &os,
        const utils::Str &line_prefix
    ) const override;

public:

    /**
     * Constructs the resource. No error checking here; this is up to the
     * resource manager.
     */
    explicit QubitResource(const rmgr::Context &context);

    /**
     * Returns a user-friendly type name for this resource.
     */
    utils::Str get_friendly_type() const override;

};

/**
 * Shorthand for namespace-based notation.
 */
using Resource = QubitResource;

} // namespace qubit
} // namespace resource
} // namespace ql
