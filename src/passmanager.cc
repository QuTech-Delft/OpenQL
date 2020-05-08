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
PassManager::PassManager(std::string name): name(name)
{
       // implicit hardcoded size of 3
       
       ///@todo-RN: add passes based on options and/or compiler configuration file
       ///@todo-RN: define concrete passes inside a passmanager & passes & front end & backend folder hierarchy
//         for(int i=0; i<1; i++)
//             switch(options[i]) {
//                case 0: addPass(new ReadPass("readpass"));break;
//                 default: break;
//             }
        ///@note: for initial testing we hardcode the order of passes to match the old situation
        addPass(new ReadPass("readpass"));
        addPass(new OptimizePass("Optimize"));
        addPass(new SchedulerPass("PreScheduler"));
    }

    /**
     * @brief   Applies the sequence of compiler passes to the given program
     * @param   program   Object reference to the program to be compiled
     */
void PassManager::compile(ql::quantum_program *program)
{
    for(auto pass : passes)
    {
        std::cout << "Pass: " ;
        pass->runOnProgram(program);
    }
}
        
    /**
     * @brief   Adds a compiler pass to the pass manager
     * @param   pass Object reference to the pass to be added
     */
void PassManager::addPass(AbstractPass* pass)
{
    passes.push_back(pass);
}


} // ql
