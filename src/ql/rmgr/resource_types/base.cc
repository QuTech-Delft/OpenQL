/** \file
 * Defines the base class for scheduler resources.
 */

#include "ql/rmgr/resource_types/base.h"

#include "ql/ir/describe.h"
#include "ql/ir/ir_gen_ex.h"
#include "ql/ir/ops.h"

namespace ql {
namespace rmgr {
namespace resource_types {

/**
 * Constructs the abstract resource. No error checking here; this is up to
 * the resource manager.
 */
Base::Base(const Context &context) :
    context(context),
    initialized(false),
    direction(Direction::UNDEFINED),
    prev_cycle(0)
{
}

/**
 * Abstract implementation for initialize(). This is where the JSON
 * structure should be parsed and the resource state should be initialized.
 * This will only be called once during the lifetime of this resource. The
 * default implementation is no-op.
 */
void Base::on_initialize(Direction direction) {
    (void)direction;
}

/**
 * Returns the type name for this resource.
 */
const utils::Str &Base::get_type() const {
    return context->type_name;
}

/**
 * Returns the user-specified or generated unique instance name for this
 * resource.
 */
const utils::Str &Base::get_name() const {
    return context->instance_name;
}

/**
 * Writes the documentation for this resource to the given output stream.
 * May depend on type_name, but should not depend on anything else. The help
 * information should end in a newline, and every line printed should start
 * with line_prefix.
 */
void Base::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    on_dump_docs(os, line_prefix);
}

/**
 * Writes information about the configuration of this resource. This is
 * called before initialize(). The printed information should end in a
 * newline, and every line printed should start with line_prefix.
 */
void Base::dump_config(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    on_dump_config(os, line_prefix);
}

/**
 * Initializes the state for this resource for a particular scheduling
 * direction.
 */
void Base::initialize(Direction direction) {
    if (initialized) {
        throw utils::Exception("resource initialize() called twice");
    }
    this->direction = direction;
    if (direction == Direction::FORWARD) {
        prev_cycle = utils::MIN;
    } else {
        prev_cycle = utils::MAX;
    }
    on_initialize(direction);
    initialized = true;
}

/**
 * Checks and optionally updates the resource manager state for the given
 * gate data structure and (start) cycle number. Note that the cycle number
 * may be negative. The state is only updated if the gate is schedulable for
 * the given cycle and commit is set.
 */
utils::Bool Base::gate(
    utils::Int cycle,
    const GateData &data,
    utils::Bool commit
) {
    if (!initialized) {
        throw utils::Exception("resource gate() called before initialization");
    }

    // Verify that the scheduling direction (if any) is respected. If not,
    // simply return false. The behavior of the old resources was basically
    // undefined, which we're not going to emulate...
    QL_DOUT("commit = " << commit << ", cycle = " << cycle << ", prev = " << prev_cycle);
    utils::Bool out_of_order = false;
    switch (direction) {
        case Direction::FORWARD: out_of_order = cycle < prev_cycle; break;
        case Direction::BACKWARD: out_of_order = cycle > prev_cycle; break;
        default: void();
    }
    if (out_of_order) {
        return false;
    }

    // Run the resource implementation.
    utils::Bool retval = on_gate(cycle, data, commit);

    // If the above committed a gate, update prev_cycle.
    if (retval && commit) {
        prev_cycle = cycle;
    }

    return retval;
}

/**
 * Checks and optionally updates the resource manager state for the given
 * old-IR gate and (start) cycle number. The state is only updated if the
 * gate is schedulable for the given cycle and commit is set.
 */
utils::Bool Base::gate(
    utils::Int cycle,
    const ir::compat::GateRef &gate,
    utils::Bool commit
) {
    if (!initialized) {
        throw utils::Exception("resource gate() called before initialization");
    }

    // Convert to GateData wrapper.
    GateData data;
    data.gate = gate;
    data.name = gate->name;
    data.duration_cycles = utils::div_ceil(gate->duration, context->platform->cycle_time);
    data.qubits = gate->operands;
    data.data = &context->platform->find_instruction(gate->name);

    return this->gate((utils::Int)cycle, data, commit);
}

/**
 * Checks and optionally updates the resource manager state for the given
 * new-IR statement and (start) cycle number. Note that cycles may be
 * negative in the new IR during scheduling. The state is only updated if
 * the gate is schedulable for the given cycle and commit is set.
 */
utils::Bool Base::gate(
    utils::Int cycle,
    const ir::StatementRef &statement,
    utils::Bool commit
) {
    if (!initialized) {
        throw utils::Exception("resource gate() called before initialization");
    }

    QL_DOUT("processing new-IR statement " << ir::describe(statement));

    // Convert to GateData wrapper.
    static const utils::Json EMPTY = {};
    GateData data;
    data.statement = statement;
    data.duration_cycles = ir::get_duration_of_statement(statement);

    // Figure out a name and JSON data record in all cases.
    if (auto custom = statement->as_custom_instruction()) {
        data.name = custom->instruction_type->name;
        data.data = &custom->instruction_type->data.data;
    } else if (statement->as_set_instruction()) {
        data.name = "set";
        data.data = &EMPTY;
    } else if (statement->as_goto_instruction()) {
        data.name = "goto";
        data.data = &EMPTY;
    } else if (statement->as_wait_instruction()) {
        data.name = "wait";
        data.data = &EMPTY;
    } else if (statement->as_break_statement()) {
        data.name = "break";
        data.data = &EMPTY;
    } else if (statement->as_continue_statement()) {
        data.name = "continue";
        data.data = &EMPTY;
    } else {
        data.name = "";
        data.data = &EMPTY;
    }

    // Figure out main qubit register operands.
    auto insn = statement.as<ir::Instruction>();
    if (!insn.empty()) {
        for (const auto &oper : ir::get_operands(statement.as<ir::Instruction>())) {
            if (auto ref = oper->as_reference()) {
                if (
                    ref->target == context->ir->platform->qubits &&
                    ref->data_type == context->ir->platform->qubits->data_type &&
                    ref->indices.size() == 1 &&
                    ref->indices[0]->as_int_literal()
                ) {
                    data.qubits.push_back(ref->indices[0]->as_int_literal()->value);
                }
            }
        }
    }

    return this->gate(cycle, data, commit);
}

/**
 * Dumps a debug representation of the current resource state.
 */
void Base::dump_state(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    if (!initialized) {
        throw utils::Exception("resource dump_state() called before initialization");
    }
    on_dump_state(os, line_prefix);
}

} // namespace resource_types
} // namespace rmgr
} // namespace ql
