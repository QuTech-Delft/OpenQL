/**
 * @file   compiler.h
 * @date   04/2020
 * @author Razvan Nane
 * @brief  OpenQL Compiler
 */

#pragma once

#include <string>

#include "program.h"
#include "passmanager.h"

namespace ql {

/**
 * Quantum compiler class
 */
class quantum_compiler {
public:

    quantum_compiler(const std::string &name);

    void compile(ql::quantum_program*);
    void addPass(const std::string &realPassName, const std::string &symbolicPassName);
    void addPass(const std::string &realPassName);
    void setPassOption(const std::string &passName, const std::string &optionName, const std::string &optionValue);

private:

    void constructPassManager();//TODO: potentially read the IR->Options!

    std::string           name;
    ql::PassManager       *passManager;
};

} // namespace ql
