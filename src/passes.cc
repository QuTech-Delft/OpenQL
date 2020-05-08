/**
 * @file   passes.cc
 * @date   04/2020
 * @author Razvan Nane
 * @brief  OpenQL Passes
 * @note   Below passes should eventually be implemented into their own files.
 * @todo-rn Split this file into multiple (pass) files together with folder restructuring
 */

#include "passes.h"

#include <iostream>

namespace ql
{

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
     * @brief  Reader pass constructor
     * @param  Name of the read pass
     */
ReadPass::ReadPass(std::string name) 
{ 
    setPassName(name); 
}

    /**
     * @brief  Apply the pass to the input program
     * @param  Program object to be read
     */
void ReadPass::runOnProgram(ql::quantum_program *program)
{
    DOUT("run ReadPass with name = " << getPassName() << " on program " << program->name);
    ///@todo-rn: call or import the actual reader pass from the openql file
}

    /**
     * @brief  Optimizer pass constructor
     * @param  Name of the optimized pass
     */
OptimizePass::OptimizePass(std::string name) 
{ 
    setPassName(name); 
}
    

    /**
     * @brief  Apply the pass to the input program
     * @param  Program object to be read
     */
void OptimizePass::runOnProgram(ql::quantum_program *program)
{
    DOUT("run OptimizePass with name = " << getPassName() << " on program " << program->name);
    
    std::vector<quantum_kernel> kernels = program->get_kernels(); 
    
    if( ql::options::get("optimize") == "yes" )
    {
        IOUT("optimizing quantum kernels...");
        for (auto k : kernels)
            k.optimize();
    }
}
  
    /**
     * @brief  Scheduler pass constructor
     * @param  Name of the scheduler pass
     */
SchedulerPass::SchedulerPass(std::string name)
{ 
    setPassName(name); 
}

    /**
     * @brief  Apply the pass to the input program
     * @param  Program object to be read
     */
void SchedulerPass::runOnProgram(ql::quantum_program *program)
{
    DOUT("run ReadPass with name = " << getPassName() << " on program " << program->name);
    
    ///@todo-rn: import the scheduler here and disentangle from platform. Needed to make a difference between prescheduler and rcscheduler, whereas the first can be used just for platform-independent simulations
    program->schedule(); 
}

} // ql
