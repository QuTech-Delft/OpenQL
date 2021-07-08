/** \file
 * Defines a basic resource-constrained ASAP/ALAP list scheduler for use as a
 * building block within more complex schedulers.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/opt.h"
#include "ql/ir/ir.h"
#include "ql/ir/describe.h"
#include "ql/com/ddg/ops.h"
#include "ql/com/sch/heuristics.h"
#include "ql/rmgr/manager.h"

namespace ql {
namespace com {
namespace sch {

/**
 * Scheduler interface. This implements a potentially resource-constrained
 * as-soon-as-possible/as-late-as-possible list scheduler, with criticality
 * determined by the HeuristicComparator comparator object (a less-than
 * comparator class, just like what's used for Set, Map, etc). The default
 * values for the criticality heuristic and resources effectively reduces the
 * algorithm to true ASAP/ALAP, with guaranteed stability of the statement order
 * for statements that become available simultaneously.
 *
 * The normal usage pattern is as follows:
 *
 *  - construct a data dependency graph for the block in question;
 *  - construct a Scheduler;
 *  - call Scheduler::run(); and
 *  - call Scheduler::convert_cycles().
 *
 * However, more control can be exerted over the way statements are scheduled
 * as well. For example, instead of run(), one can use get_available(),
 * try_schedule(), advance(), and is_done() to override the criticality metric.
 * The Scheduler object can also be cloned, to implement backtracking
 * algorithms.
 */
template <typename HeuristicComparator = TrivialHeuristic>
class Scheduler {

    /**
     * Criticality comparator for the availability list/set. The most critical
     * statement will end up in the front, so set iteration starts with the most
     * critical instruction. This uses the heuristic provided as template
     * argument to the surrounding function, and falls back on the original
     * statement order as recorded when the DDG was constructed for stability.
     */
    struct AvailableListComparator {
        utils::Bool operator()(const ir::StatementRef &lhs, const ir::StatementRef &rhs) const {

            // The heuristic implements "criticality less than," which would
            // result in reverse order, so we swap the value here.
            HeuristicComparator heuristic;
            if (heuristic(rhs, lhs)) return true;
            if (heuristic(lhs, rhs)) return false;

            // If the heuristic says both RHS and LHS are equal, fall back to
            // the original statement order.
            return com::ddg::get_node(lhs)->order < com::ddg::get_node(rhs)->order;

        }
    };

    /**
     * Returns whether the absolute value of a is less than the absolute value
     * of b.
     */
    static utils::Int abs_lt(utils::Int a, utils::Int b) {
        return utils::abs(a) < utils::abs(b);
    }

    /**
     * Returns the integer that has the highest absolute value.
     */
    static utils::Int abs_max(utils::Int a, utils::Int b) {
        return abs_lt(a, b) ? b : a;
    }

    /**
     * Comparator based on the abs_lt() function.
     */
    struct AbsoluteComparator {
        utils::Bool operator()(
            const utils::Int &lhs,
            const utils::Int &rhs
        ) const {
            return abs_lt(lhs, rhs);
        }
    };

    /**
     * The block that we're scheduling for.
     */
    ir::BlockBaseRef block;

    /**
     * The cycle we're currently scheduling for. This always starts at 0 for the
     * source node, and either increments (for ASAP/forward DDG order) or
     * decrements (for ALAP/reversed DDG) from there.
     */
    utils::Int cycle;

    /**
     * Representation of the scheduling direction, 1 for forward/ASAP, -1 for
     * reverse/ALAP.
     */
    utils::Int direction;

    /**
     * State of the resources for resource-constrained scheduling.
     */
    utils::Opt<rmgr::State> resource_state;

    /**
     * Set of statements that have been scheduled.
     */
    utils::Set<ir::StatementRef> scheduled;

    /**
     * List of available statements, i.e. statements we can immediately schedule
     * as far as the data dependency graph is concerned (but not necessarily as
     * far as the resource constraints are concerned). Per the comparator,
     * forward iteration over the set yields statements starting from the most
     * critical one per the HeuristicComparator template argument.
     */
    utils::Set<ir::StatementRef, AvailableListComparator> available;

    /**
     * The statements for which all predecessors have been scheduled, but which
     * aren't available yet because of edge weights/preceding statement
     * duration. The key is the cycle in which the accompanied list of
     * statements becomes valid. The comparator ensures that the first cycle
     * we'll encounter when scheduling will appear at the front, because we
     * always schedule away from cycle 0 regardless of the scheduling direction.
     */
    utils::Map<utils::Int, utils::List<ir::StatementRef>, AbsoluteComparator> available_in;

    /**
     * Set of statements that are still blocked, because their data
     * dependencies have not yet been scheduled.
     */
    utils::Set<ir::StatementRef> waiting;

    /**
     * Schedules the given statement in the current cycle, updating all state
     * accordingly.
     */
    void schedule(ir::StatementRef statement) {

        // Update the resource state.
        resource_state->reserve(cycle, statement);

        // Set the cycle number of the statement to the current cycle.
        statement->cycle = cycle;

        // Move the statement from available to scheduled.
        QL_ASSERT(available.erase(statement));
        QL_ASSERT(scheduled.insert(statement).second);

        // The DDG successors of the statement should all still be in the
        // waiting list, but some may be unblocked now. Check for that, and
        // move the unblocked statements to available_in or available
        // accordingly.
        for (const auto &successor_ep : com::ddg::get_node(statement)->successors) {
            const auto &successor_stmt = successor_ep.first;
            auto successor_node = com::ddg::get_node(successor_stmt);

            // Check if this successor of the statement we just scheduled is
            // now available.
            utils::Bool is_now_available = true;
            utils::Int available_from_cycle = 0;
            for (const auto &predecessor_ep : successor_node->predecessors) {
                const auto &predecessor_stmt = predecessor_ep.first;
                const auto &edge = predecessor_ep.second;

                // Ensure that all predecessors have been scheduled.
                if (!scheduled.count(predecessor_stmt)) {
                    is_now_available = false;
                    break;
                }

                // Compute the minimum cycle for which this statement will
                // become available.
                available_from_cycle = abs_max(
                    available_from_cycle,
                    predecessor_stmt->cycle + edge->weight
                );

            }

            // If the statement is now available, actually make it available by
            // moving it to the appropriate list.
            if (is_now_available) {
                if (available_from_cycle == cycle) {

                    // The statement is immediately available.
                    QL_ASSERT(available.insert(successor_stmt).second);

                } else {

                    // The statement is not immediately available, so we have
                    // to move it to available_in.
                    auto it = available_in.insert({available_from_cycle, {}});
                    it.first->second.push_back(successor_stmt);

                }

                // Remove the statement from the waiting list.
                QL_ASSERT(waiting.erase(successor_stmt));

            }

        }

        // If no more instructions are available in this cycle, advance to the
        // next cycle in which instructions will become available.
        if (available.empty()) {
            auto it = available_in.begin();
            if (it != available_in.end()) {
                cycle = it->first;
                for (const auto &available_statement : it->second) {
                    QL_ASSERT(available.insert(available_statement).second);
                }
                available_in.erase(it);
            }
        }

    }

public:

    /**
     * Creates a scheduler for the given block and initializes it.
     */
    Scheduler(
        const ir::BlockBaseRef &block,
        const rmgr::CRef &resources = {}
    ) : block(block) {

        // Always start scheduling at cycle 0.
        cycle = 0;

        // Cache the scheduling direction.
        direction = com::ddg::get_direction(block);
        if (direction == 1) {
            QL_DOUT("scheduling in forward direction (ASAP)");
        } else if (direction == -1) {
            QL_DOUT("scheduling in reverse direction (ALAP)");
        } else {
            QL_ICE("no data dependency graph is present");
        }

        // Construct the resource state. When scheduling without resource
        // constraints, the state will simply be empty and always say a
        // statement is available for scheduling.
        if (!resources.has_value()) {
            resource_state = rmgr::Manager({}).build(rmgr::Direction::UNDEFINED);
        } else if (direction > 0) {
            resource_state = resources->build(rmgr::Direction::FORWARD);
        } else {
            resource_state = resources->build(rmgr::Direction::BACKWARD);
        }

        // Initialize by putting the source statement in the available list and
        // all other statements in the waiting list.
        QL_ASSERT(available.insert(com::ddg::get_source(block)).second);
        for (const auto &statement : block->statements) {
            QL_ASSERT(waiting.insert(statement).second);
        }
        QL_ASSERT(waiting.insert(com::ddg::get_sink(block)).second);

        // Start by scheduling the source node.
        schedule(com::ddg::get_source(block));

    }

    /**
     * Returns the current cycle number.
     */
    utils::Int get_cycle() const {
        return cycle;
    }

    /**
     * Returns the direction in which the cycle number will be advanced by the
     * advance() function. This will be 1 for forward/ASAP scheduling, or -1 for
     * backward/ALAP scheduling.
     */
    utils::Int get_direction() const {
        return direction;
    }

    /**
     * Advances to the next cycle, or advances by the given number of cycles.
     */
    void advance(utils::UInt by = 1) {

        // Advance to the next cycle.
        cycle += direction * (utils::Int)by;

        // Advancing the cycle number may mean more statements will become
        // available due to data dependencies. If this is the case, move them
        // from available_in to available.
        auto it = available_in.begin();
        if (it != available_in.end() && it->first == cycle) {
            for (const auto &available_statement : it->second) {
                QL_ASSERT(available.insert(available_statement).second);
            }
            available_in.erase(it);
        }

    }

    /**
     * Returns the list of statements that are currently available, ordered by
     * decreasing criticality.
     */
    utils::List<ir::StatementRef> get_available() const {
        utils::List<ir::StatementRef> result;
        for (const auto &statement : available) {
            if (resource_state->available(cycle, statement)) {
                result.push_back(statement);
            }
        }
        return result;
    }

    /**
     * Tries to schedule either the given statement or (if no statement is
     * specified) the most critical available statement in the current cycle.
     * Returns whether scheduling was successful; if not, the specified
     * statement is not available in this cycle (or no statements are available
     * in this cycle if no statement was specified). If a statement was
     * scheduled and no more statements are available w.r.t. data dependencies
     * after that, the current cycle is automatically advanced to the next cycle
     * in which statements are available again.
     */
    utils::Bool try_schedule(const ir::StatementRef &statement = {}) {
        if (statement.empty()) {

            // Try to schedule statements that are available w.r.t. data
            // dependencies. Note that the iteration order here is implicitly by
            // decreasing criticality, because available is a set that uses the
            // criticality heuristic for its comparator.
            for (const auto &statement : available) {
                if (try_schedule(statement)) {
                    return true;
                }
            }
            return false;

        } else {

            // Schedule the given statement, if it's available.
            QL_DOUT("trying n" << utils::abs(ddg::get_node(statement)->order) << " = " << ir::describe(statement));
            QL_DOUT(" |-> with criticality " << HeuristicComparator()(statement));
            if (available.find(statement) == available.end()) {
                QL_DOUT(" '-> not available due to data dependencies");
                return false;
            }
            if (!resource_state->available(cycle, statement)) {
                QL_DOUT(" '-> not available due to resources");
                return false;
            }
            QL_DOUT(" '-> ok, scheduling in cycle " << cycle);
            schedule(statement);
            return true;

        }
    }

    /**
     * Returns whether the scheduler is done, i.e. all statements have been
     * scheduled.
     */
    utils::Bool is_done() const {
        if (!available.empty()) return false;
        if (!available_in.empty()) return false;
        if (!waiting.empty()) return false;
        QL_ASSERT(scheduled.size() == block->statements.size() + 2);
        return true;
    }

    /**
     * Runs the scheduler, scheduling all instructions in the block using
     * potentially resource-constrained ASAP (or ALAP if the DDG was
     * reversed) list scheduling w.r.t. the criticality heuristic specified via
     * HeuristicComparator. When resource constraints are used,
     * max_resource_block_cycles specifies how many cycles we'll spend waiting
     * for resources to become available when there is nothing else to do; this
     * is used to detect resource deadlocks and should simply be set to a high
     * enough number to prevent false deadlock detection. It may also be set to
     * 0 to disable the check.
     *
     * This function does *not* make all cycle numbers positive (cycle numbers
     * are referenced such that the source node has cycle 0) or sort statements
     * by the cycle numbers once done. This must be done manually using
     * convert_cycles() before the block is passed to anything that requires the
     * IR-mandated invariants on cycle numbers to be valid.
     */
    void run(utils::UInt max_resource_block_cycles = 0) {
        QL_DOUT("starting scheduler...");

        // Now schedule statements until all statements have been scheduled.
        while (!is_done()) {
            QL_DOUT(
                "cycle " << cycle << ", " <<
                scheduled.size() << " scheduled, " <<
                available.size() << " available w.r.t. data dependencies, " <<
                available_in.size() << " batches available later, " <<
                waiting.size() << " waiting"
            );
            QL_ASSERT(!available.empty());
            utils::UInt advanced = 0;
            while (!try_schedule()) {
                advance();
                advanced++;
                QL_DOUT("nothing is available, advancing to cycle " << cycle);
                if (max_resource_block_cycles && advanced > max_resource_block_cycles) {
                    utils::StrStrm ss;
                    ss << "scheduling resources seem to be deadlocked! ";
                    ss << "The current cycle is " << cycle << ", ";
                    ss << "and the available statements are:\n";
                    for (const auto &available_statement : available) {
                        ss << "  " << ir::describe(available_statement) << "\n";
                    }
                    ss << "The state of the resources is:\n";
                    resource_state->dump(ss, "  ");
                    QL_USER_ERROR(ss.str());
                }
            }
        }

        QL_DOUT(
            "scheduler done; schedule takes " <<
            utils::abs(com::ddg::get_sink(block)->cycle) << " cycles"
        );
    }

    /**
     * Adjusts the cycle numbers generated by the scheduler such that they
     * comply with the rules for the IR, i.e. statements must be ordered by
     * cycle, and the block starts at cycle zero.
     */
    void convert_cycles() {

        // Adjust the cycles such that the lowest cycle number is cycle 0.
        utils::Int min_cycle = utils::min(
            com::ddg::get_source(block)->cycle,
            com::ddg::get_sink(block)->cycle
        );
        com::ddg::get_source(block)->cycle -= min_cycle;
        for (const auto &statement : block->statements) {
            statement->cycle -= min_cycle;
        }
        com::ddg::get_sink(block)->cycle -= min_cycle;

        // Sort the statements by cycle.
        std::stable_sort(
            block->statements.begin(),
            block->statements.end(),
            [](const ir::StatementRef &lhs, const ir::StatementRef &rhs) {
                return lhs->cycle < rhs->cycle;
            }
        );

    }

};

// Explicitly instantiate the common scheduler types to reduce compilation time.
extern template class Scheduler<TrivialHeuristic>;
extern template class Scheduler<CriticalPathHeuristic>;
extern template class Scheduler<DeepCriticality::Heuristic>;

} // namespace sch
} // namespace com
} // namespace ql
