/** \file
 * OpenQL Passes.
 */

#pragma once

#include "utils/str.h"
#include "utils/map.h"
#include "options.h"
#include "program.h"

namespace ql {

/**
 * Compiler Pass Interface
 */
class AbstractPass {
public:
    virtual void runOnProgram(quantum_program *program) = 0;

    explicit AbstractPass(const utils::Str &name);
    utils::Str getPassName() const;
    void setPassName(const utils::Str &name);
    void setPassOption(const utils::Str &optionName, const utils::Str &optionValue);
    options::Options &getPassOptions();
    const options::Options &getPassOptions() const;
    utils::Bool getSkip() const;
    void initPass(quantum_program *program);
    void finalizePass(quantum_program *program);
    void appendStatistics(const utils::Str &statistic);
    utils::Str getPassStatistics() const;
    void resetStatistics();

private:
    utils::Str passName;
    utils::Str statistics;
    options::Options passOptions;
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
    explicit ReaderPass(const utils::Str &name);
    void runOnProgram(quantum_program *program) override;
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
    explicit WriterPass(const utils::Str &name);
    void runOnProgram(quantum_program *program) override;
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
    explicit RotationOptimizerPass(const utils::Str &name);
    void runOnProgram(quantum_program *program) override;
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
    explicit DecomposeToffoliPass(const utils::Str &name);
    void runOnProgram(quantum_program *program) override;
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
    explicit SchedulerPass(const utils::Str &name);
    void runOnProgram(quantum_program *program) override;
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
    explicit BackendCompilerPass(const utils::Str &name);
    void runOnProgram(quantum_program *program) override;
};

/**
 * Report Statistics Pass
 */
class ReportStatisticsPass : public AbstractPass {
public:
    /**
     * @brief  Statistics pass constructor
     * @param  Name of the scheduler pass
     */
    explicit ReportStatisticsPass(const utils::Str &name);
    void runOnProgram(quantum_program *program) override;
};

/**
 * Visualizer Pass
 */
class VisualizerPass : public AbstractPass  {
public:
    /**
     * @brief  Visualizer pass constructor
     * @param  Name of the visualizer pass
     */
    explicit VisualizerPass(const utils::Str &name);
    void runOnProgram(quantum_program *program) override;
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
    explicit CCLPrepCodeGeneration(const utils::Str &name);
    void runOnProgram(quantum_program *program) override;
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
    explicit CCLDecomposePreSchedule(const utils::Str &name);
    void runOnProgram(quantum_program *program) override;
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
    explicit MapPass(const utils::Str &name);
    void runOnProgram(quantum_program *program) override;
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
    explicit CliffordOptimizePass(const utils::Str &name);
    void runOnProgram(quantum_program *program) override;
};

/**
 * Commute Variation Pass
 */
class CommuteVariationPass : public AbstractPass {
public:
    /**
     * @brief  Commute variation pass constructor
     * @param  Name of the commute variation pass
     */
    explicit CommuteVariationPass(const utils::Str &name);
    void runOnProgram(quantum_program *program) override;
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
    explicit RCSchedulePass(const utils::Str &name);
    void runOnProgram(quantum_program *program) override;
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
    explicit LatencyCompensationPass(const utils::Str &name);
    void runOnProgram(quantum_program *program) override;
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
    explicit InsertBufferDelaysPass(const utils::Str &name);
    void runOnProgram(quantum_program *program) override;
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
    explicit CCLDecomposePostSchedulePass(const utils::Str &name);
    void runOnProgram(quantum_program *program) override;
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
    explicit WriteQuantumSimPass(const utils::Str &name);
    void runOnProgram(quantum_program *program) override;
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
    explicit QisaCodeGenerationPass(const utils::Str &name);
    void runOnProgram(quantum_program *program) override;
};

} // namespace ql
