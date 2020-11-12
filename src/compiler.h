/** \file
 * Modular entry point class for the OpenQL Compiler.
 */

#pragma once

#include "utils/str.h"
#include "program.h"
#include "passmanager.h"

namespace ql {

/**
 * Quantum compiler class
 */
class quantum_compiler {
public:

    quantum_compiler(const utils::Str &name);

    void compile(quantum_program*);
    void addPass(const utils::Str &realPassName, const utils::Str &symbolicPassName);
    void addPass(const utils::Str &realPassName);
    void setPassOption(const utils::Str &passName, const utils::Str &optionName, const utils::Str &optionValue);

private:

    void constructPassManager();//TODO: potentially read the IR->Options!

    utils::Str name;
    PassManager *passManager;
};

} // namespace ql
