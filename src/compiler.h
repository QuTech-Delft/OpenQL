/**
 * @file   compiler.h
 * @date   04/2020
 * @author Razvan Nane
 * @brief  OpenQL Compiler
 */

#ifndef QL_COMPILER_H
#define QL_COMPILER_H

#include "options.h"
#include "program.h"
#include "passmanager.h"

namespace ql
{

/**
 * Quantum compiler class
 */
class quantum_compiler
{
public:

    quantum_compiler(std::string name);

    void compile(ql::quantum_program*);
    void addPass(std::string realPassName, std::string symbolicPassName);
    void addPass(std::string realPassName);
    void setPassOption(std::string passName, std::string optionName, std::string optionValue);
    
private:
  
    void constructPassManager();//TODO: potentially read the IR->Options!
    
    std::string           name;
    ql::PassManager       *passManager;
};

} // ql

#endif //QL_COMPILER_H
