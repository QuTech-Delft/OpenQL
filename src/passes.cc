/**
 * @file   passes.cc
 * @date   04/2020
 * @author Razvan Nane
 * @brief  OpenQL Passes
 * @note   Below passes should eventually be implemented into their own files.
 * @todo-rn Split this file into multiple (pass) files together with folder restructuring
 */

#include "passes.h"
#include "report.h"
#include "optimizer.h"
#include "clifford.h"
#include "decompose_toffoli.h"
#include "cqasm/cqasm_reader.h"
#include "latency_compensation.h"
#include "buffer_insertion.h"
         
#include <iostream>
#include <chrono>

/* JvS: guys, you can't define a class already defined in another header
 * *differently* and then "resolve" the inevitable compile errors by not
 * including the header. This fundamentally breaks how C++ works and resulted in
 * code that I'm astonished linked at all and even half-worked.
 *
 * In your defense, including the appropriate headers broke the rest of OpenQL
 * because the rest of OpenQL is also fundamentally broken, apparently relying
 * on headers only being included by a single compile units, which is precisely
 * NOT the function of a header file.
namespace ql
{
    class eqasm_compiler
    {
        public:
            virtual void compile(ql::quantum_program* programp, const ql::quantum_platform& plat);
    };
    
    namespace arch
    {
        class cbox_eqasm_compiler: public eqasm_compiler
        {
            public:
                cbox_eqasm_compiler(){}
        };

        class cc_light_eqasm_compiler: public eqasm_compiler
        {
            public:
                void ccl_decompose_pre_schedule(quantum_program* programp, const ql::quantum_platform& platform, std::string passname);
                void ccl_decompose_post_schedule(quantum_program* programp, const ql::quantum_platform& platform, std::string passname);
                void map(quantum_program* programp, const ql::quantum_platform& platform, std::string passname, std::string* stats);
                void write_quantumsim_script(quantum_program* programp, const ql::quantum_platform& platform, std::string passname);
                void qisa_code_generation(quantum_program* programp, const ql::quantum_platform& platform, std::string passname);
        };

        class eqasm_backend_cc: public eqasm_compiler
        {
            public:
                eqasm_backend_cc(){}
        };
    }
}
*/
#include "arch/cc_light/cc_light_eqasm_compiler.h"
#include "arch/cbox/cbox_eqasm_compiler.h"
#include "arch/cc/eqasm_backend_cc.h"

// JvS: this does not belong here *at all*. All header files should have their
// own CC file with their defs. But only putting the globals in a new
// corresponding CC file for now breaks linkage on MSVC, because annoyingly CC
// files are treated as unused and not linked when none of their *functions*
// are used.
namespace ql {
namespace arch {
size_t CurrSRegCount = 0;
size_t CurrTRegCount = 0;
} // namespace arch
} // namespace ql


namespace ql
{

extern void rcschedule(ql::quantum_program* programp, const ql::quantum_platform& platform, std::string passname);
extern void schedule(ql::quantum_program*, const ql::quantum_platform&, std::string);

    /**
     * @brief  Scheduler pass constructor
     * @param  Name of the scheduler pass
     */
AbstractPass::AbstractPass(std::string name)
{ 
    DOUT("In AbstractPass::AbstractPass set name " << name << std::endl);
    setPassName(name); 
    createPassOptions(); 
}

    /**
     * @brief   Gets the name of the pass
     * @return  Name of the compiler pass
     */
std::string AbstractPass::getPassName()
{
    return passName;
}

    /**
     * @brief  Sets the name of the pass
     * @param  Name of the compiler pass
     */
void AbstractPass::setPassName(std::string name) 
{ 
    passName = name; 
}

    /**
     * @brief   Sets a pass option
     * @param   optionName String option name
     * @param   optionValue String value of the option
     */
void AbstractPass::setPassOption(std::string optionName, std::string optionValue)
{
    DOUT("In AbstractPass::setPassOption");
    
    passOptions->setOption(optionName, optionValue);  
}

    /**
     * @brief   Initializes the pass options object
     */
void AbstractPass::createPassOptions()
{
    passOptions = new PassOptions(getPassName());
}

    /**
     * @brief   Queries the skip option of the pass
     * @return  bool representing wheather the pass should be skipped
     */
bool AbstractPass::getSkip()
{
    if(getPassOptions()->getOption("skip") == "yes")
        return true;
    else 
        return false;
}

    /**
     * @brief   Modifies/Stores statistics about the pass
     */
void AbstractPass::appendStatistics(std::string statistic)
{
    statistics += statistic;
}
    
    /**
     * @brief   Initializes the pass by printing useful information
     */
void AbstractPass::initPass(ql::quantum_program *program)
{
    if(getPassOptions()->getOption("write_qasm_files") == "yes")
    {
        //temporary store old value 
        ///@note-rn: this is only needed to overwrite global option set for old program flow for compatibility reasons ==> This should be deprecated when we remove old code
        std::string writeQasmLocal = ql::options::get("write_qasm_files");
        ql::options::set("write_qasm_files", "yes");
        
        ql::report_qasm(program, program->platform, "in", getPassName());
        
        ql::options::set("write_qasm_files", writeQasmLocal);
        
    }
    
    if(getPassOptions()->getOption("write_report_files") == "yes")
    {
        //temporary store old value 
        ///@note-rn: this is only needed to overwrite global option set for old program flow for compatibility reasons ==> This should be deprecated when we remove old code
        std::string writeReportLocal = ql::options::get("write_report_files");
        ql::options::set("write_report_files", "yes");
        
        ql::report_statistics(program, program->platform, "in", getPassName(), "# ");
        
        ql::options::set("write_report_files", writeReportLocal);
    }
}

    /**
     * @brief   Finilazes the pass by printing useful information and cleaning
     */
void AbstractPass::finalizePass(ql::quantum_program *program)
{
    if(getPassOptions()->getOption("write_qasm_files") == "yes")
    {
        //temporary store old value 
        ///@note-rn: this is only needed to overwrite global option set for old program flow for compatibility reasons ==> This should be deprecated when we remove old code
        std::string writeQasmLocal = ql::options::get("write_qasm_files");
        ql::options::set("write_qasm_files", "yes");
        
        ql::report_qasm(program, program->platform, "out", getPassName());
        
        ql::options::set("write_qasm_files", writeQasmLocal);
    }
    
    if(getPassOptions()->getOption("write_report_files") == "yes")
    {
        //temporary store old value 
        ///@note-rn: this is only needed to overwrite global option set for old program flow for compatibility reasons ==> This should be deprecated when we remove old code
        std::string writeReportLocal = ql::options::get("write_report_files");
        ql::options::set("write_report_files", "yes");
        
        ql::report_statistics(program, program->platform, "out", getPassName(), "# ", getPassStatistics());
        
        ql::options::set("write_report_files", writeReportLocal);
    }
    
    resetStatistics();
}

    /**
     * @brief  Apply the pass to the input program
     * @param  Program object to be read
     */
void ReaderPass::runOnProgram(ql::quantum_program *program)
{
    DOUT("run ReaderPass with name = " << getPassName() << " on program " << program->name);
    
    ql::cqasm_reader* reader = new ql::cqasm_reader(program->platform, *program);
    
    DOUT("!!!!!!!!!!! start reader !!!!!!!!");    

    // reset kernels if they are not empty, needed for the case when the reader pass 
    // is used after a Writer pass within the sequence os passes and not at the start 
    // of the compiler when there is no IR
    auto it = program->kernels.begin();

    for(;it != program->kernels.end(); it++)
    {
        ql::quantum_kernel ktmp = (*it);//->kernels.erase(*it);
        std::cout << ktmp.name << std::endl;
        ktmp.c.clear();
    }
        
    program->kernels.clear();
    
    ///@todo-rn: come up with a parametrized naming scheme to do this printing. This should reflect
    // if the pass is outputing non- or scheduled qasm depending if it is used before or after sched
    // currently this works only when Writer pass creating the qasm file is called outputIR
    reader->file2circuit(ql::options::get("output_dir")+"/"+program->name+"_outputIR_out.qasm");
}

    /**
     * @brief  Apply the pass to the input program
     * @param  Program object to be write
     */
void WriterPass::runOnProgram(ql::quantum_program *program)
{
    DOUT("run WriterPass with name = " << getPassName() << " on program " << program->name);

    // report/write_qasm initialization
    //ql::report_init(program, program->platform);
    
    // writer pass of the initial qasm file (program.qasm)
    ql::report_qasm(program, program->platform, "out", getPassName());
    
//     if (getPassName() != "initialqasmwriter" && getPassName() != "scheduledqasmwriter")
//     { ///@note-rn: temoporary hack to make the writer pass for those 2 configurations soft (i.e., do not delete the subcircuits) so that it does not require a reader pass after it!. This is needed until we fix the synchronization between hardware configuration files and openql tests. Until then a Reader pass would be needed after a hard Write pass. However, a Reader pass will make some unit tests to fail due to a mismatch between the instructions in the tests (i.e., prepz) and included/defined in the hardware config files CONFLICTING with the prepz instr not being available in libQASM.
}

    /**
     * @brief  Apply the pass to the input program
     * @param  Program object to be read
     */
void RotationOptimizerPass::runOnProgram(ql::quantum_program *program)
{
    DOUT("run RotationOptimizerPass with name = " << getPassName() << " on program " << program->name);
    
    rotation_optimize(program, program->platform, "rotation_optimize");
}
  
    /**
     * @brief  Apply the pass to the input program
     * @param  Program object to be read
     */
void DecomposeToffoliPass::runOnProgram(ql::quantum_program *program)
{
    DOUT("run DecomposeToffoliPass with name = " << getPassName() << " on program " << program->name);
    
    // decompose_toffoli pass
    ql::decompose_toffoli(program, program->platform, "decompose_toffoli");
}

    /**
     * @brief  Apply the pass to the input program
     * @param  Program object to be read
     */
void SchedulerPass::runOnProgram(ql::quantum_program *program)
{
    DOUT("run SchedulerPass with name = " << getPassName() << " on program " << program->name);
    
    // prescheduler pass
    ql::schedule(program, program->platform, "prescheduler");
}

    /**
     * @brief  Apply the pass to the input program
     * @param  Program object to be read
     */
void BackendCompilerPass::runOnProgram(ql::quantum_program *program)
{
    DOUT("run BackendCompilerPass with name = " << getPassName() << " on program " << program->name);
    
    std::unique_ptr<ql::eqasm_compiler> backend_compiler;
    
    std::string eqasm_compiler_name = program->platform.eqasm_compiler_name;
    //getPassOptions()->getOption("eqasm_compiler_name");
    
    if (eqasm_compiler_name == "qumis_compiler")
        ///@todo-rn: REMOVE THIS DURING CLEANUP together with the whole cbox backend!
        backend_compiler = std::unique_ptr<ql::eqasm_compiler>(new ql::arch::cbox_eqasm_compiler());
    else if (eqasm_compiler_name == "cc_light_compiler" )
    {
        backend_compiler = std::unique_ptr<ql::eqasm_compiler>(new ql::arch::cc_light_eqasm_compiler());
    }
    else if (eqasm_compiler_name == "eqasm_backend_cc" )
        backend_compiler = std::unique_ptr<ql::eqasm_compiler>(new ql::arch::eqasm_backend_cc());
    else
    {
        FATAL("the '" << eqasm_compiler_name << "' eqasm compiler backend is not suported !");
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
     * @brief  Report Statistics for the input program
     * @param  Program object to be read
     */
void ReportStatisticsPass::runOnProgram(ql::quantum_program *program)
{
    ///@note-rn: below call should be manually inlined here and removed from its current location
    ///@note-rn: pass should be moved to separate file containing only this pass
    ql::report_statistics(program, program->platform, "todo-inout", getPassName(), "# ");
}

     /**
     * @brief  Prepare the program for code generation
     * @param  Program object to be prepared
     */
void CCLPrepCodeGeneration::runOnProgram(ql::quantum_program *program)
{
    const json& instruction_settings = program->platform.instruction_settings;
    for(const json & i : instruction_settings)
    {
       if(i.count("cc_light_instr") <= 0)
       {
            FATAL("cc_light_instr not found for " << i);
       }
    }
}

    /**
     * @brief  Decompose the input program before scheduling
     * @param  Program object to be decomposed
     */
void CCLDecomposePreSchedule::runOnProgram(ql::quantum_program *program)
{
    std::unique_ptr<ql::arch::cc_light_eqasm_compiler> ccl_backend_compiler(new ql::arch::cc_light_eqasm_compiler());
    
    ccl_backend_compiler->ccl_decompose_pre_schedule(program, program->platform, getPassName());
    
    ccl_backend_compiler.reset();
}

    /**
     * @brief  Generate QuantumSim output 
     * @param  Program object to be simulated using quantumsim
     */
void WriteQuantumSimPass::runOnProgram(ql::quantum_program *program)
{
    std::unique_ptr<ql::arch::cc_light_eqasm_compiler> ccl_backend_compiler(new ql::arch::cc_light_eqasm_compiler());
    
    ccl_backend_compiler->write_quantumsim_script(program, program->platform, getPassName());
    
    ccl_backend_compiler.reset();
}

    /**
     * @brief  Clifford optimizer
     * @param  Program object to be clifford optimized
     */
void CliffordOptimizePass::runOnProgram(ql::quantum_program *program)
{
    ql::clifford_optimize(program, program->platform, getPassName());
}

    /**
     * @brief  Maps the input program to the target platform
     * @param  Program object to be mapped
     */
void MapPass::runOnProgram(ql::quantum_program *program)
{
    std::unique_ptr<ql::arch::cc_light_eqasm_compiler> ccl_backend_compiler(new ql::arch::cc_light_eqasm_compiler());
    
    std::string stats;
    
    ccl_backend_compiler->map(program, program->platform, getPassName(), &stats);
    
    appendStatistics(stats);
    
    ccl_backend_compiler.reset();
}

    /**
     * @brief  Resource Constraint Scheduling of the input program
     * @param  Program object to be rcscheduled
     */
void RCSchedulePass::runOnProgram(ql::quantum_program *program)
{
    ql::rcschedule(program, program->platform, getPassName());
}

    /**
     * @brief  Apply Latency Compensation to the scheduled program
     * @param  Program object to be latency compensated
     */
void LatencyCompensationPass::runOnProgram(ql::quantum_program *program)
{
    ql::latency_compensation(program, program->platform, getPassName());
}

    /**
     * @brief  Insert Buffer Delays to the input program
     * @param  Program object to be extended with buffer delays
     */
void InsertBufferDelaysPass::runOnProgram(ql::quantum_program *program)
{
    ql::insert_buffer_delays(program, program->platform, getPassName());
}

    /**
     * @brief  CC-Light specific Decomposition of the scheduled program
     * @param  Program object to be postscheduler decomposed
     */
void CCLDecomposePostSchedulePass::runOnProgram(ql::quantum_program *program)
{
    std::unique_ptr<ql::arch::cc_light_eqasm_compiler> ccl_backend_compiler(new ql::arch::cc_light_eqasm_compiler());
    
    ccl_backend_compiler->ccl_decompose_post_schedule(program, program->platform, getPassName());
    
    ccl_backend_compiler.reset();
}

    /**
     * @brief  Generate the QISA output from the input program
     * @param  Program object to be transformed into QISA output
     */
void QisaCodeGenerationPass::runOnProgram(ql::quantum_program *program)
{
    std::unique_ptr<ql::arch::cc_light_eqasm_compiler> ccl_backend_compiler(new ql::arch::cc_light_eqasm_compiler());

    ccl_backend_compiler->qisa_code_generation(program, program->platform, getPassName());
    
    ccl_backend_compiler.reset();
}

    /**
     * @brief  Construct an object to hold the pass options
     * @param  app_name String with the name of the pass options object
     */
PassOptions::PassOptions(std::string app_name)
{
    app = new CLI::App(app_name);

    ///@todo-rn: update this list with meaningful pass options
    // default values
    opt_name2opt_val["skip"] = "no";
    opt_name2opt_val["write_report_files"] = "no";
    opt_name2opt_val["write_qasm_files"] = "no";
    opt_name2opt_val["read_qasm_files"] = "no";
    opt_name2opt_val["hwconfig"] = "none";
    opt_name2opt_val["nqubits"] = "100";
    opt_name2opt_val["eqasm_compiler_name"] = "cc_light_compiler";

    // add options with default values and list of possible values
    app->add_set_ignore_case("--skip", opt_name2opt_val["skip"], {"yes", "no"}, "skip running the pass", true);
    app->add_set_ignore_case("--write_report_files", opt_name2opt_val["write_report_files"], {"yes", "no"}, "report compiler statistics", true);
    app->add_set_ignore_case("--write_qasm_files", opt_name2opt_val["write_qasm_files"], {"yes", "no"}, "write (un-)scheduled (with and without resource-constraint) qasm files", true);
    app->add_set_ignore_case("--read_qasm_files", opt_name2opt_val["read_qasm_files"], {"yes", "no"}, "read (un-)scheduled (with and without resource-constraint) qasm files", true);
    app->add_option("--hwconfig", opt_name2opt_val["hwconfig"], "path to the platform configuration file", true);
    app->add_option("--nqubits", opt_name2opt_val["nqubits"], "number of qubits used by the program", true);
    app->add_set_ignore_case("--eqasm_compiler_name", opt_name2opt_val["eqasm_compiler_name"], {"qumis_compiler", "cc_light_compiler", "eqasm_backend_cc"}, "Set the compiler backend", true);
}

    /**
     * @brief  Show the values set for the pass options.
     */
void PassOptions::print_current_values()
{
    ///@todo-rn: update this list with meaningful pass options
    std::cout << "write_qasm_files: " << opt_name2opt_val["write_qasm_files"] << std::endl
              << "write_report_files: " << opt_name2opt_val["write_report_files"] << std::endl
              << "skip: " << opt_name2opt_val["skip"] << std::endl
              << "read_qasm_files: " << opt_name2opt_val["read_qasm_files"] << std::endl
              << "hwconfig: " << opt_name2opt_val["hwconfig"] << std::endl
              << "nqubits: " << opt_name2opt_val["nqubits"] << std::endl
              << "eqasm_compiler_name: " << opt_name2opt_val["eqasm_compiler_name"] << std::endl;
}

    /**
     * @brief  Displays the help menu to list the available options.
     */
void PassOptions::help()
{
    std::cout << app->help() << std::endl;
}

    /**
     * @brief   Sets a pass option
     * @param   opt_name String option name
     * @param   opt_value String value of the option
     */
void PassOptions::setOption(std::string opt_name, std::string opt_value)
{
    DOUT("In PassOptions: setting option " << opt_name << " to value " << opt_value << std::endl);
    try
    {
       std::vector<std::string> opts = {opt_value, "--"+opt_name};
       app->parse(opts);
    }
    catch (const std::exception &e)
    {
       app->reset();
       EOUT("Un-known option: "<< e.what());
       throw ql::exception("Error parsing options. "+std::string(e.what())+" ! \n Allowed values are: \n"+app->help(opt_name),false);

    }
    app->reset();
}

    /**
     * @brief  Queries an option
     * @param opt_name Name of the options
     * @return Value of the option
     */
std::string PassOptions::getOption(std::string opt_name)
{
    std::string opt_value("UNKNOWN");
    if( opt_name2opt_val.find(opt_name) != opt_name2opt_val.end() )
    {
        opt_value = opt_name2opt_val[opt_name];
    }
    else
    {
        EOUT("Un-known option: "<< opt_name << "\n Allowed values are: \n");
        std::cout << app->help(opt_name);
        exit(1);
    }
    return opt_value;
}


} // ql
