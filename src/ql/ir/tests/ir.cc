#include "ql/ir/ir.h"
#include "ql/ir/old_to_new.h"
#include "ql/ir/new_to_old.h"
#include "ql/ir/ops.h"
#include "ql/ir/cqasm/write.h"
#include "ql/ir/cqasm/read.h"
#include "ql/com/ddg/types.h"
#include "ql/com/ddg/ops.h"
#include "ql/com/ddg/build.h"
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
static utils::Bool trivial_heuristic(
    const ir::StatementRef &lhs,
    const ir::StatementRef &rhs
) {
    return false;
}

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
static utils::Bool critical_path_heuristic(
    const ir::StatementRef &lhs,
    const ir::StatementRef &rhs
) {
    return utils::abs(lhs->cycle) < utils::abs(rhs->cycle);
}

/**
 * Schedules the given block using the DDG that must already have been created
 * for the block in the direction of the DDG; if the DDG is not reversed that
 * amounts to ASAP, if it is reversed it amounts to ALAP.
 *
 * Scheduling is done in accordance with the given resource constraints and
 * criticality function. The heuristic function acts like a less-than
 * comparison for the criticality of the two instructions. Using the default
 * values, there are no resource constraints, and all statements are considered
 * to be equally critical. Note that, in the case of equal criticality,
 * instructions are ordered by their original place in the block, such that
 * the ordering is stable in this case.
 *
 * Cycles are not adjusted to be non-negative afterward. Rather, the source node
 * of the DDG will always be assigned cycle zero, so the assigned cycle numbers
 * are non-negative for ASAP, and non-positive for ALAP (due to the negated DDG
 * edge weights). The statements are also not ordered by cycle. These
 * post-processing operations are to be handled by the schedule() wrapper.
 */
static void schedule_internal(
    const ir::BlockBaseRef &block,
    std::function<utils::Bool(const ir::StatementRef&, const ir::StatementRef&)> heuristic = trivial_heuristic,
    const rmgr::CRef &resources = {}
) {
    // TODO
}

/**
 * Criticality annotation for statements/DDG nodes for use in list scheduling.
 * When constructed via compute_deep_criticality(), criticality is assigned by ALAP
 * scheduling without resource constraints; the difference between the cycle
 * number of the sink node and the cycle number of the statement then becomes
 * the (shallow) criticality. List scheduling may then use this information
 * to schedule more critical statements first. When two statements are equally
 * critical by this metric, the criticality of the most critical dependent
 * statement is recursively compared as a tie-breaking strategy.
 */
struct DeepCriticality {

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
     * Compares the criticality of two statements by means of their Criticality
     * annotation.
     */
    static utils::Bool lt(
        const ir::StatementRef &lhs,
        const ir::StatementRef &rhs
    ) {
        return get(lhs) < get(rhs);
    }

};

/**
 * Ensures that a valid criticality annotation exists for the given statement.
 * This will recursively ensure that dependent statements are annotated,
 * because this is needed to compute which of the dependent statements is the
 * most critical for deep criticality. The set tracks which statements have
 * valid annotations (there may be stray annotations from previous scheduling
 * operations that we must be sure to override).
 */
static void ensure_deep_criticality_annotation(
    const ir::StatementRef &statement,
    utils::Set<ir::StatementRef> &annotated
) {

    // If insertion into the set succeeds, we haven't annotated this statement
    // yet.
    if (annotated.insert(statement).second) {
        DeepCriticality criticality;

        // Determine the critical path length for shallow criticality. Because
        // the schedule used to determine criticality is constructed in reverse
        // order from the list scheduler it is intended for, instructions that
        // could be scheduled quickly have lower criticality. So, the
        // criticality of an instruction is simply its distance from the source
        // node of the reversed DDG, which is 0 by definition before the cycles
        // adjusted, so this is just the absolute value.
        criticality.critical_path_length = utils::abs(statement->cycle);

        // Find the most critical dependent statement for the given
        // scheduling direction.
        for (const auto &dependent : com::ddg::get_node(statement)->successors) {
            const auto &dependent_stmt = dependent.first;

            // Make sure the dependent statement has a criticality annotation
            // already.
            ensure_deep_criticality_annotation(dependent_stmt, annotated);

            // If the dependent statement is more critical than the most critical
            // dependent found thus far, replace it.
            if (
                criticality.most_critical_dependent.empty() ||
                    DeepCriticality::lt(criticality.most_critical_dependent, dependent_stmt)
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
 * that DeepCriticality::lt() can be used on them, usable as scheduling
 * heuristic. This requires that a data dependency graph has already been
 * constructed for the block, and that the block has already been scheduled in
 * the reverse direction of the desired list scheduling direction, with cycle
 * numbers still referenced such that the source node is at cycle 0.
 */
void compute_deep_criticality(const ir::SubBlockRef &block) {

    // Tracks which statements have already been annotated by *this call* (we
    // can't just check whether the annotation already exists, because it could
    // be an out-of-date annotation added by an earlier call).
    utils::Set<ir::StatementRef> annotated;

    // Annotate all the statements in the block. The order doesn't matter: when
    // a dependent statement doesn't yet have the criticality annotation needed
    // to determine deep criticality, it will be computed automatically using
    // recursion, and if criticality has already been computed the function
    // becomes no-op.
    for (const auto &statement : block->statements) {
        ensure_deep_criticality_annotation(statement, annotated);
    }

}

/**
 * Clears the deep criticality annotations from the given block.
 */
void clear_deep_criticality(const ir::SubBlockRef &block) {
    auto source = com::ddg::get_source(block);
    if (!source.empty()) source->erase_annotation<DeepCriticality>();
    auto sink = com::ddg::get_source(block);
    if (!sink.empty()) sink->erase_annotation<DeepCriticality>();
    for (const auto &statement : block->statements) {
        statement->erase_annotation<DeepCriticality>();
    }
}

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
        schedule_internal(block);

        // Reverse the DDG again so we don't clobber its direction.
        com::ddg::reverse(block);

    }

    // Perform the actual scheduling operation.
    rmgr::CRef manager;
    if (options.resource_constraints) {
        manager = *ir->platform->resources;
    }
    switch (options.heuristic) {
        case Heuristic::TRIVIAL:
            schedule_internal(block, trivial_heuristic, manager);
            break;
        case Heuristic::SHALLOW_CRITICAL_PATH:
            schedule_internal(block, critical_path_heuristic, manager);
            break;
        case Heuristic::DEEP_CRITICAL_PATH:
            compute_deep_criticality(block);
            schedule_internal(block, DeepCriticality::lt, manager);
            clear_deep_criticality(block);
            break;
    }

    // Adjust the cycles such that the lowest cycle number is cycle 0, to comply
    // with IR conventions.
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

    // Clean up the DDG.
    com::ddg::clear(block);

}

} // namespace detail
} // namespace schedule
} // namespace sch
} // namespace pass
} // namespace ql
