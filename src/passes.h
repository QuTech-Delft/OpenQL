/**
 * @file   passes.h
 * @date   04/2020
 * @author Razvan Nane
 * @brief  OpenQL Passes
 */

#pragma once

#include "program.h"

#include <CLI/CLI.hpp>

namespace ql {

class PassOptions;

/**
 * Compiler Pass Interface 
 */
class AbstractPass {
public:
    virtual void runOnProgram(ql::quantum_program *program) = 0;

    explicit AbstractPass(const std::string &name);
    std::string getPassName() const;
    void setPassName(const std::string &name);
    void setPassOption(const std::string &optionName, const std::string &optionValue);
    PassOptions *getPassOptions();
    const PassOptions *getPassOptions() const;
    void createPassOptions();
    bool getSkip() const;
    void initPass(ql::quantum_program *program);
    void finalizePass(ql::quantum_program *program);
    void appendStatistics(const std::string &statistic);
    std::string getPassStatistics() const;
    void resetStatistics();
    
private:
    std::string passName;
    std::string statistics;
    PassOptions *passOptions;
};

/**
 * Program Reader Pass 
 */
class ReaderPass : public AbstractPass {
public:
    /**
     * @brief  Reader pass constructor
     * @param  Name of the read pass
     */
    explicit ReaderPass(const std::string &name);
    void runOnProgram(ql::quantum_program *program) override;
};

/**
 * Program Writer Pass 
 */
class WriterPass : public AbstractPass {
public:
    /**
     * @brief  Writer pass constructor
     * @param  Name of the read pass
     */
    explicit WriterPass(const std::string &name);
    void runOnProgram(ql::quantum_program *program) override;
};

/**
 * Optimizer Pass 
 */
class RotationOptimizerPass : public AbstractPass {
public:
    /**
     * @brief  Rotation optimizer pass constructor
     * @param  Name of the optimized pass
     */
    explicit RotationOptimizerPass(const std::string &name);
    void runOnProgram(ql::quantum_program *program) override;
};

/**
 * Decompose Toffoli Pass 
 */
class DecomposeToffoliPass : public AbstractPass {
public:
    /**
     * @brief  Rotation optimizer pass constructor
     * @param  Name of the optimized pass
     */
    explicit DecomposeToffoliPass(const std::string &name);
    void runOnProgram(ql::quantum_program *program) override;
};

/**
 * Scheduler Pass 
 */
class SchedulerPass : public AbstractPass {
public:
    /**
     * @brief  Scheduler pass constructor
     * @param  Name of the scheduler pass
     */
    explicit SchedulerPass(const std::string &name);
    void runOnProgram(ql::quantum_program *program) override;
};

/**
 * Backend Compiler Pass 
 */
class BackendCompilerPass : public AbstractPass {
public:
    /**
     * @brief  Scheduler pass constructor
     * @param  Name of the scheduler pass
     */
    explicit BackendCompilerPass(const std::string &name);
    void runOnProgram(ql::quantum_program *program) override;
};

/**
 * Report Statistics Pass 
 */
class ReportStatisticsPass: public AbstractPass {
public:
    /**
     * @brief  Statistics pass constructor
     * @param  Name of the scheduler pass
     */
    explicit ReportStatisticsPass(const std::string &name);
    void runOnProgram(ql::quantum_program *program) override;
};

/**
 * CC-Light Prepare Backend Code Generation Pass 
 */
class CCLPrepCodeGeneration : public AbstractPass {
public:
    /**
     * @brief  CCL Preparation for Code Generation pass constructor
     * @param  Name of the preparation pass
     */
    explicit CCLPrepCodeGeneration(const std::string &name);
    void runOnProgram(ql::quantum_program *program) override;
};

/**
 * CC-Light Prescheduler Decompose Pass 
 */
class CCLDecomposePreSchedule : public AbstractPass {
public:
    /**
     * @brief  CCL Decompose PreSchedule pass constructor
     * @param  Name of the decomposer pass
     */
    explicit CCLDecomposePreSchedule(const std::string &name);
    void runOnProgram(ql::quantum_program *program) override;
};

/**
 * Mapper Pass 
 */
class MapPass : public AbstractPass {
public:
    /**
     * @brief  Mapper pass constructor
     * @param  Name of the mapper pass
     */
    explicit MapPass(const std::string &name);
    void runOnProgram(ql::quantum_program *program) override;
};

/**
 * Clifford Optimizer Pass 
 */
class CliffordOptimizePass : public AbstractPass {
public:
    /**
     * @brief  Clifford Optimize pass constructor
     * @param  Name of the optimizer pass (premapper or postmapper)
     */
    explicit CliffordOptimizePass(const std::string &name);
    void runOnProgram(ql::quantum_program *program) override;
};

/**
 * Resource Constraint Scheduler Pass 
 */
class RCSchedulePass : public AbstractPass {
public:
    /**
     * @brief  Resource Constraint Scheduler pass constructor
     * @param  Name of the scheduler pass
     */
    explicit RCSchedulePass(const std::string &name);
    void runOnProgram(ql::quantum_program *program) override;
};

/**
 * Latency Compensation Pass
 */
class LatencyCompensationPass : public AbstractPass {
public:
    /**
     * @brief  Latency compensation pass constructor
     * @param  Name of the latency compensation pass
     */
    explicit LatencyCompensationPass(const std::string &name);
    void runOnProgram(ql::quantum_program *program) override;
};

/**
 * Insert Buffer Delays Pass 
 */
class InsertBufferDelaysPass : public AbstractPass {
public:
    /**
     * @brief  Insert Buffer Delays pass  constructor
     * @param  Name of the buffer delay insertion pass
     */
    explicit InsertBufferDelaysPass(const std::string &name);
    void runOnProgram(ql::quantum_program *program) override;
};

/**
 * CC-Light Decompose PostSchedule Pass
 */
class CCLDecomposePostSchedulePass : public AbstractPass {
public:
    /**
     * @brief  Decomposer Post Schedule  Pass
     * @param  Name of the decomposer pass
     */
    explicit CCLDecomposePostSchedulePass(const std::string &name);
    void runOnProgram(ql::quantum_program *program) override;
};

/**
 * Write QuantumSim Program Pass
 */
class WriteQuantumSimPass : public AbstractPass {
public:
    /**
     * @brief  QuantumSim Writer Pass constructor
     * @param  Name of the writer pass
     */
    explicit WriteQuantumSimPass(const std::string &name);
    void runOnProgram(ql::quantum_program *program) override;
};

/**
 * QISA Generation Pass 
 */
class QisaCodeGenerationPass : public AbstractPass {
public:
    /**
     * @brief  QISA generation pass constructor
     * @param  Name of the QISA generator pass
     */
    explicit QisaCodeGenerationPass(const std::string &name);
    void runOnProgram(ql::quantum_program *program) override;
};

/**
 * Pass Options Class
 */
class PassOptions {
public:
      explicit PassOptions(std::string app_name="passOpts");
      void print_current_values() const;
      void help() const;
      void setOption(const std::string &opt_name, const std::string &opt_value);
      std::string getOption(const std::string &opt_name) const;
      
private:
      CLI::App * app;
      std::map<std::string, std::string> opt_name2opt_val;
};

} // namespace ql
