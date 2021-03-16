/** \file
 * Modular entry point class for the OpenQL Compiler.
 */

#include "compiler.h"

#include <iostream>
#include "options.h"

namespace ql {

using namespace utils;

/**
 * @brief   Compiler constructor (empty)
 * @param   name Name of the compiler
 */
quantum_compiler::quantum_compiler(const Str &n) : name(n), configuration_file_name("empty") {
    QL_DOUT("In quantum_compiler constructor before PassManager initialization ");
    constructPassManager();
}

/**
 * @brief   Compiler constructor (using configuration file)
 * @param   name Name of the compiler
 */
quantum_compiler::quantum_compiler(const Str &n, const Str &cfg) : name(n), configuration_file_name(cfg) {
    QL_DOUT("In quantum_compiler constructor before PassManager initialization using configuration file " << cfg);
    constructPassManager();
}

/**
 * @brief   Compiles the program passed as parameter
 * @param   quantum_program   Object reference to the program to be compiled
 */
void quantum_compiler::compile(quantum_program *program) {
    QL_DOUT("Compiler compiles program ");
    passManager->compile(program);
}

/**
 * @brief   Adds a compiler pass with its actual name to the pass manager
 * @param   realPassName String of the actual pass name
 * @param   symbolicPassName String of the alias pass name
 */
void quantum_compiler::addPass(const Str &realPassName, const Str &symbolicPassName) {
    QL_DOUT("Add real pass named: " << realPassName << " with alias " << symbolicPassName);
    passManager->addPassNamed(realPassName, symbolicPassName);
}

/**
 * @brief   Adds a compiler pass with its actual name to the pass manager
 * @param   realPassName String of the actual pass name
 */
void quantum_compiler::addPass(const Str &realPassName) {
    QL_DOUT("Add real pass named: " << realPassName);
    passManager->addPassNamed(realPassName, realPassName);
}

/**
 * @brief   Sets a pass option
 * @param   passName String name of the pass
 * @param   optionName String option name
 * @param   optionValue String value of the option
 */
void quantum_compiler::setPassOption(
    const Str &passName,
    const Str &optionName,
    const Str &optionValue
) {
    QL_DOUT(" Set option " << optionName << " = " << optionValue << " for pass " << passName);

    if (passName == "ALL") {
        passManager->setPassOptionAll(optionName, optionValue);
    } else {
        AbstractPass *pass = passManager->findPass(passName);
        assert(pass);
        pass->setPassOption(optionName, optionValue);
    }
}

/**
 * @brief   Configures the passes of the compiler based on an external configuration file
 * @param   name Name of the new configured compiler
 * @param   cfg Name of the compiler configuration file
 */
void quantum_compiler::loadPassesFromConfigFile(const Str &newName, const Str &cfg) {
    passManager->loadPassesFromConfigFile(newName,cfg);
}

/**
 * @brief   Constructs the sequence of compiler passes
 */
void quantum_compiler::constructPassManager() {
    QL_DOUT("Construct the passManager " << name << " using configuration file: " << configuration_file_name);
    if(configuration_file_name == "empty")
        passManager = new PassManager(name);
    else
        passManager = new PassManager(name, configuration_file_name);

    assert(passManager);
}
} // namespace ql
