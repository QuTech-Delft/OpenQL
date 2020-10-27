/**
 * @file   passmanager.h
 * @date   04/2020
 * @author Razvan Nane
 * @brief  OpenQL Pass Manager
 */

#pragma once

#include "passes.h"
#include "program.h"
#include <string>
#include <list>

namespace ql {

/**
 * Pass manager class that contains all compiler passes to be executed
 */
class PassManager {
public:
    PassManager(const std::string &n);

    void compile(ql::quantum_program *program) const;
    void addPassNamed(const std::string &realPassName, const std::string &symbolicPassName);
    static AbstractPass *createPass(const std::string &passName, const std::string &aliasName);
    AbstractPass *findPass(const std::string &passName);
    void setPassOptionAll(const std::string &optionName, const std::string &optionValue);

private:
    void addPass(AbstractPass *pass);

    std::string name;
    std::list<AbstractPass*> passes;
};

} // namespace ql
