/** \file
 * Defines the scheduler pass.
 */

#include "ql/pass/sch/schedule/schedule.h"

#include "ql/utils/filesystem.h"
#include "ql/pmgr/pass_types.h"
#include "detail/scheduler.h"

namespace ql {
namespace pass {
namespace sch {
namespace schedule {

// schedule support for program.h::schedule()
static void schedule_kernel(
    ir::KernelRef &kernel,
    const plat::PlatformRef &platform
) {
    utils::Str scheduler = com::options::get("scheduler");
    utils::Str scheduler_uniform = com::options::get("scheduler_uniform");

    QL_IOUT(scheduler << " scheduling the quantum kernel '" << kernel->name << "'...");

    detail::Scheduler sched;
    sched.init(
        kernel,
        com::options::get("output_dir") + "/",
        com::options::get("scheduler_commute") == "yes",
        com::options::get("scheduler_commute_rotations") == "yes"
    );

    /*if (com::options::get("print_dot_graphs") == "yes") {
        // NOTE JvS: basically, this preschedules with ASAP to generate a dot
        //  file before scheduling... I don't see why you wouldn't be able to do
        //  this by just prefixing another scheduler pass, so I'm taking this
        //  one out.
        utils::Str dot;
        sched.get_dot(dot);
        auto fname = com::options::get("output_dir") + "/" + kernel->get_name() + "_dependence_graph.dot";
        QL_IOUT("writing scheduled dot to '" << fname << "' ...");
        utils::OutFile(fname).write(dot);
    }*/

    if (scheduler_uniform == "yes") {
        sched.schedule_alap_uniform(); // result in current kernel's circuit (k.c)
    } else if (scheduler == "ASAP") {
        sched.schedule_asap(); // result in current kernel's circuit (k.c)
    } else if (scheduler == "ALAP") {
        sched.schedule_alap(); // result in current kernel's circuit (k.c)
    } else {
        QL_FATAL("Not supported scheduler option: scheduler=" << scheduler);
    }

    if (com::options::get("print_dot_graphs") == "yes") {
        utils::Str scheduler_opt = com::options::get("scheduler");
        auto fname = com::options::get("output_dir") + "/" + kernel->get_name() + scheduler_opt + "_scheduled.dot";
        QL_IOUT("writing scheduled dot to '" << fname << "' ...");
        utils::OutFile outf{fname};
        sched.get_dot(false, true, outf.unwrap());
    }



    QL_DOUT(scheduler << " scheduling the quantum kernel '" << kernel->name << "' DONE");
}

/**
 * Main entry point of the non-resource-constrained scheduler.
 * FIXME JvS: remove; only used by old pass manager
 */
void schedule(
    const ir::ProgramRef &program,
    const plat::PlatformRef &platform,
    const utils::Str &passname
) {
    if (com::options::get("prescheduler") == "yes") {
        report_statistics(program, platform, "in", passname, "# ");
        report_qasm(program, platform, "in", passname);

        QL_IOUT("scheduling the quantum program");
        for (auto &k : program->kernels) {
            schedule_kernel(k, platform);
        }

        report_statistics(program, platform, "out", passname, "# ");
        report_qasm(program, platform, "out", passname);
    }
}

static void rcschedule_kernel(
    const ir::KernelRef &kernel,
    const plat::PlatformRef &platform,
    const utils::Str &passname
) {
    QL_IOUT("Resource constraint scheduling ...");

    detail::Scheduler sched;
    sched.init(
        kernel,
        com::options::get("output_dir") + "/",
        com::options::get("scheduler_commute") == "yes",
        com::options::get("scheduler_commute_rotations") == "yes"
    );

    utils::Str schedopt = com::options::get("scheduler");
    if (schedopt == "ASAP") {
        sched.schedule_asap(plat::resource::Manager::from_defaults(platform));
    } else if (schedopt == "ALAP") {
        sched.schedule_alap(plat::resource::Manager::from_defaults(platform));
    } else {
        QL_FATAL("Not supported scheduler option: scheduler=" << schedopt);
    }

    if (com::options::get("print_dot_graphs") == "yes") {
        utils::StrStrm fname;
        fname << com::options::get("output_dir") << "/" << kernel->name << "_" << passname << ".dot";
        QL_IOUT("writing " << passname << " dependency graph dot file to '" << fname.str() << "' ...");
        utils::OutFile outf{fname.str()};
        sched.get_dot(false, true, outf.unwrap());
    }

    QL_IOUT("Resource constraint scheduling [Done].");
}


/**
 * Main entry point of the resource-constrained scheduler.
 * FIXME JvS: remove; only used by old pass manager
 */
void rcschedule(
    const ir::ProgramRef &program,
    const plat::PlatformRef &platform,
    const utils::Str &passname
) {
    report_statistics(program, platform, "in", passname, "# ");
    report_qasm(program, platform, "in", passname);

    for (auto &kernel : program->kernels) {
        QL_IOUT("Scheduling kernel: " << kernel->name);
        if (!kernel->c.empty()) {
            rcschedule_kernel(kernel, platform, passname);
        }
    }

    report_statistics(program, platform, "out", passname, "# ");
    report_qasm(program, platform, "out", passname);
}

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
 * Constructs a scheduler.
 */
SchedulePass::SchedulePass(
    const utils::Ptr<const pmgr::PassFactory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::KernelTransformation(pass_factory, instance_name, type_name) {
    options.add_bool(
        "resource_constraints",
        "Whether to respect or ignore resource constraints when scheduling.",
        true
    );
    options.add_enum(
        "heuristic",
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
        "write_dot_graph",
        "Whether to emit a graphviz dot graph representation of the schedule "
        "of the kernel. The emitted file will use suffix \"_<kernel>.dot\".",
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
        options["commute_single_qubit"].as_bool()
    );

    // Run the appropriate scheduling algorithm.
    if (options["resource_constraints"].as_bool()) {
        auto rm = plat::resource::Manager::from_defaults(kernel->platform);
        if (options["heuristic"].as_str() == "asap") {
            sched.schedule_asap(rm);
        } else if (options["heuristic"].as_str() == "alap") {
            sched.schedule_alap(rm);
        } else {
            utils::StrStrm ss;
            ss << context.full_pass_name << " is configured to use the ";
            ss << options["heuristic"].as_str() << " scheduling heuristic, ";
            ss << "but is also configured to respect resource constraints, ";
            ss << "and this combination is not supported";
            throw utils::Exception(ss.str());
        }
    } else {
        if (options["heuristic"].as_str() == "asap") {
            sched.schedule_asap();
        } else if (options["heuristic"].as_str() == "alap") {
            sched.schedule_alap();
        } else if (options["heuristic"].as_str() == "uniform") {
            sched.schedule_alap_uniform();
        } else {
            utils::StrStrm ss;
            ss << "unimplemented scheduling heuristic for " << context.full_pass_name;
            throw utils::Exception(ss.str());
        }
    }

    // Write dot file if requested.
    // TODO: maybe make this a separate pass, actually?
    if (options["write_dot_graph"].as_bool()) {
        utils::OutFile outf{context.output_prefix + "_" + kernel->name + ".dot"};
        sched.get_dot(false, true, outf.unwrap());
    }

    return 0;
}

} // namespace schedule
} // namespace sch
} // namespace pass
} // namespace ql
