/**
 * @file   passes.h
 * @date   04/2020
 * @author Razvan Nane
 * @brief  OpenQL Passes
 */

#ifndef QL_PASSES_H
#define QL_PASSES_H

#include "program.h"

namespace ql
{

/**
 * Compiler Pass Interface 
 */
class AbstractPass
{
public:
    virtual void runOnProgram(ql::quantum_program *program){};
    
    std::string  getPassName();
    void setPassName(std::string name);
    
private:
    std::string      passName;

};

/**
 * Program Reader Pass 
 */
class ReadPass: public AbstractPass 
{
public:
    ReadPass(std::string name);
    
    void runOnProgram(ql::quantum_program *program);
};

/**
 * Optimizer Pass 
 */
class OptimizePass: public AbstractPass 
{
public:
    OptimizePass(std::string name);
    
    void runOnProgram(ql::quantum_program *program);
};

/**
 * Scheduler Pass 
 */
class SchedulerPass: public AbstractPass 
{
public:
    SchedulerPass(std::string name);

    void runOnProgram(ql::quantum_program *program);
};
    
} // ql

#endif //QL_PASSES_H
