/** \file
 * Defines the list scheduler pass.
 */

#include "ql/pass/dec/instructions/instructions.h"

#include "ql/ir/old_to_new.h"
#include "ql/pmgr/pass_types/base.h"

namespace ql {
namespace pass {
namespace dec {
namespace instructions {

/**
 * Dumps docs for the instruction decomposer.
 */
void DecomposeInstructionsPass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix, R"(
    This pass (conditionally) applies instructions decomposition rules as
    specified in the platform configuration JSON structure. The pass returns the
    number of rules that were applied.

    Rules can be disabled for the purpose of this pass using the `predicate_key`
    and `predicate_value` options. When set, the key given by `predicate_key` is
    resolved in the JSON data that may be associated with new-style
    decomposition rules (the ones associated with instructions, rather than
    the ones specified in the `"gate_decomposition"` section of the platform
    JSON file). If this resolves to a string, the `predicate_value` option is
    matched against it. The rule is then only applied if there is a match. Some
    special cases:

     - if the key does not exist in the JSON data associated with the
       decomposition rule, or if it exists but maps to something that isn't a
       string, the predicate will match if `predicate_value` matches an empty
       string; and
     - the effective JSON structure for legacy decomposition rules is
       `{"name": "legacy"}`.

    The `ignore_schedule` option controls how scheduling information is treated.
    When set to yes (the default), the cycle numbers of the decomposed
    instructions will be set to the same cycle number as the original
    instruction. When set to no, the schedule of the decomposed instructions is
    taken from the decomposition rule, and instructions are reordered
    accordingly after all decompositions have taken place.
)" R"(
    For example, assume that we have the following decomposition rule for a
    CNOT gate:

        ym90 op(1)
        cz op(0), op(1)
        skip 1
        y90 op(1)

    and that we have the following program as input:

        {
            cnot q[0], q[1]
            cnot q[1], q[2]
        }

    Now, if `ignore_schedule` is enabled, the resulting program would be

        {
            ym90 q[1]
            cz q[0], q[1]
            y90 q[1]
            ym90 q[2]
            cz q[1], q[2]
            y90 q[2]
        }

    The schedule is obviously invalid, because qubits are being used by multiple
    gates in the same cycle. But so was the input. Nevertheless, the order of
    the instructions is what we wanted; after scheduling, the program will be
    correct.

    If we were to turn `ignore_schedule` off, however, this is what we'd get:

        {
            ym90 q[1]
            ym90 q[2]
        }
        {
            cz q[0], q[1]
            cz q[1], q[2]
        }
        skip 1
        {
            y90 q[1]
            y90 q[2]
        }
)" R"(
    Which is wrong! The `ym90` and `y90` gates execute out of order with the
    `cz q[1], q[2]` now. Scheduling won't fix this.

    The key takeaway here is that you should leave `ignore_schedule` enabled if
    A) the program has not been scheduled yet or B) you're not sure that the
    schedules in the decomposition rules are actually defined correctly.

    Of course, there are cases where `ignore_schedule` needs to be disabled,
    otherwise the option wouldn't need to be there. It's useful specifically
    when you need to process code expansions *after* scheduling. You will need
    to make sure that the decomposition rules that the predicate matches are
    written such that they won't ever break a correctly scheduled program, but
    if that's the case, you won't have to schedule the program again after the
    decomposition. For example, if the input program had been

        cnot q[0], q[1]
        skip 3
        cnot q[1], q[2]

    the result with `ignore_schedule` disabled would have been

        ym90 q[1]
        cz q[0], q[1]
        skip 1
        y90 q[1]
        ym90 q[2]
        cz q[1], q[2]
        skip 1
        y90 q[2]

    which is not an optimal schedule by any means, but a correct one
    nonetheless. A more reasonable use case for this than CNOT to CZ
    decomposition would be expanding a CZ gate to single-qubit flux and parking
    gates; it's vital that these gates will not be shifted around with respect
    to each other, which scheduling after decomposing them might do.
    )");
}

/**
 * Returns a user-friendly type name for this pass.
 */
utils::Str DecomposeInstructionsPass::get_friendly_type() const {
    return "Instruction decomposer";
}

/**
 * Constructs an instruction decomposer.
 */
DecomposeInstructionsPass::DecomposeInstructionsPass(
    const utils::Ptr<const pmgr::Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::Transformation(pass_factory, instance_name, type_name) {

    options.add_str(
        "predicate_key",
        "The key to use for the predicate check.",
        "name"
    );

    options.add_str(
        "predicate_value",
        "Pattern that must match for the value of the key specified by the "
        "`predicate_key` option for a decomposition rule to be applied. `*` "
        "and `?` may be used to construct nontrivial patterns. The entire "
        "pattern must match; for partial matches, prefix and append an `*`."
        "Nonexistent keys or non-string values are treated as if they are an "
        "empty string.",
        "*"
    );

    options.add_bool(
        "ignore_schedule",
        "When set, the schedule of the decomposition expansions is ignored. "
        "This prevents instructions from ever needing to be reordered, and "
        "thus prevents the behavior of the program from changing due to "
        "incorrect schedules in the decomposition rules, but will almost "
        "certainly require the program to be rescheduled. You should only "
        "turn this off when you really want to keep scheduling information, "
        "and are really sure that the schedules in the decomposition rule "
        "expansions are correct.",
        "yes"
    );

}

/**
 * Runs the instruction decomposer on the given block.
 */
utils::UInt DecomposeInstructionsPass::run_on_block(
    const ir::Ref &ir,
    const ir::BlockBaseRef &block,
    utils::Bool ignore_schedule,
    const com::dec::RulePredicate &predicate
) {

    // Apply the decomposition rules.
    auto number_of_applications = com::dec::apply_decomposition_rules(
        ir, block, ignore_schedule, predicate
    );

    // Remove the KernelCyclesValid annotation if we broke the schedule for
    // sure.
    if (number_of_applications && ignore_schedule) {
        block->erase_annotation<ir::KernelCyclesValid>();
    }

    // Recurse into structured control-flow sub-blocks.
    for (const auto &statement : block->statements) {
        if (auto if_else = statement->as_if_else()) {
            for (const auto &branch : if_else->branches) {
                number_of_applications += run_on_block(ir, branch->body, ignore_schedule, predicate);
            }
            if (!if_else->otherwise.empty()) {
                number_of_applications += run_on_block(ir, if_else->otherwise, ignore_schedule, predicate);
            }
        } else if (auto loop = statement->as_loop()) {
            number_of_applications += run_on_block(ir, loop->body, ignore_schedule, predicate);
        }
    }

    return number_of_applications;
}

/**
 * Runs the instruction decomposer.
 */
utils::Int DecomposeInstructionsPass::run(
    const ir::Ref &ir,
    const pmgr::pass_types::Context &context
) const {

    // Parse options.
    auto ignore_schedule = options["ignore_schedule"].as_bool();
    auto predicate_key = options["predicate_key"].as_str();
    auto predicate_value = options["predicate_value"].as_str();

    // Construct the predicate function.
    auto predicate = [predicate_key, predicate_value](const ir::DecompositionRef &rule) {
        utils::Str value;
        if (predicate_key == "name") {
            value = rule->name;
        } else {
            auto it = rule->data->find(predicate_key);
            if (it != rule->data->end() && it->is_string()) {
                value = it->get<utils::Str>();
            }
        }
        return utils::pattern_match(predicate_value, value);
    };

    // Process the decomposition rules for the whole program.
    utils::UInt number_of_applications = 0;
    if (!ir->program.empty()) {
        for (const auto &block : ir->program->blocks) {
            try {
                number_of_applications += run_on_block(ir, block, ignore_schedule, predicate);
            } catch (utils::Exception &e) {
                e.add_context("in block " + block->name);
                throw;
            }
        }
    }
    return (utils::Int)number_of_applications;
}

} // namespace instructions
} // namespace dec
} // namespace pass
} // namespace ql
