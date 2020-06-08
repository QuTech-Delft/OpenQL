/**
 * @file   passmanager.h
 * @date   04/2020
 * @author Razvan Nane
 * @brief  OpenQL Pass Manager
 */

#ifndef QL_PASSMANAGER_H
#define QL_PASSMANAGER_H

#include "passes.h"
#include "program.h"

namespace ql
{

/**
 * Pass manager class that contains all compiler passes to be executed
 */
class PassManager
{
public:
    PassManager(std::string n);

    void compile(ql::quantum_program*);
    void addPassNamed(std::string realPassName, std::string symbolicPassName);
    AbstractPass* createPass(std::string passName, std::string aliasName);
    AbstractPass* findPass(std::string passName);
    void setPassOptionAll(std::string optionName, std::string optionValue);

private: 
    void addPass (AbstractPass *pass);
    
    std::string           name;
    std::list <class AbstractPass*> passes;

};

} // ql

#endif //QL_PASSMANAGER_H
