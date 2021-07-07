/** \file
 * Defines basic criticality heuristics for the list scheduler.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/set.h"
#include "ql/ir/ir.h"

namespace ql {
namespace com {
namespace sch {

/**
 * Dummy scheduling heuristic that assigns equal criticality to all statements.
 */
struct TrivialHeuristic {
    utils::Bool operator()(const ir::StatementRef &lhs, const ir::StatementRef &rhs);
};

/**
 * Scheduling heuristic that assigns higher criticality to statements with a
 * cycle value further away from zero. This corresponds to critical path length,
 * if the statements were first scheduled in reverse order. However, it only
 * works properly when the cycle numbers are referenced such that the source
 * node of the (possibly reversed) DDG is at cycle zero.
 *
 * Note that this works even though scheduling will clobber the cycle numbers,
 * because the heuristic is only called for statements that are still
 * available, i.e. haven't yet been scheduled, while the cycle value is only
 * adjusted by the scheduler when a statement is scheduled.
 */
struct CriticalPathHeuristic {
    utils::Bool operator()(const ir::StatementRef &lhs, const ir::StatementRef &rhs);
};

/**
 * Deep criticality heuristic for use in list scheduling. This behaves like
 * CriticalPathHeuristic, except when the criticality of two statements is
 * equal: in this case, the criticality of the most critical successor is
 * recursively checked, until a difference is found.
 *
 * Deep criticality requires preprocessing to be performant. The usage pattern
 * is as followed:
 *
 *  - pre-schedule in the same way as you would for CriticalPathHeuristic;
 *  - call DeepCriticality::compute();
 *  - run scheduling using DeepCriticality::Heuristic; and
 *  - call DeepCriticality::clear();
 */
class DeepCriticality {
private:

    /**
     * Length of the critical path to the end of the schedule in cycles.
     */
    utils::UInt critical_path_length = 0;

    /**
     * When determining which of two nodes is more critical and they have
     * equal critical_path_length, the criticality of the most critical
     * dependent statement is checked instead. This is a recursive process
     * until no more dependent node exists on for either node.
     */
    ir::StatementRef most_critical_dependent;

    /**
     * Returns the Criticality annotation for the given statement, or returns
     * zero criticality if no statement exist.
     */
    static const DeepCriticality &get(const ir::StatementRef &statement);

    /**
     * Compares the criticality of two Criticality annotations.
     */
    utils::Bool operator<(const DeepCriticality &other) const;

    /**
     * Ensures that a valid criticality annotation exists for the given
     * statement. This will recursively ensure that dependent statements are
     * annotated, because this is needed to compute which of the dependent
     * statements is the most critical for deep criticality. The set tracks
     * which statements have valid annotations (there may be stray annotations
     * from previous scheduling operations that we must be sure to override).
     */
    static void ensure_annotation(
        const ir::StatementRef &statement,
        utils::Set<ir::StatementRef> &annotated
    );

public:

    /**
     * Annotates the instructions in block with DeepCriticality structures, such
     * that DeepCriticality::Heuristic() can be used as scheduling heuristic.
     * This requires that a data dependency graph has already been constructed
     * for the block, and that the block has already been scheduled in the
     * reverse direction of the desired list scheduling direction, with cycle
     * numbers still referenced such that the source node is at cycle 0.
     */
    static void compute(const ir::SubBlockRef &block);

    /**
     * Compares the criticality of two statements by means of their Criticality
     * annotation.
     */
    struct Heuristic {
        utils::Bool operator()(const ir::StatementRef &lhs, const ir::StatementRef &rhs);
    };

    /**
     * Clears the deep criticality annotations from the given block.
     */
    static void clear(const ir::SubBlockRef &block);

};

} // namespace sch
} // namespace com
} // namespace ql
