/**
 * @file   passmanager.cc
 * @date   04/2020
 * @author Razvan Nane
 * @brief  OpenQL Pass Manager
 */

#include "passmanager.h"

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
        pass->runOnProgram(program);
    }
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
    AbstractPass *pass;
    
    /// @todo-rn: check that aliasname has not been used before!
    
    /** Search between available passes in OpenQL. 
     * @note PassManager::createPass defines the collection of passes available in OpenQL. Whenever a new pass is added to the compiler, this list should be extended in order for the pass to be found.
     */
    
    if (passName == "Reader") return new ReaderPass(aliasName);
    if (passName == "Writer") return new WriterPass(aliasName);
    if (passName == "Optimizer") return new OptimizerPass(aliasName);
    if (passName == "Scheduler") return new SchedulerPass(aliasName);
    
    WOUT("========================= Pass not found! ============================");
}

    /**
     * @brief   Searches for the pass with the given name
     * @param   passName String representing the name of the pass to be found
     * @return   AbstracPass Object reference to the pass 
     */
AbstractPass* PassManager::findPass(std::string passName)
{
    DOUT("In PassManager::findPass");
    
    std::list<class AbstractPass*>::iterator it;
    
    for( it = passes.begin(); it != passes.end(); ++it)
    {
        if((*it)->getPassName() == passName)
        {
            DOUT(" Found pass " << passName);
            return (*it);
        }
    }
    
    WOUT(" PASS " << passName << " not found!");
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

} // ql
