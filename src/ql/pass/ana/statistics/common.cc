/** \file
 * Defines common types and utility functions related to the statistics passes.
 */

#include "ql/pass/ana/statistics/common.h"

#include "ql/com/metrics.h"

namespace ql {
namespace pass {
namespace ana {
namespace statistics {

/**
 * Attaches a statistic to the given node.
 */
static void push_node(
    tree::annotatable::Annotatable &node,
    const utils::Str &line
) {
    if (!node.has_annotation<AdditionalStats>()) {
        node.set_annotation<AdditionalStats>({});
    }
    node.get_annotation<AdditionalStats>().stats.push_back(line);
}

/**
 * Attaches a statistic to the given kernel node.
 */
void AdditionalStats::push(const ir::KernelRef &kernel, const utils::Str &line) {
    push_node(*kernel, line);
}

/**
 * Attaches a statistic to the given program node.
 */
void AdditionalStats::push(const ir::ProgramRef &program, const utils::Str &line) {
    push_node(*program, line);
}

/**
 * Pops all statistics annotations from the given node.
 */
static utils::List<utils::Str> pop_node(tree::annotatable::Annotatable &node) {
    if (auto s = node.get_annotation_ptr<AdditionalStats>()) {
        auto stats = std::move(s->stats);
        node.erase_annotation<AdditionalStats>();
        return stats;
    } else {
        return {};
    }
}

/**
 * Pops all statistics annotations from the given kernel.
 */
utils::List<utils::Str> AdditionalStats::pop(const ir::KernelRef &kernel) {
    return pop_node(*kernel);
}

/**
 * Pops all statistics annotations from the given program.
 */
utils::List<utils::Str> AdditionalStats::pop(const ir::ProgramRef &program) {
    return pop_node(*program);
}

/**
 * Dumps basic statistics for the given kernel to the given output stream.
 */
void dump(
    const ir::KernelRef &kernel,
    std::ostream &os,
    const utils::Str &line_prefix
) {
    using namespace com::metrics;

    os << line_prefix << "kernel: " << kernel->name << "\n";
    os << line_prefix << "----- circuit_latency: " << compute<Latency>(kernel) << "\n";
    os << line_prefix << "----- quantum gates: " << compute<QuantumGateCount>(kernel) << "\n";
    os << line_prefix << "----- non single qubit gates: " << compute<MultiQubitGateCount>(kernel) << "\n";
    os << line_prefix << "----- classical operations: " << compute<ClassicalOperationCount>(kernel) << "\n";
    os << line_prefix << "----- qubits used: " << compute<QubitUsageCount>(kernel).sparse_size() << "\n";
    os << line_prefix << "----- qubit cycles use:" << compute<QubitUsedCycleCount>(kernel) << "\n";
    for (const auto &line : AdditionalStats::pop(kernel)) {
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
    for (const auto &line : AdditionalStats::pop(program)) {
        os << line_prefix << line << "\n";
    }
    os.flush();
}

} // namespace statistics
} // namespace ana
} // namespace pass
} // namespace ql
