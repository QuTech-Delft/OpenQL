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
    void resetStatistics() { statistics = "";};
    
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
 * CC-Light Prepare Backend Code Generation Pass 
 */
class CCLPrepCodeGeneration: public AbstractPass 
{
public:
    /**
     * @brief  CCL Preparation for Code Generation pass constructor
     * @param  Name of the preparation pass
     */
    CCLPrepCodeGeneration(std::string name):AbstractPass(name){};

    void runOnProgram(ql::quantum_program *program);
};

/**
 * CC-Light Prescheduler Decompose Pass 
 */
class CCLDecomposePreSchedule: public AbstractPass 
{
public:
    /**
     * @brief  CCL Decompose PreSchedule pass constructor
     * @param  Name of the decomposer pass
     */
    CCLDecomposePreSchedule(std::string name):AbstractPass(name){};

    void runOnProgram(ql::quantum_program *program);
};

/**
 * Mapper Pass 
 */
class MapPass: public AbstractPass 
{
public:
    /**
     * @brief  Mapper pass constructor
     * @param  Name of the mapper pass
     */
    MapPass(std::string name):AbstractPass(name){};

    void runOnProgram(ql::quantum_program *program);
};

/**
 * Clifford Optimizer Pass 
 */
class CliffordOptimizePass: public AbstractPass 
{
public:
    /**
     * @brief  Clifford Optimize pass constructor
     * @param  Name of the optimizer pass (premapper or postmapper)
     */
    CliffordOptimizePass(std::string name):AbstractPass(name){};

    void runOnProgram(ql::quantum_program *program);
};

/**
 * Resource Constraint Scheduler Pass 
 */
class RCSchedulePass: public AbstractPass 
{
public:
    /**
     * @brief  Resource Constraint Scheduler pass constructor
     * @param  Name of the scheduler pass
     */
    RCSchedulePass(std::string name):AbstractPass(name){};

    void runOnProgram(ql::quantum_program *program);
};

/**
 * Latency Compensation Pass
 */
class LatencyCompensationPass: public AbstractPass 
{
public:
    /**
     * @brief  Latency compensation pass constructor
     * @param  Name of the latency compensation pass
     */
    LatencyCompensationPass(std::string name):AbstractPass(name){};

    void runOnProgram(ql::quantum_program *program);
};

/**
 * Insert Buffer Delays Pass 
 */
class InsertBufferDelaysPass: public AbstractPass 
{
public:
    /**
     * @brief  Insert Buffer Delays pass  constructor
     * @param  Name of the buffer delay insertion pass
     */
    InsertBufferDelaysPass(std::string name):AbstractPass(name){};

    void runOnProgram(ql::quantum_program *program);
};

/**
 * CC-Light Decompose PostSchedule Pass
 */
class CCLDecomposePostSchedulePass: public AbstractPass 
{
public:
    /**
     * @brief  Decomposer Post Schedule  Pass
     * @param  Name of the decomposer pass
     */
    CCLDecomposePostSchedulePass(std::string name):AbstractPass(name){};

    void runOnProgram(ql::quantum_program *program);
};

/**
 * Write QuantumSim Program Pass
 */
class WriteQuantumSimPass: public AbstractPass 
{
public:
    /**
     * @brief  QuantumSim Writer Pass constructor
     * @param  Name of the writer pass
     */
    WriteQuantumSimPass(std::string name):AbstractPass(name){};

    void runOnProgram(ql::quantum_program *program);
};

/**
 * QISA Generation Pass 
 */
class QisaCodeGenerationPass: public AbstractPass 
{
public:
    /**
     * @brief  QISA generation pass constructor
     * @param  Name of the QISA generator pass
     */
    QisaCodeGenerationPass(std::string name):AbstractPass(name){};

    void runOnProgram(ql::quantum_program *program);
};

/**
 * Pass Options Class
 */
class PassOptions
{
public:
      PassOptions(std::string app_name="passOpts");
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
