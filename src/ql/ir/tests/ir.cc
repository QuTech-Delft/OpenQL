#include "ql/utils/filesystem.h"
#include "ql/ir/ir.h"
#include "ql/ir/describe.h"
#include "ql/ir/old_to_new.h"
#include "ql/ir/new_to_old.h"
#include "ql/ir/ops.h"
#include "ql/ir/cqasm/write.h"
#include "ql/ir/cqasm/read.h"
#include "ql/com/ddg/types.h"
#include "ql/com/ddg/ops.h"
#include "ql/com/ddg/build.h"
#include "ql/com/ddg/dot.h"
#include "ql/pass/io/cqasm/report.h"
#include "ql/rmgr/manager.h"

using namespace ql;

int main() {
    auto plat = ir::compat::Platform::build("test_plat", utils::Str("cc_light"));
    auto program = utils::make<ir::compat::Program>("test_prog", plat, 7, 32, 10);

    auto kernel = utils::make<ir::compat::Kernel>("static_kernel", plat, 7, 32, 10);
    kernel->x(0);
    kernel->classical(ir::compat::ClassicalRegister(1), 0);
    kernel->classical(ir::compat::ClassicalRegister(2), 10);
    program->add(kernel);

    auto sub_program = utils::make<ir::compat::Program>("x", plat, 7, 32, 10);
    kernel = utils::make<ir::compat::Kernel>("inner_loop_kernel", plat, 7, 32, 10);
    kernel->y(0);
    sub_program->add_for(kernel, 10);

    kernel = utils::make<ir::compat::Kernel>("outer_loop_kernel", plat, 7, 32, 10);
    kernel->z(0);
    kernel->classical(ir::compat::ClassicalRegister(3), 1);
    kernel->classical(ir::compat::ClassicalRegister(1), ir::compat::ClassicalOperation(
        ir::compat::ClassicalRegister(1), "+", ir::compat::ClassicalRegister(3)
    ));
    sub_program->add(kernel);

    program->add_do_while(sub_program, ir::compat::ClassicalOperation(
        ir::compat::ClassicalRegister(1), "<", ir::compat::ClassicalRegister(2)
    ));

    kernel = utils::make<ir::compat::Kernel>("if_a", plat, 7, 32, 10);
    kernel->x(1);
    auto kernel2 = utils::make<ir::compat::Kernel>("else", plat, 7, 32, 10);
    kernel2->y(1);
    program->add_if_else(kernel, kernel2, ir::compat::ClassicalOperation(
        ir::compat::ClassicalRegister(1), "==", ir::compat::ClassicalRegister(2)
    ));

    kernel = utils::make<ir::compat::Kernel>("if_b", plat, 7, 32, 10);
    kernel->z(1);
    program->add_if(kernel, ir::compat::ClassicalOperation(
        ir::compat::ClassicalRegister(1), ">", ir::compat::ClassicalRegister(2)
    ));

    auto ir = ir::convert_old_to_new(program);
    //ir->dump_seq();

    ir->program->objects.emplace<ir::TemporaryObject>("", ir->platform->default_bit_type);
    ir->program->objects.emplace<ir::VariableObject>("hello", ir::add_type<ir::IntType>(ir, "int64", true, 64));

    utils::StrStrm ss;
    ir::cqasm::write(ir, {}, ss);
    ir->program.reset();
    ir::cqasm::read(ir, ss.str());
    ss << "\n*** after read/write ***\n\n";

    ir::cqasm::WriteOptions wo;
    wo.include_statistics = true;
    ir::cqasm::write(ir, wo, ss);
    //ss << "\n*** after conversion to old and back to new ***\n\n";
    //ir = ir::convert_old_to_new(ir::convert_new_to_old(ir));
    //ir::cqasm::write(ir, {}, ss);

    std::cout << ss.str() << std::endl;

    return 0;
}


// Staging stuff from here onwards, just for syntax checking basically.

namespace ql {
namespace pass {
namespace sch {
namespace schedule {
namespace detail {

/**
 * Dummy scheduling heuristic that assigns equal criticality to all statements.
 */
struct TrivialHeuristic {
    utils::Bool operator()(
        const ir::StatementRef &lhs,
        const ir::StatementRef &rhs
    ) {
        return false;
    }
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
    utils::Bool operator()(
        const ir::StatementRef &lhs,
        const ir::StatementRef &rhs
    ) {
        return utils::abs(lhs->cycle) < utils::abs(rhs->cycle);
    }
};

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
        utils::Bool operator()(
            const ir::StatementRef &lhs,
            const ir::StatementRef &rhs
        ) {

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
        ) {
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
    void schedule(const ir::StatementRef &statement) {

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
        if (direction > 0) {
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
                if (resource_state->available(cycle, statement)) {
                    schedule(statement);
                    return true;
                }
            }
            return false;

        } else {

            // Schedule the given statement, if it's available.
            if (available.find(statement) == available.end()) return false;
            if (!resource_state->available(cycle, statement)) return false;
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

        // Now schedule statements until all statements have been scheduled.
        while (!is_done()) {
            utils::UInt advanced = 0;
            while (!try_schedule()) {
                advance();
                advanced++;
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
        for (const auto &statement : block->statements) {
            statement->cycle -= min_cycle;
        }

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

/**
 * Criticality annotation for statements/DDG nodes for use in list scheduling.
 * When constructed via compute(), criticality is assigned by means of the
 * current cycle numbers; the difference between the cycle number of the
 * sink node and the cycle number of the statement then becomes
 * the (shallow) criticality. List scheduling may then use this information
 * to schedule the most deep-critical statements first. That is, when two
 * statements are equally critical by the usual shallow criticality metric, the
 * criticality of the most critical dependent statement is recursively compared
 * as a tie-breaking strategy.
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
    static const DeepCriticality &get(const ir::StatementRef &statement) {
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
    utils::Bool operator<(const DeepCriticality &other) const {

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

public:

    /**
     * Annotates the instructions in block with DeepCriticality structures, such
     * that DeepCriticality::Heuristic() can be used as scheduling heuristic.
     * This requires that a data dependency graph has already been constructed
     * for the block, and that the block has already been scheduled in the
     * reverse direction of the desired list scheduling direction, with cycle
     * numbers still referenced such that the source node is at cycle 0.
     */
    static void compute(const ir::SubBlockRef &block) {

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
    struct Heuristic {
        utils::Bool operator()(
            const ir::StatementRef &lhs,
            const ir::StatementRef &rhs
        ) {
            return get(lhs) < get(rhs);
        }
    };

    /**
     * Clears the deep criticality annotations from the given block.
     */
    static void clear(const ir::SubBlockRef &block) {
        auto source = com::ddg::get_source(block);
        if (!source.empty()) source->erase_annotation<DeepCriticality>();
        auto sink = com::ddg::get_source(block);
        if (!sink.empty()) sink->erase_annotation<DeepCriticality>();
        for (const auto &statement : block->statements) {
            statement->erase_annotation<DeepCriticality>();
        }
    }

};

/**
 * Scheduling heuristic, determining the order in which available statements are
 * (attempted to be) scheduled.
 */
enum class Heuristic {

    /**
     * The trivial heuristic, which assigns priority according to the order in
     * which the statements appear in the original schedule. For forward/ASAP
     * scheduling, this means statements appearing earlier will be scheduled
     * earlier. For backward/ALAP the order is reversed accordingly.
     *
     * Note that, besides the nice characteristic that instruction order of the
     * original program will be preserved in tie-breaking situations, this also
     * means that you can chain multiple scheduler passes to get more complex
     * behavior. For example, if you first schedule using ASAP and then via
     * ALAP, the effect is that the latter will behave like a
     * critical-path-based list scheduler.
     */
    TRIVIAL,

    /**
     * Assigns priority to instructions based on the length of their critical
     * path to the sink node in the data dependency graph. The behavior is
     * largely the same as two back-to-back trivial schedulers, the former of
     * which without resource constraints and in reverse order, but is slightly
     * faster.
     */
    SHALLOW_CRITICAL_PATH,

    /**
     * Like SHALLOW_CRITICAL_PATH, but recursively tie-breaks equal-length
     * critical paths based on the length of the critical path of the most
     * critical dependents of the instruction.
     */
    DEEP_CRITICAL_PATH

};

/**
 * Options for the scheduler.
 */
struct Options {

    /**
     * The heuristic used to determine in which order available statements are
     * (attempted to be) scheduled.
     */
    utils::Bool resource_constraints;

    /**
     * Whether to respect or ignore resource constraints when scheduling.
     */
    Heuristic heuristic;

    /**
     * Whether to reverse the direction of the data dependency graph prior to
     * scheduling. This turns the otherwise forward/ASAP-like scheduling
     * algorithms into backward/ALAP-like scheduling.
     */
    utils::Bool reverse_direction;

    /**
     * Whether to consider commutation rules for multi-qubit gates.
     */
    utils::Bool commute_multi_qubit;

    /**
     * Whether to consider commutation rules for single-qubit gates.
     */
    utils::Bool commute_single_qubit;

    /**
     * The maximum number of cycles to wait for the resource constraints to
     * unblock a statement when there is nothing else to do. This is used for
     * deadlock detection. It should just be set to a high number, or can be
     * set to 0 to disable deadlock detection (but then the scheduler might end
     * up in an infinite loop).
     */
    utils::UInt max_resource_block_cycles;

    /**
     * Filename of a dot file to write, representing the data dependency graph
     * that was used and the cycle numbers assigned.
     */
    utils::Str dot_file;

};

/**
 * Entry point for scheduling a single block.
 */
void schedule(
    const ir::Ref &ir,
    const ir::SubBlockRef &block,
    const Options &options
) {

    // Build a data dependency graph for the block.
    com::ddg::build(
        ir,
        block,
        options.commute_multi_qubit,
        options.commute_single_qubit
    );

    // Reverse the DDG if backward/ALAP scheduling is desired.
    if (options.reverse_direction) {
        com::ddg::reverse(block);
    }

    // Pre-schedule in the reverse direction for critical-path-length-based
    // heuristics.
    if (
        options.heuristic == Heuristic::SHALLOW_CRITICAL_PATH ||
        options.heuristic == Heuristic::DEEP_CRITICAL_PATH
    ) {

        // Criticality for ASAP list scheduling is computed via ALAP
        // pre-scheduling and vice-versa. So we need to reverse the direction of
        // the DDG to reverse the scheduling direction prior to prescheduling.
        com::ddg::reverse(block);

        // Perform prescheduling.
        Scheduler<>(block).run();

        // Reverse the DDG again so we don't clobber its direction.
        com::ddg::reverse(block);

    }

    // Perform the actual scheduling operation.
    rmgr::CRef manager;
    if (options.resource_constraints) {
        manager = *ir->platform->resources;
    }
    switch (options.heuristic) {
        case Heuristic::TRIVIAL: {
            Scheduler<TrivialHeuristic> scheduler(block, manager);
            scheduler.run(options.max_resource_block_cycles);
            scheduler.convert_cycles();
            break;
        }
        case Heuristic::SHALLOW_CRITICAL_PATH: {
            Scheduler<CriticalPathHeuristic> scheduler(block, manager);
            scheduler.run(options.max_resource_block_cycles);
            scheduler.convert_cycles();
            break;
        }
        case Heuristic::DEEP_CRITICAL_PATH: {
            DeepCriticality::compute(block);
            Scheduler<DeepCriticality::Heuristic> scheduler(block, manager);
            scheduler.run(options.max_resource_block_cycles);
            scheduler.convert_cycles();
            DeepCriticality::clear(block);
            break;
        }
    }

    // Write the schedule as a dot file if requested.
    if (!options.dot_file.empty()) {

        // Reverse the DDG back to forward direction if needed, since that makes
        // it much more readable.
        if (options.reverse_direction) {
            com::ddg::reverse(block);
        }

        // Write the file.
        com::ddg::dump_dot(block, utils::OutFile(options.dot_file).unwrap());

    }

    // Clean up the DDG.
    com::ddg::clear(block);

}

} // namespace detail
} // namespace schedule
} // namespace sch
} // namespace pass
} // namespace ql
