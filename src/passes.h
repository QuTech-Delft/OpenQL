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
    
    AbstractPass(std::string name);
    std::string  getPassName();
    void setPassName(std::string name);
    void setPassOption(std::string optionName, std::string optionValue);
    class PassOptions* getPassOptions() {return passOptions;};
    void createPassOptions();
    bool getSkip();
    void initPass(ql::quantum_program *program);
    void finalizePass(ql::quantum_program *program);
    void appendStatistics(std::string statistic);
    std::string getPassStatistics() {return statistics;};
    
private:
    std::string        passName;
    std::string        statistics;
    class PassOptions *passOptions;
};

/**
 * Program Reader Pass 
 */
class ReaderPass: public AbstractPass 
{
public:
    /**
     * @brief  Reader pass constructor
     * @param  Name of the read pass
     */
    ReaderPass(std::string name):AbstractPass(name){};
    
    void runOnProgram(ql::quantum_program *program);
    
};

/**
 * Program Writer Pass 
 */
class WriterPass: public AbstractPass 
{
public:
    /**
     * @brief  Writer pass constructor
     * @param  Name of the read pass
     */
    WriterPass(std::string name):AbstractPass(name){};
    
    void runOnProgram(ql::quantum_program *program);
};

/**
 * Optimizer Pass 
 */
class RotationOptimizerPass: public AbstractPass 
{
public:
    /**
     * @brief  Rotation optimizer pass constructor
     * @param  Name of the optimized pass
     */
    RotationOptimizerPass(std::string name):AbstractPass(name){};
    
    void runOnProgram(ql::quantum_program *program);
};

/**
 * Decompose Toffoli Pass 
 */
class DecomposeToffoliPass: public AbstractPass 
{
public:
    /**
     * @brief  Rotation optimizer pass constructor
     * @param  Name of the optimized pass
     */
    DecomposeToffoliPass(std::string name):AbstractPass(name){};
    
    void runOnProgram(ql::quantum_program *program);
};

/**
 * Scheduler Pass 
 */
class SchedulerPass: public AbstractPass 
{
public:
    /**
     * @brief  Scheduler pass constructor
     * @param  Name of the scheduler pass
     */
    SchedulerPass(std::string name):AbstractPass(name){};

    void runOnProgram(ql::quantum_program *program);
};

/**
 * Backend Compiler Pass 
 */
class BackendCompilerPass: public AbstractPass 
{
public:
    /**
     * @brief  Scheduler pass constructor
     * @param  Name of the scheduler pass
     */
    BackendCompilerPass(std::string name):AbstractPass(name){};

    void runOnProgram(ql::quantum_program *program);
};

/**
 * Report Statistics Pass 
 */
class ReportStatisticsPass: public AbstractPass 
{
public:
    /**
     * @brief  Statistics pass constructor
     * @param  Name of the scheduler pass
     */
    ReportStatisticsPass(std::string name):AbstractPass(name){};

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
      void setOption(std::string opt_name, std::string opt_value);
      std::string getOption(std::string opt_name);
      
private:
      CLI::App * app;
      std::map<std::string, std::string> opt_name2opt_val;
  };

    
} // ql

#endif //QL_PASSES_H
