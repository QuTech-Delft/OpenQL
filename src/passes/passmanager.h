/** \file
 * OpenQL pass manager implementation.
 */

#pragma once

#include "utils/str.h"
#include "utils/list.h"
#include "passes.h"
#include "program.h"

namespace ql {

/**
 * Pass manager class that contains all compiler passes to be executed
 */
class PassManager {
public:
    PassManager(const utils::Str &n);
    PassManager(const utils::Str &n, const utils::Str &cfg);

    void compile(quantum_program *program) const;
    void addPassNamed(const utils::Str &realPassName, const utils::Str &symbolicPassName);
    static AbstractPass *createPass(const utils::Str &passName, const utils::Str &aliasName);
    AbstractPass *findPass(const utils::Str &passName);
    void setPassOptionAll(const utils::Str &optionName, const utils::Str &optionValue);
    void loadPassesFromConfigFile(const utils::Str &name, const utils::Str &cfg);

private:
    void addPass(AbstractPass *pass);

    utils::Str name;
    utils::Str cfg_file_name;
    utils::List<AbstractPass*> passes;
};

} // namespace ql
