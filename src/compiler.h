/** \file
 * Modular entry point class for the OpenQL Compiler.
 */

#pragma once

#include "utils/str.h"
#include "program.h"
#include "passes/passmanager.h"

namespace ql {

/**
 * Quantum compiler class
 */
class quantum_compiler {
public:

    quantum_compiler(const utils::Str &name);
    quantum_compiler(const utils::Str &name, const utils::Str &configuration_file_name);

    void compile(quantum_program*);
    void addPass(const utils::Str &realPassName, const utils::Str &symbolicPassName);
    void addPass(const utils::Str &realPassName);
    void setPassOption(const utils::Str &passName, const utils::Str &optionName, const utils::Str &optionValue);
    void loadPassesFromConfigFile(const utils::Str &passManagerName, const utils::Str &configFile);
    
private:

    void constructPassManager();//TODO: potentially read the IR->Options!

    utils::Str name;
    utils::Str configuration_file_name;
    PassManager *passManager;
};

} // namespace ql
