/** \file
 * OpenQL Passes.
 * 
 * @note Below passes should eventually be implemented into their own files.
 */

#include "passes.h"

#include <iostream>
#include "ql/utils/opt.h"
#include "report.h"
#include "optimizer.h"
#include "clifford.h"
#include "decompose_toffoli.h"
#include "cqasm/cqasm_reader.h"
#include "latency_compensation.h"
#include "buffer_insertion.h"
#include "commute_variation.h"
#include "scheduler.h"
#include "ql/pass/ana/visualize/circuit.h"
#include "ql/pass/ana/visualize/interaction.h"
#include "ql/pass/ana/visualize/mapping.h"

#include "arch/cc_light/cc_light_eqasm_compiler.h"
#include "arch/cc/backend_cc.h"

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

    auto reader = cqasm::Reader(program->platform, program);

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
 * @brief  Rotation optimizer pass constructor
 * @param  Name of the optimized pass
 */
RotationOptimizerPass::RotationOptimizerPass(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Apply the pass to the input program
 * @param  Program object to be read
 */
void RotationOptimizerPass::runOnProgram(const ir::ProgramRef &program) {
    QL_DOUT("run RotationOptimizerPass with name = " << getPassName() << " on program " << program->name);

    rotation_optimize(program, program->platform, "rotation_optimize");
}

/**
 * @brief  Rotation optimizer pass constructor
 * @param  Name of the optimized pass
 */
ToffoliDecomposerPass::ToffoliDecomposerPass(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Apply the pass to the input program
 * @param  Program object to be read
 */
void ToffoliDecomposerPass::runOnProgram(const ir::ProgramRef &program) {
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
    
    if (eqasm_compiler_name == "cc_light_compiler") {
        arch::cc_light_eqasm_compiler::compile(program, program->platform);
    } else if (eqasm_compiler_name == "eqasm_backend_cc") {
        arch::cc::Backend().compile(program, program->platform);
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
 * @brief  CCL Preparation for Code Generation pass constructor
 * @param  Name of the preparation pass
 */
CCLConsistencyCheckerPass::CCLConsistencyCheckerPass(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Prepare the program for code generation
 * @param  Program object to be prepared
 */
void CCLConsistencyCheckerPass::runOnProgram(const ir::ProgramRef &program) {
    const Json &instruction_settings = program->platform->instruction_settings;
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
CCLPreScheduleDecomposer::CCLPreScheduleDecomposer(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Decompose the input program before scheduling
 * @param  Program object to be decomposed
 */
void CCLPreScheduleDecomposer::runOnProgram(const ir::ProgramRef &program) {
    arch::cc_light_eqasm_compiler().ccl_decompose_pre_schedule(program, program->platform, getPassName());
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
    Str stats;
    arch::cc_light_eqasm_compiler::map(program, program->platform, getPassName(), &stats);
    appendStatistics(stats);
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
    clifford_optimize(program, program->platform, getPassName());
}

/**
 * @brief  Commute variation pass constructor
 * @param  Name of the commute_variation pass
 */
CommuteVariationOptimizerPass::CommuteVariationOptimizerPass(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Exploit commuting of gates circuit-wide to minimize circuit latency beyond locally in the scheduler
 * @param  Program object to be latency compensated
 */
void CommuteVariationOptimizerPass::runOnProgram(const ir::ProgramRef &program) {
    commute_variation(program, program->platform, getPassName());
}

/**
 * @brief  Resource Constraint Scheduler pass constructor
 * @param  Name of the scheduler pass
 */
RCSchedulerPass::RCSchedulerPass(const Str &name) : AbstractPass(name) {
};

/**
 * @brief  Resource Constraint Scheduling of the input program
 * @param  Program object to be rcscheduled
 */
void RCSchedulerPass::runOnProgram(const ir::ProgramRef &program) {
    rcschedule(program, program->platform, getPassName());
}

/**
 * @brief  Latency compensation pass constructor
 * @param  Name of the latency compensation pass
 */
LatencyCompensatorPass::LatencyCompensatorPass(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Apply Latency Compensation to the scheduled program
 * @param  Program object to be latency compensated
 */
void LatencyCompensatorPass::runOnProgram(const ir::ProgramRef &program) {
    latency_compensation(program, program->platform, getPassName());
}

/**
 * @brief  Insert Buffer Delays pass  constructor
 * @param  Name of the buffer delay insertion pass
 */
BufferDelayInserterPass::BufferDelayInserterPass(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Insert Buffer Delays to the input program
 * @param  Program object to be extended with buffer delays
 */
void BufferDelayInserterPass::runOnProgram(const ir::ProgramRef &program) {
    insert_buffer_delays(program, program->platform, getPassName());
}

/**
 * @brief  Decomposer Post Schedule  Pass
 * @param  Name of the decomposer pass
 */
CCLPostScheduleDecomposerPass::CCLPostScheduleDecomposerPass(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  CC-Light specific Decomposition of the scheduled program
 * @param  Program object to be postscheduler decomposed
 */
void CCLPostScheduleDecomposerPass::runOnProgram(const ir::ProgramRef &program) {
    arch::cc_light_eqasm_compiler().ccl_decompose_post_schedule(program, program->platform, getPassName());
}

/**
 * @brief  QuantumSim Writer Pass constructor
 * @param  Name of the writer pass
 */
QuantumSimWriterPass::QuantumSimWriterPass(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Generate QuantumSim output
 * @param  Program object to be simulated using quantumsim
 */
void QuantumSimWriterPass::runOnProgram(const ir::ProgramRef &program) {
    arch::cc_light_eqasm_compiler().write_quantumsim_script(program, program->platform, getPassName());
}

/**
 * @brief  QISA generation pass constructor
 * @param  Name of the QISA generator pass
 */
CCLCodeGeneratorPass::CCLCodeGeneratorPass(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Generate the QISA output from the input program
 * @param  Program object to be transformed into QISA output
 */
void CCLCodeGeneratorPass::runOnProgram(const ir::ProgramRef &program) {
    if (com::options::get("generate_code") == "yes") {
        arch::cc_light_eqasm_compiler::qisa_code_generation(program, program->platform, getPassName());
    }
}

/**
 * @brief  C printer pass constructor
 * @param  Name of the pass
 */
CPrinterPass::CPrinterPass(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Generate the C code equivalent to the input program
 * @param  Program object to be transformed into QISA output
 */
void CPrinterPass::runOnProgram(const ir::ProgramRef &program) {
    QL_DOUT("[OPENQL] Run CPrinter pass on program " << program->unique_name);
    
    write_c(program, program->platform, getPassName());
}

/**
 * @brief  External pass constructor
 * @param  Name of the pass
 */
RunExternalCompiler::RunExternalCompiler(const Str &name) : AbstractPass(name) {
}

/**
 * @brief  Generate the C code equivalent to the input program
 * @param  Program object to be transformed into QISA output
 */
void RunExternalCompiler::runOnProgram(const ir::ProgramRef &program) {
    std::string extcompname, copycmd;

    QL_DOUT("[OPENQL] Run ExternalCompiler pass with " << getPassName() << " compiler on program " << program->unique_name);

    //TODO: parametrize this so that we can run multiple external passes using this code! (use alias_name)
    system(("cp test_output/"+program->name+".c .").c_str());
    system(("./"+getPassName()+" -dumpall "+program->name+".c").c_str());
}

} // namespace ql
