/** \file
 * Defines an instrument resource, used to model mutually-exlusive gates due
 * to instrument resource sharing.
 */

#pragma once

#include "ql/utils/set.h"
#include "ql/utils/rangemap.h"
#include "ql/rmgr/resource_types/base.h"

namespace ql {
namespace resource {
namespace instrument {

/**
 * State per instrument.
 */
using State = utils::RangeMap<utils::UInt, utils::UInt>;

/**
 * Forward-declaration for the configuration structure, defined in the CC file.
 */
struct Config;

/**
 * Instrument resource.
 */
class InstrumentResource : public rmgr::resource_types::Base {
private:

    /**
     * The reservations made for each instrument.
     */
    utils::Vec<State> state;

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
        const ir::compat::GateRef &gate,
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
    explicit InstrumentResource(const rmgr::Context &context);

    /**
     * Returns a user-friendly type name for this resource.
     */
    utils::Str get_friendly_type() const override;

};

/**
 * Shorthand for namespace-based notation.
 */
using Resource = InstrumentResource;

} // namespace instrument
} // namespace resource
} // namespace ql
