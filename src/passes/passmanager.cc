/** \file
 * OpenQL pass manager implementation.
 */

#include "utils/num.h"
#include "passmanager.h"
#include "write_sweep_points.h"

namespace ql {

using namespace utils;

/**
 * @brief   PassManager constructor
 * @param   name Name of the pass manager
 */
PassManager::PassManager(const Str &name) : name(name) {
}

/**
 * @brief   PassManager constructor and initialize from configuration file
 * @param   name Name of the pass manager
 * @param   cfg Name of the compiler configuration file
 */
PassManager::PassManager(const Str &name, const Str &cfg) : name(name), cfg_file_name(cfg) {
    loadPassesFromConfigFile(name, cfg);    
}

/**
 * @brief   Configures the passes of the compiler based on an external configuration file
 * @param   name Name of the new configured pass manager
 * @param   cfg Name of the compiler configuration file
 */
void PassManager::loadPassesFromConfigFile(const Str &newName, const Str &cfg) {
    name = newName;
    cfg_file_name = cfg;

    Json compilerConfig;
    
    QL_DOUT("Loading compiler configuration file " << cfg_file_name);
    compilerConfig = load_json(cfg_file_name); //note: fail to open error catched in util::json.cc
    
    for (auto it = compilerConfig["CompilerPasses"].begin(); it != compilerConfig["CompilerPasses"].end(); ++it) {
        
        Json compilerPass = *it;
        
        QL_DOUT("Found pass name " << compilerPass["passName"] << " with options " << compilerPass["options"] << " and alias name: " << compilerPass["passAlias"]);
        
        AbstractPass* pass = createPass(compilerPass["passName"], compilerPass["passAlias"]);
        
        assert(pass);
        addPass(pass);
        
        Json passOptions = compilerPass["options"];
        
        // We need to set the local pass options
        for(const auto &passOption : passOptions.items())
        {
            Json option = passOption.value();
                
            QL_DOUT("Found option " << option["optionName"] << " with value " << option["optionValue"]);
            pass->setPassOption(option["optionName"], option["optionValue"]);
        }
    }
}

/**
 * @brief   Applies the sequence of compiler passes to the given program
 * @param   program   Object reference to the program to be compiled
 */
void PassManager::compile(quantum_program *program) const {

    QL_DOUT("In PassManager::compile ... ");
    for (auto pass : passes) {
        ///@todo-rn: implement option to check if following options are actually needed for a pass
        ///@note-rn: currently(0.8.1.dev), all passes require platform as API parameter, and some passes depend on the nqubits internally. Therefore, these are passed through by setting the program with these fields here. However, this should change in the future since compiling for a simulator might not require a platform, and the number of qubits could be optional.

        if (!program->qubit_count) {
            program->qubit_count = pass->getPassOptions()["nqubits"].as_uint();
        }
        assert(program->qubit_count);

        //If the old interface is used, platform is already set, so it is not needed to look for platform option and configure the platform from there
        if (!program->platformInitialized) {
            Str hwconfig = pass->getPassOptions()["hwconfig"].as_str();
            program->platform = *(new quantum_platform("testPlatform",hwconfig));
        }

        if (!pass->getSkip()) {
            QL_DOUT(" Calling pass: " << pass->getPassName());
            pass->initPass(program);
            pass->runOnProgram(program);
            pass->finalizePass(program);
        }
    }

    // generate sweep_points file ==> TOOD: delete?
    write_sweep_points(program, program->platform, "write_sweep_points");
}

/**
 * @brief   Adds a compiler pass to the pass manager
 * @param   pass Object reference to the pass to be added
 */
void PassManager::addPassNamed(const Str &realPassName, const Str &symbolicPassName) {
    QL_DOUT("In PassManager::addPassNamed ");

    // search for pass after its name
    AbstractPass* pass = createPass(realPassName,symbolicPassName);

    assert(pass);

    addPass(pass);
}

/**
 * @brief   Searches for the pass with the given name
 * @param   passName String representing the name of the pass to be added to the compiler
 * @param   aliasName String representing the name of the pass to be used internally for searching
 * @return   AbstracPass Object reference to the pass
 */
AbstractPass *PassManager::createPass(const Str &passName, const Str &aliasName) {
    QL_DOUT("In PassManager::createPass");

    /// @todo-rn: check that aliasname has not been used before!
    AbstractPass *pass = nullptr;

    /** Search between available passes in OpenQL.
     * @note PassManager::createPass defines the collection of passes available in OpenQL. Whenever a new pass is added to the compiler, this list should be extended in order for the pass to be found.
     */

    if (passName == "Reader") {
        pass = new ReaderPass(aliasName);
    } else if (passName == "Writer") {
        pass = new WriterPass(aliasName);
    } else if (passName == "RotationOptimizer") {
        pass = new RotationOptimizerPass(aliasName);
    } else if (passName == "DecomposeToffoli") {
        pass = new DecomposeToffoliPass(aliasName);
    } else if (passName == "Scheduler") {
        pass = new SchedulerPass(aliasName);
    } else if (passName == "BackendCompiler") {
        pass = new BackendCompilerPass(aliasName);
    } else if (passName == "ReportStatistics") {
        pass = new ReportStatisticsPass(aliasName);
    } else if (passName == "CCLPrepCodeGeneration") {
        pass = new CCLPrepCodeGeneration(aliasName);
    } else if (passName == "CCLDecomposePreSchedule") {
        pass = new CCLDecomposePreSchedule(aliasName);
    } else if (passName == "WriteQuantumSim") {
        pass = new WriteQuantumSimPass(aliasName);
    } else if (passName == "CliffordOptimize") {
        pass = new CliffordOptimizePass(aliasName);
    } else if (passName == "Map") {
        pass = new MapPass(aliasName);
    } else if (passName == "CommuteVariation") {
        pass = new CommuteVariationPass(aliasName);
    } else if (passName == "RCSchedule") {
        pass = new RCSchedulePass(aliasName);
    } else if (passName == "LatencyCompensation") {
        pass = new LatencyCompensationPass(aliasName);
    } else if (passName == "InsertBufferDelays") {
        pass = new InsertBufferDelaysPass(aliasName);
    } else if (passName == "CCLDecomposePostSchedule") {
        pass = new CCLDecomposePostSchedulePass(aliasName);
    } else if (passName == "QisaCodeGeneration") {
        pass = new QisaCodeGenerationPass(aliasName);
    } else if (passName == "Visualizer") {
        pass = new VisualizerPass(aliasName);
    } else if (passName == "CPrinter") {
        pass = new CPrinterPass(aliasName);
    } else if (passName == "RunExternalCompiler") {
        pass = new RunExternalCompiler(aliasName);
    } else {
        QL_EOUT(" !!!Error: Pass " << aliasName << " not found!!!");
        exit(1);
    }

    return pass;
}

/**
 * @brief   Searches for the pass with the given name
 * @param   passName String representing the name of the pass to be found
 * @return   AbstracPass Object reference to the pass
 */
AbstractPass* PassManager::findPass(const Str &passName) {
    QL_DOUT("In PassManager::findPass");

    for (auto pass : passes) {
        if (pass->getPassName() == passName) {
            QL_DOUT(" Found pass " << passName);
            return pass;
        }
    }

    QL_EOUT("!!!Error: Pass " << passName << " not found!");
    exit(1);
}

/**
 * @brief   Sets global option, i.e., for all passes
 * @param   optionName String option name
 * @param   optionValue String value of the option
 */
void PassManager::setPassOptionAll(const Str &optionName, const Str &optionValue) {
    QL_DOUT("In PassManager::setPassOptionAll");

    for (auto pass : passes) {
        QL_DOUT(" Pass: " << pass->getPassName() << " --> set option " << optionName << " to " << optionValue << std::endl);
        pass->setPassOption(optionName, optionValue);
    }
}

/**
 * @brief   Adds a compiler pass to the pass manager
 * @param   pass Object reference to the pass to be added
 */
void PassManager::addPass(AbstractPass *pass) {
    QL_DOUT("In PassManager::addPass");
    passes.push_back(pass);
}

} // namespace ql
