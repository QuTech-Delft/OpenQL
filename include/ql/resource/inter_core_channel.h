/** \file
 * Defines the channel resource. This limits the amount of parallel
 * communication between cores in a multi-core system.
 */

#pragma once

#include "ql/utils/rangemap.h"
#include "ql/rmgr/resource_types/base.h"

namespace ql {
namespace resource {
namespace inter_core_channel {

/**
 * State per qubit.
 */
using State = utils::RangeSet<utils::UInt>;

/**
 * Forward-declaration for the configuration structure, defined in the CC file.
 */
struct Config;

/**
 * Communication channel resource. This limits the amount of parallel
 * communication between cores in a multi-core system.
 */
class InterCoreChannelResource : public rmgr::resource_types::Base {
private:

    /**
     * The reservations for each [core][channel].
     */
    utils::Vec<utils::Vec<State>> state;

    /**
     * Shared pointer to the configuration structure.
     */
    utils::Ptr<Config> config;

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
    explicit InterCoreChannelResource(const rmgr::Context &context);

    /**
     * Returns a user-friendly type name for this resource.
     */
    utils::Str get_friendly_type() const override;

};

/**
 * Shorthand for namespace-based notation.
 */
using Resource = InterCoreChannelResource;

} // namespace inter_core_channel
} // namespace resource
} // namespace ql
