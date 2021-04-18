/** \file
 * OpenQL Passes.
 * 
 * @note Below passes should eventually be implemented into their own files.
 */

#include "passes.h"

#include <iostream>
#include <chrono>
#include "ql/utils/opt.h"
#include "report.h"
#include "ql/pass/opt/clifford/detail/clifford.h"
#include "ql/pass/sch/schedule/schedule.h"
#include "ql/pass/map/qubits/map/detail/mapper.h"
#include "ql/pass/io/cqasm/read.h"
#include "ql/pass/ana/statistics/report.h"
#include "ql/arch/cc/pass/gen/vq1asm/detail/backend.h"

namespace ql {

using namespace utils;

/**
 * @brief  Scheduler pass constructor
 * @param  Name of the scheduler pass
 */
AbstractPass::AbstractPass(const Str &name) {
    QL_DOUT("In AbstractPass::AbstractPass set name " << name << std::endl);
    setPassName(name);
    passOptions.add_bool("skip", "skip running the pass");
    passOptions.add_bool("write_report_files", "report compiler statistics");
    passOptions.add_bool("write_qasm_files", "write (un-)scheduled (with and without resource-constraint) qasm files");
    passOptions.add_bool("read_qasm_files", "read (un-)scheduled (with and without resource-constraint) qasm files");
    passOptions.add_str ("hwconfig", "path to the platform configuration file", "none");
    passOptions.add_int ("nqubits", "number of qubits used by the program", "100", 1);
    passOptions.add_enum("eqasm_compiler_name", "Set the compiler backend", "cc_light_compiler", {"cc_light_compiler", "eqasm_backend_cc"});
}

/**
 * @brief   Gets the name of the pass
 * @return  Name of the compiler pass
 */
Str AbstractPass::getPassName() const {
    return passName;
}

/**
 * @brief  Sets the name of the pass
 * @param  Name of the compiler pass
 */
void AbstractPass::setPassName(const Str &name) {
    passName = name;
}

/**
 * @brief   Sets a pass option
 * @param   optionName String option name
 * @param   optionValue String value of the option
 */
void AbstractPass::setPassOption(const Str &optionName, const Str &optionValue) {
    QL_DOUT("In AbstractPass::setPassOption");

    passOptions[optionName] = optionValue;
}

utils::Options &AbstractPass::getPassOptions() {
    return passOptions;
}

const utils::Options &AbstractPass::getPassOptions() const {
    return passOptions;
}

/**
 * @brief   Queries the skip option of the pass
 * @return  Bool representing wheather the pass should be skipped
 */
Bool AbstractPass::getSkip() const {
    return getPassOptions()["skip"].as_bool();
}

/**
 * @brief   Initializes the pass by printing useful information
 */
void AbstractPass::initPass(const ir::ProgramRef &program) {
    QL_DOUT("initPass of " << getPassName() << " on program " << program->name);
    if (getPassOptions()["write_qasm_files"].as_bool()) {
        //temporary store old value
        ///@note-rn: this is only needed to overwrite global option set for old program flow for compatibility reasons ==> This should be deprecated when we remove old code
        Str writeQasmLocal = com::options::get("write_qasm_files");
        com::options::set("write_qasm_files", "yes");

        QL_DOUT("initPass of " << getPassName() << " write_qasm_files option was yes for pass");
        report_qasm(program, program->platform, "in", getPassName());

        com::options::set("write_qasm_files", writeQasmLocal);
    }

    if (getPassOptions()["write_report_files"].as_bool()) {
        //temporary store old value
        ///@note-rn: this is only needed to overwrite global option set for old program flow for compatibility reasons ==> This should be deprecated when we remove old code
        Str writeReportLocal = com::options::get("write_report_files");
        com::options::set("write_report_files", "yes");

        QL_DOUT("initPass of " << getPassName() << " write_report_files option was yes for pass");
        report_statistics(program, program->platform, "in", getPassName(), "# ");

        com::options::set("write_report_files", writeReportLocal);
    }
}

/**
 * @brief   Finilazes the pass by printing useful information and cleaning
 */
void AbstractPass::finalizePass(const ir::ProgramRef &program) {
    QL_DOUT("finalizePass of " << getPassName() << " on program " << program->name);
    if (getPassOptions()["write_qasm_files"].as_bool()) {
        //temporary store old value
        ///@note-rn: this is only needed to overwrite global option set for old program flow for compatibility reasons ==> This should be deprecated when we remove old code
        Str writeQasmLocal = com::options::get("write_qasm_files");
        com::options::set("write_qasm_files", "yes");

        QL_DOUT("finalizePass of " << getPassName() << " write_qasm_files option was yes for pass");
        report_qasm(program, program->platform, "out", getPassName());

        com::options::set("write_qasm_files", writeQasmLocal);
    }

    if (getPassOptions()["write_report_files"].as_bool()) {
        //temporary store old value
        ///@note-rn: this is only needed to overwrite global option set for old program flow for compatibility reasons ==> This should be deprecated when we remove old code
        Str writeReportLocal = com::options::get("write_report_files");
        com::options::set("write_report_files", "yes");

        QL_DOUT("finalizePass of " << getPassName() << " write_report_files option was yes for pass");
        report_statistics(program, program->platform, "out", getPassName(), "# ", getPassStatistics());

        com::options::set("write_report_files", writeReportLocal);
    }

    resetStatistics();
}

/**
 * @brief   Modifies/Stores statistics about the pass
 */
void AbstractPass::appendStatistics(const Str &statistic) {
    statistics += statistic;
}

Str AbstractPass::getPassStatistics() const {
    return statistics;
}

void AbstractPass::resetStatistics() {
    statistics = "";
}

// schedule support for program.h::schedule()
static void schedule_kernel(
    ir::KernelRef &kernel,
    const plat::PlatformRef &platform
) {
    utils::Str scheduler = com::options::get("scheduler");
    utils::Str scheduler_uniform = com::options::get("scheduler_uniform");

    QL_IOUT(scheduler << " scheduling the quantum kernel '" << kernel->name << "'...");

    pass::sch::schedule::detail::Scheduler sched;
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
static void schedule(
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

/**
 * @brief  Reader pass constructor
 * @param  Name of the read pass
 */
CQasmReaderPass::CQasmReaderPass(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Apply the pass to the input program
 * @param  Program object to be read
 */
void CQasmReaderPass::runOnProgram(const ir::ProgramRef &program) {
    QL_DOUT("run ReaderPass with name = " << getPassName() << " on program " << program->name);

    auto reader = pass::io::cqasm::read::Reader(program->platform, program);

    QL_DOUT("!!!!!!!!!!! start reader !!!!!!!!");

    // reset kernels if they are not empty, needed for the case when the reader pass
    // is used after a Writer pass within the sequence os passes and not at the start
    // of the compiler when there is no IR
    program->kernels.reset();

    ///@todo-rn: come up with a parametrized naming scheme to do this printing. This should reflect
    // if the pass is outputing non- or scheduled qasm depending if it is used before or after sched
    // currently this works only when Writer pass creating the qasm file is called outputIR
    reader.file2circuit(com::options::get("output_dir")+"/"+program->name+"_outputIR_out.qasm");
}

/**
 * @brief  Writer pass constructor
 * @param  Name of the read pass
 */
CQasmWriterPass::CQasmWriterPass(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Apply the pass to the input program
 * @param  Program object to be write
 */
void CQasmWriterPass::runOnProgram(const ir::ProgramRef &program) {
    QL_DOUT("run WriterPass with name = " << getPassName() << " on program " << program->name);

    // report/write_qasm initialization
    //report_init(program, program->platform);

    // writer pass of the initial qasm file (program.qasm)
    write_qasm(program, program->platform, getPassName());

//     if (getPassName() != "initialqasmwriter" && getPassName() != "scheduledqasmwriter")
//     { ///@note-rn: temoporary hack to make the writer pass for those 2 configurations soft (i.e., do not delete the subcircuits) so that it does not require a reader pass after it!. This is needed until we fix the synchronization between hardware configuration files and openql tests. Until then a Reader pass would be needed after a hard Write pass. However, a Reader pass will make some unit tests to fail due to a mismatch between the instructions in the tests (i.e., prepz) and included/defined in the hardware config files CONFLICTING with the prepz instr not being available in libQASM.
}

/**
 * @brief  Scheduler pass constructor
 * @param  Name of the scheduler pass
 */
SchedulerPass::SchedulerPass(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Apply the pass to the input program
 * @param  Program object to be read
 */
void SchedulerPass::runOnProgram(const ir::ProgramRef &program) {
    QL_DOUT("run SchedulerPass with name = " << getPassName() << " on program " << program->name);

    // prescheduler pass
    schedule(program, program->platform, "prescheduler");
}

/**
 * @brief  Scheduler pass constructor
 * @param  Name of the scheduler pass
 */
BackendCompilerPass::BackendCompilerPass(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Apply the pass to the input program
 * @param  Program object to be read
 */
void BackendCompilerPass::runOnProgram(const ir::ProgramRef &program) {
    QL_DOUT("run BackendCompilerPass with name = " << getPassName() << " on program " << program->name);
    
    Str eqasm_compiler_name = program->platform->eqasm_compiler_name;
    //getPassOptions()->getOption("eqasm_compiler_name");
    
    if (eqasm_compiler_name == "eqasm_backend_cc") {

        // This was hardcoded in the CC backend still, taken out now.
        schedule(program, program->platform, "scheduler");

        // Parse options structure from global options.
        namespace detail = arch::cc::pass::gen::vq1asm::detail;
        auto options = utils::Ptr<detail::Options>::make();
        options->output_prefix = com::options::get("output_dir") + "/" + program->unique_name;
        options->map_input_file = com::options::get("backend_cc_map_input_file");
        options->run_once = com::options::get("backend_cc_run_once") == "yes";
        options->verbose = com::options::get("backend_cc_verbose") == "yes";

        // Run backend.
        detail::Backend().compile(program, options.as_const());

    } else {
        QL_FATAL("the '" << eqasm_compiler_name << "' eqasm compiler backend is not suported !");
    }
    
}

/**
 * @brief  Statistics pass constructor
 * @param  Name of the scheduler pass
 */
StatisticsReporterPass::StatisticsReporterPass(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Report Statistics for the input program
 * @param  Program object to be read
 */
void StatisticsReporterPass::runOnProgram(const ir::ProgramRef &program) {
    ///@note-rn: below call should be manually inlined here and removed from its current location
    ///@note-rn: pass should be moved to separate file containing only this pass
    report_statistics(program, program->platform, "todo-inout", getPassName(), "# ");
}

/**
 * @brief  Visualizer pass constructor
 * @param  Name of the visualizer pass
 */
VisualizerPass::VisualizerPass(const Str &name) : AbstractPass(name) {
    getPassOptions().add_enum("visualizer_type", "the type of visualization performed", "CIRCUIT", {"CIRCUIT", "MAPPING_GRAPH", "INTERACTION_GRAPH"});
    getPassOptions().add_str("visualizer_config_path", "path to the visualizer configuration file", "visualizer_config.json");
    getPassOptions().add_str("visualizer_waveform_mapping_path", "path to the visualizer waveform mapping file", "waveform_mapping.json");
}

/**
 * @brief  Visualize the quantum program
 * @param  Program object to be read
 */
void VisualizerPass::runOnProgram(const ir::ProgramRef &program) {
    QL_EOUT("the visualizer can no longer be run using this interface");
}

/**
 * @brief  Mapper pass constructor
 * @param  Name of the mapper pass
 */
MapperPass::MapperPass(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Maps the input program to the target platform
 * @param  Program object to be mapped
 */
void MapperPass::runOnProgram(const ir::ProgramRef &program) {

    namespace detail = pass::map::qubits::map::detail;

    auto platform = program->platform;
    auto passname = getPassName();

    auto mapopt = com::options::get("mapper");
    if (mapopt == "no") {
        QL_IOUT("Not mapping kernels");
        return;
    }

    report_statistics(program, platform, "in", passname, "# ");
    report_qasm(program, platform, "in", passname);

    // Build the options structure for the mapper.
    detail::Options parsed_options;

    parsed_options.output_prefix = com::options::get("output_dir") + "/";

    if (mapopt == "base") {
        parsed_options.heuristic = detail::Heuristic::BASE;
    } else if (mapopt == "baserc") {
        parsed_options.heuristic = detail::Heuristic::BASE_RC;
    } else if (mapopt == "minextend") {
        parsed_options.heuristic = detail::Heuristic::MIN_EXTEND;
    } else if (mapopt == "minextendrc") {
        parsed_options.heuristic = detail::Heuristic::MIN_EXTEND_RC;
    } else if (mapopt == "maxfidelity") {
        parsed_options.heuristic = detail::Heuristic::MAX_FIDELITY;
    } else {
        QL_ASSERT(false);
    }

    parsed_options.initialize_one_to_one = com::options::global["mapinitone2one"].as_bool();
    parsed_options.assume_initialized = com::options::global["mapassumezeroinitstate"].as_bool();
    parsed_options.assume_prep_only_initializes = com::options::global["mapprepinitsstate"].as_bool();

    auto lookahead_mode = com::options::global["maplookahead"].as_str();
    if (lookahead_mode == "no") {
        parsed_options.lookahead_mode = detail::LookaheadMode::DISABLED;
    } else if (lookahead_mode == "1qfirst") {
        parsed_options.lookahead_mode = detail::LookaheadMode::ONE_QUBIT_GATE_FIRST;
    } else if (lookahead_mode == "noroutingfirst") {
        parsed_options.lookahead_mode = detail::LookaheadMode::NO_ROUTING_FIRST;
    } else if (lookahead_mode == "all") {
        parsed_options.lookahead_mode = detail::LookaheadMode::ALL;
    } else {
        QL_ASSERT(false);
    }

    auto path_selection_mode = com::options::global["mappathselect"].as_str();
    if (path_selection_mode == "all") {
        parsed_options.path_selection_mode = detail::PathSelectionMode::ALL;
    } else if (path_selection_mode == "borders") {
        parsed_options.path_selection_mode = detail::PathSelectionMode::BORDERS;
    } else {
        QL_ASSERT(false);
    }

    auto swap_selection_mode = com::options::global["mapselectswaps"].as_str();
    if (swap_selection_mode == "one") {
        parsed_options.swap_selection_mode = detail::SwapSelectionMode::ONE;
    } else if (path_selection_mode == "all") {
        parsed_options.swap_selection_mode = detail::SwapSelectionMode::ALL;
    } else if (path_selection_mode == "earliest") {
        parsed_options.swap_selection_mode = detail::SwapSelectionMode::EARLIEST;
    } else {
        QL_ASSERT(false);
    }

    parsed_options.recurse_on_nn_two_qubit = com::options::global["maprecNN2q"].as_bool();

    if (com::options::global["mapselectmaxlevel"].as_str() == "inf") {
        parsed_options.recursion_depth_limit = utils::MAX;
    } else {
        parsed_options.recursion_depth_limit = com::options::global["mapselectmaxlevel"].as_uint();
    }

    parsed_options.recursion_width_factor = com::options::global["mapselectmaxwidth"].as_real();

    auto tie_break_method = com::options::global["maptiebreak"].as_str();
    if (tie_break_method == "first") {
        parsed_options.tie_break_method = detail::TieBreakMethod::FIRST;
    } else if (tie_break_method == "last") {
        parsed_options.tie_break_method = detail::TieBreakMethod::LAST;
    } else if (tie_break_method == "random") {
        parsed_options.tie_break_method = detail::TieBreakMethod::RANDOM;
    } else if (tie_break_method == "critical") {
        parsed_options.tie_break_method = detail::TieBreakMethod::CRITICAL;
    } else {
        QL_ASSERT(false);
    }

    auto use_moves = com::options::global["mapusemoves"].as_str();
    if (use_moves == "no") {
        parsed_options.use_move_gates = false;
    } else if (use_moves == "yes") {
        parsed_options.use_move_gates = true;
        parsed_options.max_move_penalty = 0;
    } else {
        parsed_options.use_move_gates = true;
        parsed_options.max_move_penalty = utils::parse_uint(use_moves);
    }

    parsed_options.reverse_swap_if_better = com::options::global["mapreverseswap"].as_bool();
    parsed_options.commute_multi_qubit = com::options::global["scheduler_commute"].as_bool();
    parsed_options.commute_single_qubit = com::options::global["scheduler_commute_rotations"].as_bool();
    parsed_options.write_dot_graphs = com::options::global["print_dot_graphs"].as_bool();
    parsed_options.enable_mip_placer = com::options::global["initialplace"].as_bool();
    parsed_options.mip_horizon = com::options::global["initialplace2qhorizon"].as_uint();

    detail::OptionsRef parsed_options_ref;
    parsed_options_ref.emplace(parsed_options);

    detail::Mapper mapper;  // virgin mapper creation; for role of Init functions, see comment at top of mapper.h
    mapper.map(program, parsed_options_ref); // platform specifies number of real qubits, i.e. locations for virtual qubits

    report_statistics(program, platform, "out", passname, "# ");
    report_qasm(program, platform, "out", passname);
}

/**
 * @brief  Clifford Optimize pass constructor
 * @param  Name of the optimizer pass (premapper or postmapper)
 */
CliffordOptimizerPass::CliffordOptimizerPass(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Clifford optimizer
 * @param  Program object to be clifford optimized
 */
void CliffordOptimizerPass::runOnProgram(const ir::ProgramRef &program) {
    auto passname = getPassName();
    auto platform = program->platform;

    if (com::options::get(passname) == "no") {
        QL_DOUT("Clifford optimization on program " << program->name << " at "
                                                    << passname << " not DONE");
        return;
    }
    QL_DOUT("Clifford optimization on program " << program->name << " at "
                                                << passname << " ...");

    report_statistics(program, platform, "in", passname, "# ");
    report_qasm(program, platform, "in", passname);

    pass::opt::clifford::optimize::detail::Clifford cliff;
    for (auto &kernel : program->kernels) {
        cliff.optimize_kernel(kernel);
    }

    report_statistics(program, platform, "out", passname, "# ");
    report_qasm(program, platform, "out", passname);
}

/**
 * @brief  Resource Constraint Scheduler pass constructor
 * @param  Name of the scheduler pass
 */
RCSchedulerPass::RCSchedulerPass(const Str &name) : AbstractPass(name) {
};

static void rcschedule_kernel(
    const ir::KernelRef &kernel,
    const plat::PlatformRef &platform,
    const utils::Str &passname
) {
    QL_IOUT("Resource constraint scheduling ...");

    pass::sch::schedule::detail::Scheduler sched;
    sched.init(
        kernel,
        com::options::get("output_dir") + "/",
        com::options::get("scheduler_commute") == "yes",
        com::options::get("scheduler_commute_rotations") == "yes"
    );

    utils::Str schedopt = com::options::get("scheduler");
    if (schedopt == "ASAP") {
        sched.schedule_asap(rmgr::Manager::from_defaults(platform));
    } else if (schedopt == "ALAP") {
        sched.schedule_alap(rmgr::Manager::from_defaults(platform));
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
 * @brief  Resource Constraint Scheduling of the input program
 * @param  Program object to be rcscheduled
 */
void RCSchedulerPass::runOnProgram(const ir::ProgramRef &program) {
    auto platform = program->platform;
    auto passname = getPassName();

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

} // namespace ql
