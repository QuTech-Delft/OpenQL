/** \file
 * Defines the statistics reporting pass.
 */

#include "ql/ir/dump.h"

#include "ql/utils/filesystem.h"
#include "ql/com/ana/metrics.h"
#include "ql/ir/annotations.h"

namespace ql {
namespace ir {

/**
 * Dumps basic statistics for the given kernel to the given output stream.
 */
void dump(
    const ir::Ref &ir,
    const ir::BlockRef &block,
    std::ostream &os,
    const utils::Str &line_prefix
) {
    using namespace com::ana;

    os << line_prefix << "Duration (assuming no control-flow): " << compute_block<Latency>(ir, block) << "\n";
    os << line_prefix << "Number of quantum gates: " << compute_block<QuantumGateCount>(ir, block) << "\n";
    os << line_prefix << "Number of multi-qubit gates: " << compute_block<MultiQubitGateCount>(ir, block) << "\n";
    os << line_prefix << "Number of classical operations: " << compute_block<ClassicalOperationCount>(ir, block) << "\n";
    os << line_prefix << "Number of qubits used: " << compute_block<QubitUsageCount>(ir, block).sparse_size() << "\n";
    os << line_prefix << "Qubit cycles use (assuming no control-flow): " << compute_block<QubitUsedCycleCount>(ir, block) << "\n";
    for (const auto &line : AdditionalStats::pop(block)) {
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
    const ir::Ref &ir,
    const ir::ProgramRef &program,
    std::ostream &os,
    const utils::Str &line_prefix
) {
    using namespace com::ana;

    os << line_prefix << "Total duration (assuming no control-flow): " << compute_program<Latency>(ir) << "\n";
    os << line_prefix << "Total number of quantum gates: " << compute_program<QuantumGateCount>(ir) << "\n";
    os << line_prefix << "Total number of multi-qubit gates: " << compute_program<MultiQubitGateCount>(ir) << "\n";
    os << line_prefix << "Total number of classical operations: " << compute_program<ClassicalOperationCount>(ir) << "\n";
    os << line_prefix << "Number of qubits used: " << compute_program<QubitUsageCount>(ir).sparse_size() << "\n";
    os << line_prefix << "Qubit cycles use (assuming no control-flow): " << compute_program<QubitUsedCycleCount>(ir) << "\n";
    for (const auto &line : AdditionalStats::pop(program)) {
        os << line_prefix << line << "\n";
    }
    os.flush();
}

/**
 * Dumps statistics for the given program and its kernels to the given output
 * stream.
 */
void dump_all(
    const ir::Ref &ir,
    std::ostream &os,
    const utils::Str &line_prefix
) {
    if (ir->program.empty()) {
        os << line_prefix << "no program node to dump statistics for" << std::endl;
    } else {
        for (const auto &block : ir->program->blocks) {
            os << line_prefix << "For block with name \"" << block->name << "\":\n";
            dump(ir, block, os, line_prefix + "    ");
            os << "\n";
        }
        os << line_prefix << "Global statistics:\n";
        dump(ir, ir->program, os, line_prefix);
    }
}

} // namespace ir
} // namespace ql
