/** \file
 * OpenQL Passes.
 * 
 * @note Below passes should eventually be implemented into their own files.
 */

#include "passes.h"

#include <iostream>
#include "utils/opt.h"
#include "report.h"
#include "optimizer.h"
#include "clifford.h"
#include "decompose_toffoli.h"
#include "cqasm/cqasm_reader.h"
#include "latency_compensation.h"
#include "buffer_insertion.h"
#include "scheduler.h"
#include "visualizer.h"

#include "arch/cc_light/cc_light_eqasm_compiler.h"
#include "arch/cc/eqasm_backend_cc.h"

namespace ql {

using namespace utils;

/**
 * @brief  Scheduler pass constructor
 * @param  Name of the scheduler pass
 */
AbstractPass::AbstractPass(const Str &name) {
    QL_DOUT("In AbstractPass::AbstractPass set name " << name << std::endl);
    setPassName(name);
    createPassOptions();
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

    passOptions->setOption(optionName, optionValue);
}

/**
 * @brief   Initializes the pass options object
 */
void AbstractPass::createPassOptions() {
    passOptions = new PassOptions(getPassName());
}

PassOptions *AbstractPass::getPassOptions() {
    return passOptions;
}

const PassOptions *AbstractPass::getPassOptions() const {
    return passOptions;
}

/**
 * @brief   Queries the skip option of the pass
 * @return  Bool representing wheather the pass should be skipped
 */
Bool AbstractPass::getSkip() const {
    return getPassOptions()->getOption("skip") == "yes";
}

/**
 * @brief   Initializes the pass by printing useful information
 */
void AbstractPass::initPass(quantum_program *program) {
    if (getPassOptions()->getOption("write_qasm_files") == "yes") {
        //temporary store old value
        ///@note-rn: this is only needed to overwrite global option set for old program flow for compatibility reasons ==> This should be deprecated when we remove old code
        Str writeQasmLocal = options::get("write_qasm_files");
        options::set("write_qasm_files", "yes");

        report_qasm(program, program->platform, "in", getPassName());

        options::set("write_qasm_files", writeQasmLocal);
    }

    if (getPassOptions()->getOption("write_report_files") == "yes") {
        //temporary store old value
        ///@note-rn: this is only needed to overwrite global option set for old program flow for compatibility reasons ==> This should be deprecated when we remove old code
        Str writeReportLocal = options::get("write_report_files");
        options::set("write_report_files", "yes");

        report_statistics(program, program->platform, "in", getPassName(), "# ");

        options::set("write_report_files", writeReportLocal);
    }
}

/**
 * @brief   Finilazes the pass by printing useful information and cleaning
 */
void AbstractPass::finalizePass(quantum_program *program) {
    if (getPassOptions()->getOption("write_qasm_files") == "yes") {
        //temporary store old value
        ///@note-rn: this is only needed to overwrite global option set for old program flow for compatibility reasons ==> This should be deprecated when we remove old code
        Str writeQasmLocal = options::get("write_qasm_files");
        options::set("write_qasm_files", "yes");

        report_qasm(program, program->platform, "out", getPassName());

        options::set("write_qasm_files", writeQasmLocal);
    }

    if (getPassOptions()->getOption("write_report_files") == "yes") {
        //temporary store old value
        ///@note-rn: this is only needed to overwrite global option set for old program flow for compatibility reasons ==> This should be deprecated when we remove old code
        Str writeReportLocal = options::get("write_report_files");
        options::set("write_report_files", "yes");

        report_statistics(program, program->platform, "out", getPassName(), "# ", getPassStatistics());

        options::set("write_report_files", writeReportLocal);
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

/**
 * @brief  Reader pass constructor
 * @param  Name of the read pass
 */
ReaderPass::ReaderPass(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Apply the pass to the input program
 * @param  Program object to be read
 */
void ReaderPass::runOnProgram(quantum_program *program) {
    QL_DOUT("run ReaderPass with name = " << getPassName() << " on program " << program->name);

    cqasm_reader* reader = new cqasm_reader(program->platform, *program);

    QL_DOUT("!!!!!!!!!!! start reader !!!!!!!!");

    // reset kernels if they are not empty, needed for the case when the reader pass
    // is used after a Writer pass within the sequence os passes and not at the start
    // of the compiler when there is no IR
    auto it = program->kernels.begin();

    for (; it != program->kernels.end(); it++) {
        quantum_kernel ktmp = (*it);//->kernels.erase(*it);
        std::cout << ktmp.name << std::endl;
        ktmp.c.clear();
    }

    program->kernels.clear();

    ///@todo-rn: come up with a parametrized naming scheme to do this printing. This should reflect
    // if the pass is outputing non- or scheduled qasm depending if it is used before or after sched
    // currently this works only when Writer pass creating the qasm file is called outputIR
    reader->file2circuit(options::get("output_dir")+"/"+program->name+"_outputIR_out.qasm");
}

/**
 * @brief  Writer pass constructor
 * @param  Name of the read pass
 */
WriterPass::WriterPass(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Apply the pass to the input program
 * @param  Program object to be write
 */
void WriterPass::runOnProgram(quantum_program *program) {
    QL_DOUT("run WriterPass with name = " << getPassName() << " on program " << program->name);

    // report/write_qasm initialization
    //report_init(program, program->platform);

    // writer pass of the initial qasm file (program.qasm)
    write_qasm(program, program->platform, getPassName());

//     if (getPassName() != "initialqasmwriter" && getPassName() != "scheduledqasmwriter")
//     { ///@note-rn: temoporary hack to make the writer pass for those 2 configurations soft (i.e., do not delete the subcircuits) so that it does not require a reader pass after it!. This is needed until we fix the synchronization between hardware configuration files and openql tests. Until then a Reader pass would be needed after a hard Write pass. However, a Reader pass will make some unit tests to fail due to a mismatch between the instructions in the tests (i.e., prepz) and included/defined in the hardware config files CONFLICTING with the prepz instr not being available in libQASM.
}

/**
 * @brief  Rotation optimizer pass constructor
 * @param  Name of the optimized pass
 */
RotationOptimizerPass::RotationOptimizerPass(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Apply the pass to the input program
 * @param  Program object to be read
 */
void RotationOptimizerPass::runOnProgram(quantum_program *program) {
    QL_DOUT("run RotationOptimizerPass with name = " << getPassName() << " on program " << program->name);

    rotation_optimize(program, program->platform, "rotation_optimize");
}

/**
 * @brief  Rotation optimizer pass constructor
 * @param  Name of the optimized pass
 */
DecomposeToffoliPass::DecomposeToffoliPass(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Apply the pass to the input program
 * @param  Program object to be read
 */
void DecomposeToffoliPass::runOnProgram(quantum_program *program) {
    QL_DOUT("run DecomposeToffoliPass with name = " << getPassName() << " on program " << program->name);

    // decompose_toffoli pass
    decompose_toffoli(program, program->platform, "decompose_toffoli");
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
void SchedulerPass::runOnProgram(quantum_program *program) {
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
void BackendCompilerPass::runOnProgram(quantum_program *program) {
    QL_DOUT("run BackendCompilerPass with name = " << getPassName() << " on program " << program->name);

    Opt<eqasm_compiler> backend_compiler;

    Str eqasm_compiler_name = program->platform.eqasm_compiler_name;
    //getPassOptions()->getOption("eqasm_compiler_name");

    if (eqasm_compiler_name == "cc_light_compiler") {
        backend_compiler.emplace<arch::cc_light_eqasm_compiler>();
    } else if (eqasm_compiler_name == "eqasm_backend_cc") {
        backend_compiler.emplace<eqasm_backend_cc>();
    } else {
        QL_FATAL("the '" << eqasm_compiler_name << "' eqasm compiler backend is not suported !");
    }

    ///@todo-rn: Decide how to construct backend:
    // 1) we can run backend as one big composite engine, e.g.,
    //assert(backend_compiler);
    backend_compiler->compile(program, program->platform); //called here
    // OR
    // 2) in the user program add one for one individual backed passes.

    backend_compiler.reset();
}

/**
 * @brief  Statistics pass constructor
 * @param  Name of the scheduler pass
 */
ReportStatisticsPass::ReportStatisticsPass(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Report Statistics for the input program
 * @param  Program object to be read
 */
void ReportStatisticsPass::runOnProgram(quantum_program *program) {
    ///@note-rn: below call should be manually inlined here and removed from its current location
    ///@note-rn: pass should be moved to separate file containing only this pass
    report_statistics(program, program->platform, "todo-inout", getPassName(), "# ");
}

/**
 * @brief  Visualizer pass constructor
 * @param  Name of the visualizer pass
 */
VisualizerPass::VisualizerPass(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Visualize the quantum program
 * @param  Program object to be read
 */
void VisualizerPass::runOnProgram(quantum_program *program) {
    QL_DOUT("run VisualizerPass with name = " << getPassName() << " on program " << program->name);
    
    ql::visualize(program, getPassOptions()->getOption("visualizer_type"), {
        getPassOptions()->getOption("visualizer_config_path"),
        getPassOptions()->getOption("visualizer_waveform_mapping_path")
    });
}

/**
 * @brief  CCL Preparation for Code Generation pass constructor
 * @param  Name of the preparation pass
 */
CCLPrepCodeGeneration::CCLPrepCodeGeneration(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Prepare the program for code generation
 * @param  Program object to be prepared
 */
void CCLPrepCodeGeneration::runOnProgram(quantum_program *program) {
    const Json &instruction_settings = program->platform.instruction_settings;
    for (const Json &i : instruction_settings) {
       if (i.count("cc_light_instr") <= 0) {
            QL_FATAL("cc_light_instr not found for " << i);
       }
    }
}

/**
 * @brief  CCL Decompose PreSchedule pass constructor
 * @param  Name of the decomposer pass
 */
CCLDecomposePreSchedule::CCLDecomposePreSchedule(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Decompose the input program before scheduling
 * @param  Program object to be decomposed
 */
void CCLDecomposePreSchedule::runOnProgram(quantum_program *program) {
    arch::cc_light_eqasm_compiler().ccl_decompose_pre_schedule(program, program->platform, getPassName());
}

/**
 * @brief  Mapper pass constructor
 * @param  Name of the mapper pass
 */
MapPass::MapPass(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Maps the input program to the target platform
 * @param  Program object to be mapped
 */
void MapPass::runOnProgram(quantum_program *program) {
    Str stats;
    arch::cc_light_eqasm_compiler::map(program, program->platform, getPassName(), &stats);
    appendStatistics(stats);
}

/**
 * @brief  Clifford Optimize pass constructor
 * @param  Name of the optimizer pass (premapper or postmapper)
 */
CliffordOptimizePass::CliffordOptimizePass(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Clifford optimizer
 * @param  Program object to be clifford optimized
 */
void CliffordOptimizePass::runOnProgram(quantum_program *program) {
    clifford_optimize(program, program->platform, getPassName());
}

/**
 * @brief  Resource Constraint Scheduler pass constructor
 * @param  Name of the scheduler pass
 */
RCSchedulePass::RCSchedulePass(const Str &name) : AbstractPass(name) {
};

/**
 * @brief  Resource Constraint Scheduling of the input program
 * @param  Program object to be rcscheduled
 */
void RCSchedulePass::runOnProgram(quantum_program *program) {
    rcschedule(program, program->platform, getPassName());
}

/**
 * @brief  Latency compensation pass constructor
 * @param  Name of the latency compensation pass
 */
LatencyCompensationPass::LatencyCompensationPass(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Apply Latency Compensation to the scheduled program
 * @param  Program object to be latency compensated
 */
void LatencyCompensationPass::runOnProgram(quantum_program *program) {
    latency_compensation(program, program->platform, getPassName());
}

/**
 * @brief  Insert Buffer Delays pass  constructor
 * @param  Name of the buffer delay insertion pass
 */
InsertBufferDelaysPass::InsertBufferDelaysPass(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Insert Buffer Delays to the input program
 * @param  Program object to be extended with buffer delays
 */
void InsertBufferDelaysPass::runOnProgram(quantum_program *program) {
    insert_buffer_delays(program, program->platform, getPassName());
}

/**
 * @brief  Decomposer Post Schedule  Pass
 * @param  Name of the decomposer pass
 */
CCLDecomposePostSchedulePass::CCLDecomposePostSchedulePass(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  CC-Light specific Decomposition of the scheduled program
 * @param  Program object to be postscheduler decomposed
 */
void CCLDecomposePostSchedulePass::runOnProgram(quantum_program *program) {
    arch::cc_light_eqasm_compiler().ccl_decompose_post_schedule(program, program->platform, getPassName());
}

/**
 * @brief  QuantumSim Writer Pass constructor
 * @param  Name of the writer pass
 */
WriteQuantumSimPass::WriteQuantumSimPass(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Generate QuantumSim output
 * @param  Program object to be simulated using quantumsim
 */
void WriteQuantumSimPass::runOnProgram(quantum_program *program) {
    arch::cc_light_eqasm_compiler().write_quantumsim_script(program, program->platform, getPassName());
}

/**
 * @brief  QISA generation pass constructor
 * @param  Name of the QISA generator pass
 */
QisaCodeGenerationPass::QisaCodeGenerationPass(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Generate the QISA output from the input program
 * @param  Program object to be transformed into QISA output
 */
void QisaCodeGenerationPass::runOnProgram(quantum_program *program) {
    arch::cc_light_eqasm_compiler().qisa_code_generation(program, program->platform, getPassName());
}

/**
 * @brief  Construct an object to hold the pass options
 * @param  app_name String with the name of the pass options object
 */
PassOptions::PassOptions(Str app_name) {
    app = new CLI::App(app_name);

    ///@todo-rn: update this list with meaningful pass options
    // default values
    opt_name2opt_val.set("skip") = "no";
    opt_name2opt_val.set("write_report_files") = "no";
    opt_name2opt_val.set("write_qasm_files") = "no";
    opt_name2opt_val.set("read_qasm_files") = "no";
    opt_name2opt_val.set("hwconfig") = "none";
    opt_name2opt_val.set("nqubits") = "100";
    opt_name2opt_val.set("eqasm_compiler_name") = "cc_light_compiler";
    opt_name2opt_val.set("visualizer_type") = "CIRCUIT";
    opt_name2opt_val.set("visualizer_config_path") = "visualizer_config.json";
    opt_name2opt_val.set("visualizer_waveform_mapping_path") = "waveform_mapping.json";

    // add options with default values and list of possible values
    app->add_set_ignore_case("--skip", opt_name2opt_val.at("skip"), {"yes", "no"}, "skip running the pass", true);
    app->add_set_ignore_case("--write_report_files", opt_name2opt_val.at("write_report_files"), {"yes", "no"}, "report compiler statistics", true);
    app->add_set_ignore_case("--write_qasm_files", opt_name2opt_val.at("write_qasm_files"), {"yes", "no"}, "write (un-)scheduled (with and without resource-constraint) qasm files", true);
    app->add_set_ignore_case("--read_qasm_files", opt_name2opt_val.at("read_qasm_files"), {"yes", "no"}, "read (un-)scheduled (with and without resource-constraint) qasm files", true);
    app->add_option("--hwconfig", opt_name2opt_val.at("hwconfig"), "path to the platform configuration file", true);
    app->add_option("--nqubits", opt_name2opt_val.at("nqubits"), "number of qubits used by the program", true);
    app->add_set_ignore_case("--eqasm_compiler_name", opt_name2opt_val.at("eqasm_compiler_name"), {"cc_light_compiler", "eqasm_backend_cc"}, "Set the compiler backend", true);
    app->add_option("--visualizer_type", opt_name2opt_val.at("visualizer_type"), "the type of visualization performed", true);
    app->add_option("--visualizer_config_path", opt_name2opt_val.at("visualizer_config_path"), "path to the visualizer configuration file", true);
    app->add_option("--visualizer_waveform_mapping_path", opt_name2opt_val.at("visualizer_waveform_mapping_path"), "path to the visualizer waveform mapping file", true);
}

/**
 * @brief  Show the values set for the pass options.
 */
void PassOptions::print_current_values() const {
    ///@todo-rn: update this list with meaningful pass options
    std::cout << "write_qasm_files: " << opt_name2opt_val.at("write_qasm_files") << std::endl
              << "write_report_files: " << opt_name2opt_val.at("write_report_files") << std::endl
              << "skip: " << opt_name2opt_val.at("skip") << std::endl
              << "read_qasm_files: " << opt_name2opt_val.at("read_qasm_files") << std::endl
              << "hwconfig: " << opt_name2opt_val.at("hwconfig") << std::endl
              << "nqubits: " << opt_name2opt_val.at("nqubits") << std::endl
              << "eqasm_compiler_name: " << opt_name2opt_val.at("eqasm_compiler_name") << std::endl;
}

/**
 * @brief  Displays the help menu to list the available options.
 */
void PassOptions::help() const {
    std::cout << app->help() << std::endl;
}

/**
 * @brief   Sets a pass option
 * @param   opt_name String option name
 * @param   opt_value String value of the option
 */
void PassOptions::setOption(const Str &opt_name, const Str &opt_value) {
    QL_DOUT("In PassOptions: setting option " << opt_name << " to value " << opt_value << std::endl);
    try {
       std::vector<Str> opts = {opt_value, "--"+opt_name};
       app->parse(opts);
    } catch (const std::exception &e) {
       app->reset();
       QL_EOUT("Un-known option: " << e.what());
       throw Exception("Error parsing options. " + Str(e.what()) + " ! \n Allowed values are: \n" + app->help(opt_name), false);
    }
    app->reset();
}

/**
 * @brief  Queries an option
 * @param opt_name Name of the options
 * @return Value of the option
 */
Str PassOptions::getOption(const Str &opt_name) const {
    Str opt_value("UNKNOWN");
    if (opt_name2opt_val.find(opt_name) != opt_name2opt_val.end()) {
        opt_value = opt_name2opt_val.at(opt_name);
    } else {
        QL_EOUT("Un-known option: " << opt_name << "\n Allowed values are: \n");
        std::cout << app->help(opt_name);
        exit(1);
    }
    return opt_value;
}

} // namespace ql
