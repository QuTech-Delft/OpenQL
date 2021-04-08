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
namespace metrics {

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
     * Updates the metric using the given kernel. Default implementation throws
     * an unimplemented exception.
     */
    virtual void process_gate(const ir::GateRef &gate) {
        throw utils::Exception("metric is not implemented for gates");
    }

    /**
     * Updates the metric using the given kernel. Default implementation just
     * calls process_gate for each contained gate.
     */
    virtual void process_kernel(const ir::KernelRef &kernel) {
        for (const auto &gate : kernel->c) {
            process_gate(gate);
        }
    }

    /**
     * Updates the metric using the given program. Default implementation just
     * calls process_kernel for each contained kernel.
     */
    virtual void process_program(const ir::ProgramRef &program) {
        for (const auto &kernel : program->kernels) {
            process_kernel(kernel);
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
typename M::ReturnType compute(const ir::GateRef &gate) {
    M metric;
    metric.process_gate(gate);
    return metric.get_result();
}

/**
 * Computes the given metric for the given kernel.
 */
template <class M>
typename M::ReturnType compute(const ir::KernelRef &kernel) {
    M metric;
    metric.process_kernel(kernel);
    return metric.get_result();
}

/**
 * Computes the given metric for the given program.
 */
template <class M>
typename M::ReturnType compute(const ir::ProgramRef &program) {
    M metric;
    metric.process_program(program);
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
    void process_gate(const ir::GateRef &gate) override;
};

/**
 * A metric that counts the number of quantum gates.
 */
class QuantumGateCount : public SimpleValueMetric<utils::UInt, 0> {
public:
    void process_gate(const ir::GateRef &gate) override;
};

/**
 * A metric that counts the number of multi-qubit quantum gates.
 */
class MultiQubitGateCount : public SimpleValueMetric<utils::UInt, 0> {
public:
    void process_gate(const ir::GateRef &gate) override;
};

/**
 * A metric that counts the number of times each qubit is used.
 */
class QubitUsageCount : public SimpleClassMetric<utils::SparseMap<utils::UInt, utils::UInt, 0>> {
public:
    void process_gate(const ir::GateRef &gate) override;
};

/**
 * A metric that counts the number of cycles each qubit is used for.
 */
class QubitUsedCycleCount : public SimpleClassMetric<utils::SparseMap<utils::UInt, utils::UInt, 0>> {
public:
    void process_kernel(const ir::KernelRef &kernel) override;
};

/**
 * A metric that returns the duration of a scheduled kernel in cycles.
 */
class Latency : public SimpleValueMetric<utils::UInt, 0> {
public:
    void process_kernel(const ir::KernelRef &kernel) override;
};

} // namespace metrics
} // namespace com
} // namespace ql
