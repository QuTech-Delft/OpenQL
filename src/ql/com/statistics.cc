/** \file
 * Utility functions for dumping statistics to a stream.
 */

#include "ql/com/statistics.h"

#include "ql/com/metrics.h"

namespace ql {
namespace com {
namespace statistics {

/**
 * Dumps basic statistics for the given kernel to the given output stream.
 */
void dump(
    const ir::KernelRef &kernel,
    std::ostream &os,
    const utils::Str &line_prefix
) {
    using namespace metrics;

    os << line_prefix << "kernel: " << kernel->name << "\n";
    os << line_prefix << "----- circuit_latency: " << compute<Latency>(kernel) << "\n";
    os << line_prefix << "----- quantum gates: " << compute<QuantumGateCount>(kernel) << "\n";
    os << line_prefix << "----- non single qubit gates: " << compute<MultiQubitGateCount>(kernel) << "\n";
    os << line_prefix << "----- classical operations: " << compute<ClassicalOperationCount>(kernel) << "\n";
    os << line_prefix << "----- qubits used: " << compute<QubitUsageCount>(kernel).sparse_size() << "\n";
    os << line_prefix << "----- qubit cycles use:" << compute<QubitUsedCycleCount>(kernel) << "\n";
    for (const auto &line : kernel->statistics) {
        os << line_prefix << "----- " << line << "\n";
    }
    os.flush();
}

/**
 * Dumps basic statistics for the given program to the given output stream. This
 * only dumps the global statistics, not the statistics for each individual
 * kernel.
 */
void dump(
    const ir::ProgramRef &program,
    std::ostream &os,
    const utils::Str &line_prefix
) {
    using namespace com::metrics;

    os << line_prefix << "Total circuit_latency: " << compute<Latency>(program) << "\n";
    os << line_prefix << "Total no. of quantum gates: " << compute<QuantumGateCount>(program) << "\n";
    os << line_prefix << "Total no. of non single qubit gates: " << compute<MultiQubitGateCount>(program) << "\n";
    os << line_prefix << "Total no. of classical operations: " << compute<ClassicalOperationCount>(program) << "\n";
    os << line_prefix << "Qubits used: " << compute<QubitUsageCount>(program).sparse_size() << "\n";
    os << line_prefix << "No. kernels: " << compute<QubitUsedCycleCount>(program) << "\n";
    for (const auto &line : program->statistics) {
        os << line_prefix << line << "\n";
    }
    os.flush();
}

} // namespace statistics
} // namespace com
} // namespace ql
