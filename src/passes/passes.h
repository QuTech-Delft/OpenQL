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
    virtual void runOnProgram(const ir::ProgramRef &program) = 0;

    explicit AbstractPass(const utils::Str &name);
    utils::Str getPassName() const;
    void setPassName(const utils::Str &name);
    void setPassOption(const utils::Str &optionName, const utils::Str &optionValue);
    utils::Options &getPassOptions();
    const utils::Options &getPassOptions() const;
    utils::Bool getSkip() const;
    void initPass(const ir::ProgramRef &program);
    void finalizePass(const ir::ProgramRef &program);
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
    void runOnProgram(const ir::ProgramRef &program) override;
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
    void runOnProgram(const ir::ProgramRef &program) override;
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
    void runOnProgram(const ir::ProgramRef &program) override;
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
    void runOnProgram(const ir::ProgramRef &program) override;
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
    void runOnProgram(const ir::ProgramRef &program) override;
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
    void runOnProgram(const ir::ProgramRef &program) override;
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
    void runOnProgram(const ir::ProgramRef &program) override;
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
    void runOnProgram(const ir::ProgramRef &program) override;
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
    void runOnProgram(const ir::ProgramRef &program) override;
};

} // namespace ql
