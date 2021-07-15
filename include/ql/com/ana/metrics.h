/** \file
 * Utility functions for extracting statistics/metrics from programs and
 * kernels.
 *
 * Usage is for instance com::metrics::compute<ClassicalOperationCount>(kernel).
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/map.h"
#include "ql/utils/exception.h"
#include "ql/ir/ir.h"

namespace ql {
namespace com {
namespace ana {

/**
 * Base class for a metric. T is the type returned when the metric is computed.
 */
template <typename T>
class Metric {
public:

    /**
     * The type returned by get_result().
     */
    using ReturnType = T;

    /**
     * Updates the metric using the given instruction. Default implementation
     * throws an unimplemented exception.
     */
    virtual void process_instruction(
        const ir::Ref &ir,
        const ir::InstructionRef &instruction
    ) {
        throw utils::Exception("metric is not implemented for instructions");
    }

    /**
     * Updates the metric using the given statement. Default implementation
     * recurses into sub-blocks and calls process_instruction for all
     * instructions encountered.
     */
    virtual void process_statement(
        const ir::Ref &ir,
        const ir::StatementRef &statement
    ) {
        if (statement->as_instruction()) {
            process_instruction(ir, statement.as<ir::Instruction>());
        } else if (auto if_else = statement->as_if_else()) {
            for (const auto &branch : if_else->branches) {
                process_block(ir, branch->body);
            }
            if (!if_else->otherwise.empty()) {
                process_block(ir, if_else->otherwise);
            }
        } else if (auto static_loop = statement->as_static_loop()) {
            process_block(ir, static_loop->body);
        } else if (auto for_loop = statement->as_for_loop()) {
            if (!for_loop->initialize.empty()) {
                process_instruction(ir, for_loop->initialize);
            }
            if (!for_loop->update.empty()) {
                process_instruction(ir, for_loop->update);
            }
            process_block(ir, for_loop->body);
        } else if (auto repeat_until = statement->as_repeat_until_loop()) {
            process_block(ir, repeat_until->body);
        } else if (statement->as_loop_control_statement()) {
            // no-op.
        } else {
            QL_ASSERT(false);
        }
    }

    /**
     * Updates the metric using the given block. Default implementation just
     * calls process_statement() for each contained statement.
     */
    virtual void process_block(
        const ir::Ref &ir,
        const ir::BlockBaseRef &block
    ) {
        for (const auto &statement : block->statements) {
            process_statement(ir, statement);
        }
    }

    /**
     * Updates the metric using the given program. Default implementation just
     * calls process_block() for each contained block.
     */
    virtual void process_program(
        const ir::Ref &ir,
        const ir::ProgramRef &program
    ) {
        for (const auto &block : program->blocks) {
            process_block(ir, block);
        }
    }

    /**
     * Virtual destructor.
     */
    virtual ~Metric() = default;

    /**
     * Returns the results gathered thus far.
     */
    virtual T get_result() = 0;

};

/**
 * Computes the given metric for the given gate.
 */
template <class M>
typename M::ReturnType compute_statement(
    const ir::Ref &ir,
    const ir::StatementRef &statement
) {
    M metric;
    metric.process_statement(ir, statement);
    return metric.get_result();
}

/**
 * Computes the given metric for the given block.
 */
template <class M>
typename M::ReturnType compute_block(
    const ir::Ref &ir,
    const ir::BlockBaseRef &block
) {
    M metric;
    metric.process_block(ir, block);
    return metric.get_result();
}

/**
 * Computes the given metric for the given program.
 */
template <class M>
typename M::ReturnType compute_program(const ir::Ref &ir) {
    M metric;
    if (!ir->program.empty()) {
        metric.process_program(ir, ir->program);
    }
    return metric.get_result();
}

/**
 * A metric that just returns a simple C++ primitive value with the given
 * initial value.
 */
template <typename T, T INITIAL_VALUE>
class SimpleValueMetric : public Metric<T> {
protected:

    /**
     * The metric as computed thus far.
     */
    T value = INITIAL_VALUE;

public:

    /**
     * Returns the results gathered thus far.
     */
    T get_result() final {
        return value;
    }

};

/**
 * A metric that just returns a C++ class using the default constructor as the
 * default value.
 */
template <typename T>
class SimpleClassMetric : public Metric<T> {
protected:

    /**
     * The metric as computed thus far.
     */
    T value{};

public:

    /**
     * Returns the results gathered thus far.
     */
    T get_result() final {
        return value;
    }

};

/**
 * A metric that counts the number of classical operations.
 */
class ClassicalOperationCount : public SimpleValueMetric<utils::UInt, 0> {
public:
    void process_instruction(
        const ir::Ref &ir,
        const ir::InstructionRef &instruction
    ) override;
};

/**
 * A metric that counts the number of quantum gates.
 */
class QuantumGateCount : public SimpleValueMetric<utils::UInt, 0> {
public:
    void process_instruction(
        const ir::Ref &ir,
        const ir::InstructionRef &instruction
    ) override;
};

/**
 * A metric that counts the number of multi-qubit quantum gates.
 */
class MultiQubitGateCount : public SimpleValueMetric<utils::UInt, 0> {
public:
    void process_instruction(
        const ir::Ref &ir,
        const ir::InstructionRef &instruction
    ) override;
};

/**
 * A metric that counts the number of times each qubit is used.
 */
class QubitUsageCount : public SimpleClassMetric<utils::SparseMap<utils::UInt, utils::UInt, 0>> {
public:
    void process_instruction(
        const ir::Ref &ir,
        const ir::InstructionRef &instruction
    ) override;
};

/**
 * A metric that counts the number of cycles each qubit is used for.
 */
class QubitUsedCycleCount : public SimpleClassMetric<utils::SparseMap<utils::UInt, utils::UInt, 0>> {
public:
    void process_instruction(
        const ir::Ref &ir,
        const ir::InstructionRef &instruction
    ) override;
};

/**
 * A metric that returns the duration of a scheduled block in cycles.
 */
class Latency : public SimpleValueMetric<utils::UInt, 0> {
public:
    void process_block(
        const ir::Ref &ir,
        const ir::BlockBaseRef &block
    ) override;
};

} // namespace ana
} // namespace com
} // namespace ql
