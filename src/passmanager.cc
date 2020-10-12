/**
 * @file   passmanager.cc
 * @date   04/2020
 * @author Razvan Nane
 * @brief  OpenQL Pass Manager
 */

#include "passmanager.h"
#include "write_sweep_points.h"

namespace ql
{

    /**
     * @brief   PassManager constructor
     * @param   name Name of the pass manager 
     */
PassManager::PassManager(std::string name): name(name) {}

    /**
     * @brief   Applies the sequence of compiler passes to the given program
     * @param   program   Object reference to the program to be compiled
     */
void PassManager::compile(ql::quantum_program *program)
{
   
   DOUT("In PassManager::compile ... ");
   for(auto pass : passes)
    {
        ///@todo-rn: implement option to check if following options are actually needed for a pass
        ///@note-rn: currently(0.8.1.dev), all passes require platform as API parameter, and some passes depend on the nqubits internally. Therefore, these are passed through by setting the program with these fields here. However, this should change in the future since compiling for a simulator might not require a platform, and the number of qubits could be optional.
        
        if(!program->qubit_count)
            program->qubit_count = (size_t)(std::stoi(pass->getPassOptions()->getOption("nqubits")));
        assert(program->qubit_count);
        
        //If the old interface is used, platform is already set, so it is not needed to look for platform option and configure the platform from there
        if(!program->platformInitialized)
        {
            std::string hwconfig = pass->getPassOptions()->getOption("hwconfig");   
            program->platform = *(new ql::quantum_platform("testPlatform",hwconfig));
        }
        //assert(program->platform); JvS: program->platform is not a pointer and cannot be cast to a bool, so this line broke when I turned optimizations off
   
        if(!pass->getSkip())
        {
            DOUT(" Calling pass: " << pass->getPassName());
            pass->initPass(program);
            pass->runOnProgram(program);
            pass->finalizePass(program);
        }
    }
    
        // generate sweep_points file ==> TOOD: delete?
        ql::write_sweep_points(program, program->platform, "write_sweep_points");
}
   
    /**
     * @brief   Adds a compiler pass to the pass manager
     * @param   pass Object reference to the pass to be added
     */
void PassManager::addPassNamed(std::string realPassName, std::string symbolicPassName)
{
    DOUT("In PassManager::addPassNamed ");
    
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
AbstractPass* PassManager::createPass(std::string passName, std::string aliasName)
{
    DOUT("In PassManager::createPass");
    
    /// @todo-rn: check that aliasname has not been used before!
    bool passfound = false;
    AbstractPass* pass;
    
    /** Search between available passes in OpenQL. 
     * @note PassManager::createPass defines the collection of passes available in OpenQL. Whenever a new pass is added to the compiler, this list should be extended in order for the pass to be found.
     */
    
    if (passName == "Reader") {pass = new ReaderPass(aliasName); passfound = true;}
    if (passName == "Writer") {pass = new WriterPass(aliasName); passfound = true;}
    if (passName == "RotationOptimizer") {pass = new RotationOptimizerPass(aliasName); passfound = true;}
    if (passName == "DecomposeToffoli") {pass = new DecomposeToffoliPass(aliasName); passfound = true;}
    if (passName == "Scheduler") {pass = new SchedulerPass(aliasName); passfound = true;}
    if (passName == "BackendCompiler") {pass = new BackendCompilerPass(aliasName); passfound = true;}
    if (passName == "ReportStatistics") {pass = new ReportStatisticsPass(aliasName); passfound = true;}

    if (passName == "CCLPrepCodeGeneration") {pass = new CCLPrepCodeGeneration(aliasName); passfound = true;}
    if (passName == "CCLDecomposePreSchedule") {pass = new CCLDecomposePreSchedule(aliasName); passfound = true;}
    if (passName == "WriteQuantumSim") {pass = new WriteQuantumSimPass(aliasName); passfound = true;}
    if (passName == "CliffordOptimize") {pass = new CliffordOptimizePass(aliasName); passfound = true;}
    if (passName == "Map") {pass = new MapPass(aliasName); passfound = true;}
    if (passName == "RCSchedule") {pass = new RCSchedulePass(aliasName); passfound = true;}
    if (passName == "LatencyCompensation") {pass = new LatencyCompensationPass(aliasName); passfound = true;}
    if (passName == "InsertBufferDelays") {pass = new InsertBufferDelaysPass(aliasName); passfound = true;}
    if (passName == "CCLDecomposePostSchedule") {pass = new CCLDecomposePostSchedulePass(aliasName); passfound = true;}
    if (passName == "QisaCodeGeneration") {pass = new QisaCodeGenerationPass(aliasName); passfound = true;}
    
    if (!passfound) 
    {
        EOUT(" !!!Error: Pass " << aliasName << " not found!!!");
        exit(1);
    }

    return pass;
}

    /**
     * @brief   Searches for the pass with the given name
     * @param   passName String representing the name of the pass to be found
     * @return   AbstracPass Object reference to the pass 
     */
AbstractPass* PassManager::findPass(std::string passName)
{
    DOUT("In PassManager::findPass");
    AbstractPass *pass;
    bool passFound = false;
    std::list<class AbstractPass*>::iterator it;
    
    for( it = passes.begin(); it != passes.end(); ++it)
    {
        if((*it)->getPassName() == passName)
        {
            DOUT(" Found pass " << passName);
            pass = (*it);
            passFound = true;
            break;
        }
    }
    
    if(!passFound)
    {
        EOUT("!!!Error: Pass " << passName << " not found!");
        exit(1);
    }
    
    return pass;    
}

    /**
     * @brief   Adds a compiler pass to the pass manager
     * @param   pass Object reference to the pass to be added
     */
void PassManager::addPass(AbstractPass* pass)
{
    DOUT("In PassManager::addPass");
    passes.push_back(pass);
}

    /**
     * @brief   Sets global option, i.e., for all passes
     * @param   optionName String option name
     * @param   optionValue String value of the option
     */
void PassManager::setPassOptionAll(std::string optionName, std::string optionValue)
{
    DOUT("In PassManager::setPassOptionAll");
    
    std::list<class AbstractPass*>::iterator it;
    
    for( it = passes.begin(); it != passes.end(); ++it)
    {
        DOUT(" Pass: " << (*it)->getPassName() << " --> set option " << optionName << " to " << optionValue << std::endl);
        
        (*it)->setPassOption(optionName, optionValue);
    }    
}


} // ql
