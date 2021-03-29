/** \file
 * OpenQL Passes.
 */

#pragma once

#include "ql/utils/str.h"
#include "ql/utils/map.h"
#include "ql/utils/options.h"
#include "ql/ir/ir.h"

namespace ql {

/**
 * Compiler Pass Interface
 */
class AbstractPass {
public:
    virtual void runOnProgram(ir::Program &program) = 0;

    explicit AbstractPass(const utils::Str &name);
    utils::Str getPassName() const;
    void setPassName(const utils::Str &name);
    void setPassOption(const utils::Str &optionName, const utils::Str &optionValue);
    utils::Options &getPassOptions();
    const utils::Options &getPassOptions() const;
    utils::Bool getSkip() const;
    void initPass(ir::Program &program);
    void finalizePass(ir::Program &program);
    void appendStatistics(const utils::Str &statistic);
    utils::Str getPassStatistics() const;
    void resetStatistics();

private:
    utils::Str passName;
    utils::Str statistics;
    utils::Options passOptions;
};

/**
 * Program Reader Pass
 */
class CQasmReaderPass : public AbstractPass {
public:
    /**
     * @brief  Reader pass constructor
     * @param  Name of the read pass
     */
    explicit CQasmReaderPass(const utils::Str &name);
    void runOnProgram(ir::Program &program) override;
};

/**
 * Program Writer Pass
 */
class CQasmWriterPass : public AbstractPass {
public:
    /**
     * @brief  Writer pass constructor
     * @param  Name of the read pass
     */
    explicit CQasmWriterPass(const utils::Str &name);
    void runOnProgram(ir::Program &program) override;
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
    void runOnProgram(ir::Program &program) override;
};

/**
 * Decompose Toffoli Pass
 */
class ToffoliDecomposerPass : public AbstractPass {
public:
    /**
     * @brief  Rotation optimizer pass constructor
     * @param  Name of the optimized pass
     */
    explicit ToffoliDecomposerPass(const utils::Str &name);
    void runOnProgram(ir::Program &program) override;
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
    void runOnProgram(ir::Program &program) override;
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
    void runOnProgram(ir::Program &program) override;
};

/**
 * Report Statistics Pass
 */
class StatisticsReporterPass : public AbstractPass {
public:
    /**
     * @brief  Statistics pass constructor
     * @param  Name of the scheduler pass
     */
    explicit StatisticsReporterPass(const utils::Str &name);
    void runOnProgram(ir::Program &program) override;
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
    void runOnProgram(ir::Program &program) override;
};

/**
 * CC-Light Prepare Backend Code Generation Pass
 */
class CCLConsistencyCheckerPass : public AbstractPass {
public:
    /**
     * @brief  CCL Preparation for Code Generation pass constructor
     * @param  Name of the preparation pass
     */
    explicit CCLConsistencyCheckerPass(const utils::Str &name);
    void runOnProgram(ir::Program &program) override;
};

/**
 * CC-Light Prescheduler Decompose Pass
 */
class CCLPreScheduleDecomposer : public AbstractPass {
public:
    /**
     * @brief  CCL Decompose PreSchedule pass constructor
     * @param  Name of the decomposer pass
     */
    explicit CCLPreScheduleDecomposer(const utils::Str &name);
    void runOnProgram(ir::Program &program) override;
};

/**
 * Mapper Pass
 */
class MapperPass : public AbstractPass {
public:
    /**
     * @brief  Mapper pass constructor
     * @param  Name of the mapper pass
     */
    explicit MapperPass(const utils::Str &name);
    void runOnProgram(ir::Program &program) override;
};

/**
 * Clifford Optimizer Pass
 */
class CliffordOptimizerPass : public AbstractPass {
public:
    /**
     * @brief  Clifford Optimize pass constructor
     * @param  Name of the optimizer pass (premapper or postmapper)
     */
    explicit CliffordOptimizerPass(const utils::Str &name);
    void runOnProgram(ir::Program &program) override;
};

/**
 * Commute Variation Pass
 */
class CommuteVariationOptimizerPass : public AbstractPass {
public:
    /**
     * @brief  Commute variation pass constructor
     * @param  Name of the commute variation pass
     */
    explicit CommuteVariationOptimizerPass(const utils::Str &name);
    void runOnProgram(ir::Program &program) override;
};

/**
 * Resource Constraint Scheduler Pass
 */
class RCSchedulerPass : public AbstractPass {
public:
    /**
     * @brief  Resource Constraint Scheduler pass constructor
     * @param  Name of the scheduler pass
     */
    explicit RCSchedulerPass(const utils::Str &name);
    void runOnProgram(ir::Program &program) override;
};

/**
 * Latency Compensation Pass
 */
class LatencyCompensatorPass : public AbstractPass {
public:
    /**
     * @brief  Latency compensation pass constructor
     * @param  Name of the latency compensation pass
     */
    explicit LatencyCompensatorPass(const utils::Str &name);
    void runOnProgram(ir::Program &program) override;
};

/**
 * Insert Buffer Delays Pass
 */
class BufferDelayInserterPass : public AbstractPass {
public:
    /**
     * @brief  Insert Buffer Delays pass  constructor
     * @param  Name of the buffer delay insertion pass
     */
    explicit BufferDelayInserterPass(const utils::Str &name);
    void runOnProgram(ir::Program &program) override;
};

/**
 * CC-Light Decompose PostSchedule Pass
 */
class CCLPostScheduleDecomposerPass : public AbstractPass {
public:
    /**
     * @brief  Decomposer Post Schedule  Pass
     * @param  Name of the decomposer pass
     */
    explicit CCLPostScheduleDecomposerPass(const utils::Str &name);
    void runOnProgram(ir::Program &program) override;
};

/**
 * Write QuantumSim Program Pass
 */
class QuantumSimWriterPass : public AbstractPass {
public:
    /**
     * @brief  QuantumSim Writer Pass constructor
     * @param  Name of the writer pass
     */
    explicit QuantumSimWriterPass(const utils::Str &name);
    void runOnProgram(ir::Program &program) override;
};

/**
 * QISA Generation Pass
 */
class CCLCodeGeneratorPass : public AbstractPass {
public:
    /**
     * @brief  QISA generation pass constructor
     * @param  Name of the QISA generator pass
     */
    explicit CCLCodeGeneratorPass(const utils::Str &name);
    void runOnProgram(ir::Program &program) override;
};

/**
 * C Printer Pass
 */
class CPrinterPass : public AbstractPass {
public:
    /**
     * @brief  C Printer pass constructor
     * @param  Name of the CPrinter pass
     */
    explicit CPrinterPass(const utils::Str &name);
    void runOnProgram(ir::Program &program) override;
};

/**
 * External C Compiler Pass
 */
class RunExternalCompiler : public AbstractPass {
public:
    /**
     * @brief  External C compiler pass constructor
     * @param  Name of the pass
     */
    explicit RunExternalCompiler(const utils::Str &name);
    void runOnProgram(ir::Program &program) override;
};

} // namespace ql
