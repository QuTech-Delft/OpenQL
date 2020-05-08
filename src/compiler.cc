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
    constructPassManager();
}

    /**
     * @brief   Compiles the program passed as parameter
     * @param   quantum_program   Object reference to the program to be compiled
     */
void quantum_compiler::compile(ql::quantum_program *program)
{
    passManager->compile(program);
}

    /**
     * @brief   Constructs the sequence of compiler passes
     * @note    This accesses the global IR to retrieve the options selected by the user
     */
void quantum_compiler::constructPassManager()
{
    ///@todo-RN: read the pass options here!
    DOUT("Cconstruct the passManager");
    passManager = new PassManager("empty");
    
    assert(passManager);
}

    
} // ql

