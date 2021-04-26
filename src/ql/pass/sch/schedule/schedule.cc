/** \file
 * Defines the scheduler pass.
 */

#include "ql/pass/sch/schedule/schedule.h"

#include "ql/utils/filesystem.h"
#include "ql/pmgr/pass_types/base.h"
#include "detail/scheduler.h"

namespace ql {
namespace pass {
namespace sch {
namespace schedule {

/**
 * Dumps docs for the scheduler.
 */
void SchedulePass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix, R"(
    This pass analyzes the data dependencies between gates and applies cycle
    numbers to them based on some scheduling heuristic. Depending on options,
    the scheduler will either be resource-constrained or will ignore resources.
    )");
}

/**
 * Returns a user-friendly type name for this pass.
 */
utils::Str SchedulePass::get_friendly_type() const {
    return "Scheduler";
}

/**
 * Constructs a scheduler.
 */
SchedulePass::SchedulePass(
    const utils::Ptr<const pmgr::Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::KernelTransformation(pass_factory, instance_name, type_name) {

    options.add_bool(
        "resource_constraints",
        "Whether to respect or ignore resource constraints when scheduling.",
        true
    );

    options.add_enum(
        "scheduler_target",
        "Which scheduling heuristic is to be used; ASAP schedules all gates as "
        "soon as possible, ALAP starts from the last gate and schedules all "
        "gates as late as possible, and uniform tries to smoothen out the "
        "amount of parallelism throughout each kernel. Uniform scheduling is "
        "only supported without resource constraints. ALAP is best for most "
        "simple quantum circuits, because the measurements at the end will be "
        "done in parallel if possible, and state initialization is postponed "
        "as much as possible to reduce state lifetime.",
        "alap",
        {"asap", "alap", "uniform"}
    );

    options.add_enum(
        "scheduler_heuristic",
        "This controls what scheduling heuristic should be used for ordering "
        "the list of available gates by criticality.",
        "path_length",
        {"path_length", "random"}
    );

    options.add_bool(
        "commute_multi_qubit",
        "Whether to consider commutation rules for the CZ and CNOT quantum "
        "gates.",
        false
    );

    options.add_bool(
        "commute_single_qubit",
        "Whether to consider commutation rules for single-qubit X and Z "
        "rotations.",
        false
    );

    options.add_bool(
        "write_dot_graphs",
        "Whether to emit a graphviz dot graph representation of the schedule "
        "of the kernel. The emitted file will use suffix `_<kernel>.dot`.",
        false
    );

}

/**
 * Runs the scheduler.
 */
utils::Int SchedulePass::run(
    const ir::ProgramRef &program,
    const ir::KernelRef &kernel,
    const pmgr::pass_types::Context &context
) const {

    // Construct the scheduling object.
    detail::Scheduler sched;
    sched.init(
        kernel,
        context.output_prefix,
        options["commute_multi_qubit"].as_bool(),
        options["commute_single_qubit"].as_bool(),
        options["scheduler_heuristic"].as_str() == "path_length"
    );

    // Run the appropriate scheduling algorithm.
    if (options["resource_constraints"].as_bool()) {
        auto rm = rmgr::Manager::from_defaults(kernel->platform);
        if (options["scheduler_target"].as_str() == "asap") {
            sched.schedule_asap(rm);
        } else if (options["scheduler_target"].as_str() == "alap") {
            sched.schedule_alap(rm);
        } else {
            utils::StrStrm ss;
            ss << context.full_pass_name << " is configured to use the ";
            ss << options["scheduler_target"].as_str() << " scheduling target, ";
            ss << "but is also configured to respect resource constraints, ";
            ss << "and this combination is not supported";
            throw utils::Exception(ss.str());
        }
    } else {
        if (options["scheduler_target"].as_str() == "asap") {
            sched.schedule_asap();
        } else if (options["scheduler_target"].as_str() == "alap") {
            sched.schedule_alap();
        } else if (options["scheduler_target"].as_str() == "uniform") {
            sched.schedule_alap_uniform();
        } else {
            utils::StrStrm ss;
            ss << "unimplemented scheduling target for " << context.full_pass_name;
            throw utils::Exception(ss.str());
        }
    }

    // Write dot file if requested.
    // TODO: maybe make this a separate pass, actually?
    if (options["write_dot_graphs"].as_bool()) {
        utils::OutFile outf{context.output_prefix + "_" + kernel->name + ".dot"};
        sched.get_dot(false, true, outf.unwrap());
    }

    return 0;
}

} // namespace schedule
} // namespace sch
} // namespace pass
} // namespace ql
