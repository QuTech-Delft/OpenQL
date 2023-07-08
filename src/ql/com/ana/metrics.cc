/** \file
 * Utility functions for extracting statistics/metrics from programs and
 * kernels.
 */

#include "ql/com/ana/metrics.h"

#include "ql/ir/ops.h"

namespace ql {
namespace com {
namespace ana {

/**
 * Classical operation counting metric.
 */
void ClassicalOperationCount::process_instruction(
    const ir::Ref &ir,
    const ir::InstructionRef &instruction
) {
    if (instruction->as_set_instruction() || instruction->as_goto_instruction()) {
        value++;
    }
}

/**
 * Quantum gate counting metric.
 */
void QuantumGateCount::process_instruction(
    const ir::Ref &ir,
    const ir::InstructionRef &instruction
) {
    if (ir::get_number_of_qubits_involved(instruction)) {
        value++;
    }
}

/**
 * Multi-qubit gate counting metric.
 */
void MultiQubitGateCount::process_instruction(
    const ir::Ref &ir,
    const ir::InstructionRef &instruction
) {
    if (ir::get_number_of_qubits_involved(instruction) > 1) {
        value++;
    }
}

/**
 * Qubit usage counting metric.
 */
void QubitUsageCount::process_instruction(
    const ir::Ref &ir,
    const ir::InstructionRef &instruction
) {
    for (auto &op : ir::get_operands(instruction)) {
        if (auto ref = op->as_reference()) {
            if (
                *(ref->target->as_object()) == *(ir->platform->qubits->as_object()) &&
                ref->data_type == ir->platform->qubits->data_type &&
                ref->indices.size() == 1 &&
                ref->indices[0]->as_int_literal()
            ) {
                QL_ASSERT(ref->indices.size() == 1);
                value[ref->indices[0]->as_int_literal()->value]++;
            }
        }
    }
}

/**
 * Qubit cycle usage counting metric.
 */
void QubitUsedCycleCount::process_instruction(
    const ir::Ref &ir,
    const ir::InstructionRef &instruction
) {
    for (auto &op : ir::get_operands(instruction)) {
        if (auto ref = op->as_reference()) {
            if (
                *(ref->target->as_object()) == *(ir->platform->qubits->as_object()) &&
                ref->data_type == ir->platform->qubits->data_type &&
                ref->indices.size() == 1 &&
                ref->indices[0]->as_int_literal()
            ) {
                QL_ASSERT(ref->indices.size() == 1);
                value[ref->indices[0]->as_int_literal()->value] +=
                    get_duration_of_instruction(instruction);
            }
        }
    }
}

/**
 * Returns the duration of a scheduled block in cycles.
 */
void Latency::process_block(const ir::Ref &ir, const ir::BlockBaseRef &block) {
    value = ir::get_duration_of_block(block);
}

} // namespace ana
} // namespace com
} // namespace ql
