/**
 * @file   compiler.cc
 * @date   04/2020
 * @author Razvan Nane
 * @brief  OpenQL Compiler
 */

#include <iostream>
#include <fstream>

#include "compiler.h"

namespace ql
{

    /**
     * @brief   Compiler constructor
     * @param   name Name of the compiler 
     */
quantum_compiler::quantum_compiler(std::string n): name(n) 
{
    DOUT("In quantum_compiler constructor before PassManager initialization ");
    constructPassManager();
}

    /**
     * @brief   Compiles the program passed as parameter
     * @param   quantum_program   Object reference to the program to be compiled
     */
void quantum_compiler::compile(ql::quantum_program *program)
{
    DOUT("Compiler compiles program ");
    passManager->compile(program);
}

    /**
     * @brief   Adds a compiler pass with its actual name to the pass manager
     * @param   realPassName String of the actual pass name
     * @param   symbolicPassName String of the alias pass name
     */
void quantum_compiler::addPass(std::string realPassName, std::string symbolicPassName)
{
    DOUT("Add real pass named: " << realPassName << " with alias " << symbolicPassName);
    passManager->addPassNamed(realPassName, symbolicPassName);
}

    /**
     * @brief   Adds a compiler pass with its actual name to the pass manager
     * @param   realPassName String of the actual pass name
     */
void quantum_compiler::addPass(std::string realPassName)
{
    DOUT("Add real pass named: " << realPassName);
    passManager->addPassNamed(realPassName, realPassName);
}

    /**
     * @brief   Sets a pass option
     * @param   passName String name of the pass
     * @param   optionName String option name
     * @param   optionValue String value of the option
     */
void quantum_compiler::setPassOption(std::string passName, std::string optionName, std::string optionValue)
{
    DOUT(" Set option " << optionName << " = " << optionValue << " for pass " << passName); 
    
    if (passName == "ALL")
    {
        passManager->setPassOptionAll(optionName, optionValue);
    }
    else
    {
        AbstractPass *pass = passManager->findPass(passName);  
        assert(pass);
        pass->setPassOption(optionName, optionValue);
    }
}

    /**
     * @brief   Constructs the sequence of compiler passes
     */
void quantum_compiler::constructPassManager()
{
    DOUT("Construct the passManager");
    passManager = new PassManager("empty");
    
    assert(passManager);
}
    
} // ql

