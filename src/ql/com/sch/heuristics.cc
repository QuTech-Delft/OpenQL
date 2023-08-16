/** \file
 * Defines basic criticality heuristics for the list scheduler.
 */

#include "ql/com/sch/heuristics.h"

#include "ql/com/ddg/ops.h"

namespace ql {
namespace com {
namespace sch {

/**
 * Comparator implementation for TrivialHeuristic.
 */
utils::Bool TrivialHeuristic::operator()(
    const ir::StatementRef &,
    const ir::StatementRef &
) const {
    return false;
}

/**
 * String representation for TrivialHeuristic.
 */
utils::Str TrivialHeuristic::operator()(
    const ir::StatementRef &
) const {
    return "-";
}

/**
 * Comparator implementation for CriticalPathHeuristic.
 */
utils::Bool CriticalPathHeuristic::operator()(
    const ir::StatementRef &lhs,
    const ir::StatementRef &rhs
) const {
    return utils::abs(lhs->cycle) < utils::abs(rhs->cycle);
}

/**
 * String representation for CriticalPathHeuristic.
 */
utils::Str CriticalPathHeuristic::operator()(
    const ir::StatementRef &val
) const {
    return utils::to_string(utils::abs(val->cycle));
}

/**
 * Returns the Criticality annotation for the given statement, or returns
 * zero criticality if no statement exist.
 */
const DeepCriticality &DeepCriticality::get(const ir::StatementRef &statement) {
    if (auto ptr = statement->get_annotation_ptr<DeepCriticality>()) {
        return *ptr;
    } else {
        static const DeepCriticality EMPTY{};
        return EMPTY;
    }
}

/**
 * Compares the criticality of two Criticality annotations.
 */
utils::Bool DeepCriticality::operator<(const DeepCriticality &other) const {

    // The node with the largest shallow criticality wins.
    if (critical_path_length > other.critical_path_length) return false;
    if (critical_path_length < other.critical_path_length) return true;

    // The nodes have equal shallow criticality. The one with dependent
    // nodes wins.
    if (other.most_critical_dependent.empty()) return false;
    if (most_critical_dependent.empty()) return true;

    // Both nodes have dependent nodes. Recursively check their
    // criticality.
    return get(most_critical_dependent) < get(other.most_critical_dependent);

}

/**
 * String conversion for DeepCriticality.
 */
std::ostream &operator<<(std::ostream &os, const DeepCriticality &dc) {
    os << dc.critical_path_length;
    if (!dc.most_critical_dependent.empty()) {
        os << ", " << DeepCriticality::get(dc.most_critical_dependent);
    }
    return os;
}

/**
 * Ensures that a valid criticality annotation exists for the given
 * statement. This will recursively ensure that dependent statements are
 * annotated, because this is needed to compute which of the dependent
 * statements is the most critical for deep criticality. The set tracks
 * which statements have valid annotations (there may be stray annotations
 * from previous scheduling operations that we must be sure to override).
 */
void DeepCriticality::ensure_annotation(
    const ir::StatementRef &statement,
    utils::Set<ir::StatementRef> &annotated
) {

    // If insertion into the set succeeds, we haven't annotated this
    // statement yet.
    if (annotated.insert(statement).second) {
        DeepCriticality criticality;

        // Determine the critical path length for shallow criticality.
        // Because the schedule used to determine criticality is constructed
        // in reverse order from the list scheduler it is intended for,
        // instructions that could be scheduled quickly have lower
        // criticality. So, the criticality of an instruction is simply its
        // distance from the source node of the reversed DDG, which is 0 by
        // definition before the cycles adjusted, so this is just the
        // absolute value.
        criticality.critical_path_length = utils::abs(statement->cycle);

        // Find the most critical dependent statement for the given
        // scheduling direction.
        for (const auto &dependent : com::ddg::get_node(statement)->successors) {
            const auto &dependent_stmt = dependent.first;

            // Make sure the dependent statement has a criticality
            // annotation already.
            ensure_annotation(dependent_stmt, annotated);

            // If the dependent statement is more critical than the most
            // critical dependent found thus far, replace it.
            if (
                criticality.most_critical_dependent.empty() ||
                DeepCriticality::Heuristic()(criticality.most_critical_dependent, dependent_stmt)
            ) {
                criticality.most_critical_dependent = dependent_stmt;
            }

        }

        // Attach the annotation.
        statement->set_annotation<DeepCriticality>(criticality);

    }

    // There must now be a criticality annotation.
    QL_ASSERT(statement->has_annotation<DeepCriticality>());

}

/**
 * Annotates the instructions in block with DeepCriticality structures, such
 * that DeepCriticality::Heuristic() can be used as scheduling heuristic.
 * This requires that a data dependency graph has already been constructed
 * for the block, and that the block has already been scheduled in the
 * reverse direction of the desired list scheduling direction, with cycle
 * numbers still referenced such that the source node is at cycle 0.
 */
void DeepCriticality::compute(const ir::SubBlockRef &block) {

    // Tracks which statements have already been annotated by *this call*
    // (we can't just check whether the annotation already exists, because
    // it could be an out-of-date annotation added by an earlier call).
    utils::Set<ir::StatementRef> annotated;

    // Annotate all the statements in the block. The order doesn't matter:
    // when a dependent statement doesn't yet have the criticality
    // annotation needed to determine deep criticality, it will be computed
    // automatically using recursion, and if criticality has already been
    // computed the function becomes no-op.
    for (const auto &statement : block->statements) {
        ensure_annotation(statement, annotated);
    }

}

/**
 * Compares the criticality of two statements by means of their Criticality
 * annotation.
 */
utils::Bool DeepCriticality::Heuristic::operator()(
    const ir::StatementRef &lhs,
    const ir::StatementRef &rhs
) const {
    return get(lhs) < get(rhs);
}

/**
 * String representation for DeepCriticality::Heuristic.
 */
utils::Str DeepCriticality::Heuristic::operator()(
    const ir::StatementRef &val
) const {
    return utils::to_string(get(val));
}

/**
 * Clears the deep criticality annotations from the given block.
 */
void DeepCriticality::clear(const ir::SubBlockRef &block) {
    auto source = com::ddg::get_source(block);
    if (!source.empty()) source->erase_annotation<DeepCriticality>();
    auto sink = com::ddg::get_source(block);
    if (!sink.empty()) sink->erase_annotation<DeepCriticality>();
    for (const auto &statement : block->statements) {
        statement->erase_annotation<DeepCriticality>();
    }
}

} // namespace sch
} // namespace com
} // namespace ql
