/** \file
 * Defines the qubit resource. This resource prevents a qubit from being used
 * more than once in each cycle.
 */

#include "ql/resource/qubit.h"

namespace ql {
namespace resource {
namespace qubit {

/**
 * Initializes this resource.
 */
void QubitResource::on_initialize(rmgr::Direction direction) {
    state = utils::Vec<State>(context->platform->qubit_count);
    optimize = direction != rmgr::Direction::UNDEFINED;
    cycle_time = context->platform->cycle_time;
}

/**
 * Checks availability of and/or reserves a gate.
 */
utils::Bool QubitResource::on_gate(
    utils::UInt cycle,
    const ir::GateRef &gate,
    utils::Bool commit
) {

    // Compute cycle range for this gate.
    State::Range range = {
        cycle,
        cycle + utils::div_ceil(gate->duration, cycle_time)
    };

    // Check qubit availability for all operands.
    for (auto qubit : gate->operands) {
        if (state[qubit].find(range).type != utils::RangeMatchType::NONE) {
            return false;
        }
    }

    // If we're committing, reserve for all operands.
    if (commit) {
        for (auto qubit : gate->operands) {
            if (optimize) {
                state[qubit].clear();
            }
            state[qubit].set(range);
        }
    }

    return true;
}

/**
 * Dumps documentation for this resource.
 */
void QubitResource::on_dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix, R"(
    This resource ensures that a qubit is only ever in use by one gate at a
    time.

    NOTE: it assumes that a gate with a qubit operand actually uses this operand
    for its entire duration, so it may be overly pessimistic.

    This resource does not have any JSON configuration options. Historically it
    had a "count" key specifying the number of qubits, but this is now taken
    from the platform's qubit count. Any JSON options that are passed anyway are
    silently ignored.
    )");
}

/**
 * Dumps the configuration of this resource.
 */
void QubitResource::on_dump_config(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix, R"(
    The qubit resource has no configuration.
    )");
}

/**
 * Dumps the state of this resource.
 */
void QubitResource::on_dump_state(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    for (utils::UInt q = 0; q < state.size(); q++) {
        os << line_prefix << "Qubit " << q << ":\n";
        state[q].dump_state(os, line_prefix + "  ");
    }
}

/**
 * Constructs the resource. No error checking here; this is up to the
 * resource manager.
 */
QubitResource::QubitResource(
    const rmgr::Context &context
) :
    rmgr::resource_types::Base(context)
{ }

/**
 * Returns a user-friendly type name for this resource.
 */
utils::Str QubitResource::get_friendly_type() const {
    return "Qubit resource";
}

} // namespace qubit
} // namespace resource
} // namespace ql
