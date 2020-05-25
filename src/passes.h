/**
 * @file   passes.h
 * @date   04/2020
 * @author Razvan Nane
 * @brief  OpenQL Passes
 */

#ifndef QL_PASSES_H
#define QL_PASSES_H

#include "program.h"

#include <CLI/CLI.hpp>

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
    void setPassOption(std::string optionName, std::string optionValue);
    void createPassOptions();
    
private:
    std::string        passName;
    class PassOptions *passOptions;
};

/**
 * Program Reader Pass 
 */
class ReaderPass: public AbstractPass 
{
public:
    ReaderPass(std::string name);
    
    void runOnProgram(ql::quantum_program *program);
    
};

/**
 * Program Writer Pass 
 */
class WriterPass: public AbstractPass 
{
public:
    WriterPass(std::string name);
    
    void runOnProgram(ql::quantum_program *program);
};

/**
 * Optimizer Pass 
 */
class OptimizerPass: public AbstractPass 
{
public:
    OptimizerPass(std::string name);
    
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


/**
 * Pass Options Class
 */
class PassOptions
{
public:
      PassOptions(std::string app_name);
      void print_current_values();
      void help();
      void set(std::string opt_name, std::string opt_value);
      std::string get(std::string opt_name);
      
private:
      CLI::App * app;
      std::map<std::string, std::string> opt_name2opt_val;
  };

    
} // ql

#endif //QL_PASSES_H
