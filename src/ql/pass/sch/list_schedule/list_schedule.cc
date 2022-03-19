/** \file
 * Defines the list scheduler pass.
 */

#include "ql/pass/sch/list_schedule/list_schedule.h"

#include "ql/utils/filesystem.h"
#include "ql/ir/old_to_new.h"
#include "ql/com/ddg/build.h"
#include "ql/com/ddg/ops.h"
#include "ql/com/ddg/dot.h"
#include "ql/com/sch/scheduler.h"
#include "ql/pmgr/pass_types/base.h"

// #define MULTI_LINE_LOG_DEBUG to enable multi-line dumping 
#undef MULTI_LINE_LOG_DEBUG

namespace ql {
namespace pass {
namespace sch {
namespace list_schedule {

/**
 * Dumps docs for the scheduler.
 */
void ListSchedulePass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix, R"(
    This pass analyzes the data dependencies between statements and applies
    quantum cycle numbers to them using optionally resource-constrained ASAP or
    ALAP list scheduling. All blocks in the program are scheduled independently.
    )");
}

/**
 * Returns a user-friendly type name for this pass.
 */
utils::Str ListSchedulePass::get_friendly_type() const {
    return "List scheduler";
}

/**
 * Constructs a scheduler.
 */
ListSchedulePass::ListSchedulePass(
    const utils::Ptr<const pmgr::Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::Transformation(pass_factory, instance_name, type_name) {

    options.add_bool(
        "resource_constraints",
        "Whether to respect or ignore resource constraints when scheduling.",
        true
    );

    options.add_enum(
        "scheduler_target",
        "Which scheduling target is to be used; ASAP schedules all statements "
        "as soon as possible, while ALAP starts from the last statement and "
        "schedules all statements as late as possible. ALAP is best for most "
        "simple quantum circuits, because the measurements at the end will be "
        "done in parallel if possible, and state initialization is postponed "
        "as much as possible to reduce state lifetime.",
        "alap",
        {"asap", "alap"}
    );

    options.add_enum(
        "scheduler_heuristic",
        "This controls what heuristic is used to sort the list of statements "
        "available for scheduling. `none` effectively disables sorting; the "
        "available statements will be scheduled in the order in which they "
        "were specified in the original program. `critical_path` schedules "
        "the statement with the longest critical path first. `deep_criticality` "
        "is the same except for statements with equal critical path length; in "
        "this case, the deep-criticality of the most critical successor "
        "is recursively checked instead.",
        "deep_criticality",
        {"none", "critical_path", "deep_criticality"}
    );

    options.add_bool(
        "commute_multi_qubit",
        "Whether to consider commutation rules for multi-qubit gates.",
        false
    );

    options.add_bool(
        "commute_single_qubit",
        "Whether to consider commutation rules for single-qubit gates.",
        false
    );

    options.add_int(
        "max_resource_block_cycles",
        "The maximum number of cycles to wait for the resource constraints to "
        "unblock a statement when there is nothing else to do. This is used for "
        "deadlock detection. It should just be set to a high number, or can be "
        "set to 0 to disable deadlock detection (but then the scheduler might "
        "end up in an infinite loop).",
        "10000",
        0
    );

    options.add_bool(
        "write_dot_graphs",
        "Whether to emit a graphviz dot graph representation of the data "
        "dependency graph and schedule of each block. The emitted files will "
        "use suffix `_<block-name>.dot`, where `<block-name>` is a uniquified "
        "name for each block.",
        false
    );

}

/**
 * Runs the scheduler on the given block.
 */
void ListSchedulePass::run_on_block(
    const ir::Ref &ir,
    const ir::BlockBaseRef &block,
    const utils::Str &name_path,
    utils::Set<utils::Str> &used_names,
    const pmgr::pass_types::Context &context
) {

    // Figure out a unique name for this block.
    utils::Str name = name_path;
    if (!used_names.insert(name).second) {
        utils::UInt i = 1;
        do {
            name = name_path + "_" + utils::to_string(i++);
        } while (!used_names.insert(name).second);
    }

    // Build a data dependency graph for the block.
    com::ddg::build(
        ir,
        block,
        context.options["commute_multi_qubit"].as_bool(),
        context.options["commute_single_qubit"].as_bool()
    );

    // Reverse the DDG if backward/ALAP scheduling is desired.
    auto reversed = context.options["scheduler_target"].as_str() == "alap";
    if (reversed) {
        com::ddg::reverse(block);
    }

    // Pre-schedule in the reverse direction for critical-path-length-based
    // heuristics.
    auto heuristic = context.options["scheduler_heuristic"].as_str();
    if (
        heuristic == "critical_path" ||
        heuristic == "deep_criticality"
    ) {

        // Criticality for ASAP list scheduling is computed via ALAP
        // pre-scheduling and vice-versa. So we need to reverse the direction of
        // the DDG to reverse the scheduling direction prior to prescheduling.
        com::ddg::reverse(block);

        // Perform prescheduling.
        QL_DOUT("prescheduling to determine criticality for " << name << "...");
        com::sch::Scheduler<>(block).run();
        QL_DOUT("prescheduling complete for " << name);

        // Reverse the DDG again so we don't clobber its direction.
        com::ddg::reverse(block);

    }

    // Perform the actual scheduling operation.
    QL_DOUT("scheduling " << name << "...");
    rmgr::CRef manager;
    if (context.options["resource_constraints"].as_bool()) {
        manager = *ir->platform->resources;
    }
    if (heuristic == "none") {
        com::sch::Scheduler<com::sch::TrivialHeuristic> scheduler(block, manager);
        scheduler.run(context.options["max_resource_block_cycles"].as_int());
        scheduler.convert_cycles();
    } else if (heuristic == "critical_path") {
        com::sch::Scheduler<com::sch::CriticalPathHeuristic> scheduler(block, manager);
        scheduler.run(context.options["max_resource_block_cycles"].as_int());
        scheduler.convert_cycles();
    } else if (heuristic == "deep_criticality") {
        QL_DOUT("computing deep criticality:");
        com::sch::DeepCriticality::compute(block);
        for (const auto &statement : block->statements) {
            QL_DOUT(
                "  n" << utils::abs(com::ddg::get_node(statement)->order) <<
                " -> " << com::sch::DeepCriticality::get(statement)
            );
        }
        com::sch::Scheduler<com::sch::DeepCriticality::Heuristic> scheduler(block, manager);
        scheduler.run(context.options["max_resource_block_cycles"].as_int());
        scheduler.convert_cycles();
        com::sch::DeepCriticality::clear(block);
    } else {
        QL_ICE("unknown heuristic " << heuristic);
    }
    QL_DOUT("scheduling complete for " << name);

    // Reverse the DDG back to forward direction if needed, since that makes
    // it much more readable.
    if (reversed && (QL_IS_LOG_DEBUG || context.options["write_dot_graphs"].as_bool())) {
        com::ddg::reverse(block);
        reversed = false;
    }

    // Always dump dot for the schedule if we're debugging.
#ifdef MULTI_LINE_LOG_DEBUG
    QL_IF_LOG_DEBUG {
        QL_DOUT("dumping dot file...");
        com::ddg::dump_dot(block);
    }
#else
    QL_DOUT("dumping dot file (disabled)");
#endif

    // Write the schedule as a dot file if requested.
    if (context.options["write_dot_graphs"].as_bool()) {
        auto filename = context.output_prefix + "_" + name + ".dot";
        QL_DOUT("writing dot output to " << filename);
        com::ddg::dump_dot(block, utils::OutFile(filename).unwrap());
    }

    // Clean up the DDG.
    com::ddg::clear(block);

    // Attach the KernelCyclesValid annotation to set the cycles_valid flag of
    // the corresponding kernel when new-to-old conversion is applied.
    block->set_annotation<ir::KernelCyclesValid>({true});

    // Recurse into structured control-flow sub-blocks.
    for (const auto &statement : block->statements) {
        if (auto if_else = statement->as_if_else()) {
            for (const auto &branch : if_else->branches) {
                run_on_block(ir, branch->body, name + "_if", used_names, context);
            }
            if (!if_else->otherwise.empty()) {
                run_on_block(ir, if_else->otherwise, name + "_else", used_names, context);
            }
        } else if (auto loop = statement->as_loop()) {
            run_on_block(ir, loop->body, name + "_loop", used_names, context);
        }
    }

}

/**
 * Runs the scheduler.
 */
utils::Int ListSchedulePass::run(
    const ir::Ref &ir,
    const pmgr::pass_types::Context &context
) const {
    utils::Set<utils::Str> used_names;
    if (!ir->program.empty()) {
        for (const auto &block : ir->program->blocks) {
            run_on_block(ir, block, block->name, used_names, context);
        }
    }
    return 0;
}

} // namespace list_schedule
} // namespace sch
} // namespace pass
} // namespace ql
