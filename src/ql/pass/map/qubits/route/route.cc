/** \file
 * Defines the qubit router pass.
 */

#include "ql/pass/map/qubits/route/route.h"

#include "detail/mapper.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace route {

/**
 * Dumps docs for the qubit router.
 */
void RouteQubitsPass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix, R"(
    The purpose of this pass is to ensure that the qubit connectivity
    constraints are met for all multi-qubit gates in each kernel. This is done
    by optionally applying a mixed integer linear programming algorithm to look
    for a perfect solution that does not require routing, and/or by
    heuristically inserting swap/move gates to change the mapping on the fly as
    needed.

    NOTE: this pass currently operates purely on a per-kernel basis. Because it
    may adjust the qubit mapping from input to output, a program consisting of
    multiple kernels that maintains a quantum state between the kernels may be
    silently destroyed.
    )");
}

/**
 * Constructs a qubit router.
 */
RouteQubitsPass::RouteQubitsPass(
    const utils::Ptr<const pmgr::PassFactory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::KernelTransformation(pass_factory, instance_name, type_name) {

    //========================================================================//
    // Options controlling heuristic mapping                                  //
    //========================================================================//

    options.add_enum(
        "heuristic",
        "Controls which heuristic the heuristic mapper is to use.",
        "base",
        {"base", "baserc", "minextend", "minextendrc", "maxfidelity"}
    );

    options.add_bool(
        "initialize_one_to_one",
        "Controls whether the mapper should assume that each kernel starts with "
        "a one-to-one mapping between virtual and real qubits. When disabled, "
        "the initial mapping is treated as undefined.",
        true
    );

    options.add_bool(
        "assume_initialized",
        "Controls whether the mapper should assume that each qubit starts out "
        "as zero at the start of each kernel, rather than with an undefined "
        "state."
    );

    options.add_bool(
        "assume_prep_only_initializes",
        "Controls whether the mapper may assume that a user-written prepz gate "
        "actually leaves the qubit in the zero state, rather than any other "
        "quantum state. This allows it to make some optimizations."
    );

    options.add_enum(
        "lookahead_mode",
        "Controls the strategy for selecting the next gate(s) to map. "
        "TODO: document better.",
        "noroutingfirst",
        {"no", "1qfirst", "noroutingfirst", "all"}
    );

    options.add_enum(
        "path_selection_mode",
        "Controls whether to consider all paths from a source to destination "
        "qubit while routing, or to favor routing along the borders of the "
        "chip. The latter is only supported when the qubits are given "
        "coordinates in the topology section of the platform configuration "
        "file.",
        "all",
        {"all", "borders"}
    );

    options.add_enum(
        "swap_selection_mode",
        "Select only one swap, or earliest, or all swaps for one alternative. "
        "TODO: document better.",
        "all",
        {"one", "all", "earliest"}
    );

    options.add_bool(
        "recurse_nn_two_qubit",
        "Whether to recurse on non-nearest-neighbor two-qubit gates. "
        "TODO: document better."
    );

    options.add_int(
        "recursion_depth_limit",
        "Controls the maximum recursion depth while searching for alternative "
        "mapping solutions.",
        "0",
        0, utils::MAX, {"inf"}
    );

    options.add_real(
        "recursion_width_limit",
        "Limits how many alternative mapping solutions are considered as a "
        "factor of the number of best-scoring alternatives, rounded up.",
        "1",
        0, utils::INF
    );

    options.add_enum(
        "tie_break_method",
        "Controls how to tie-break equally-scoring alternative mapping "
        "solutions. \"first\" and \"last\" choose respectively the first and "
        "last solution in the list (TODO: does this mean anything or is this "
        "essentially random?), \"random\" uses random number generation to "
        "select an alternative, and \"critical\" favors the alternative that "
        "maps the most critical gate as determined by the scheduler (if any).",
        "random",
        {"first", "last", "random", "critical"}
    );

    options.add_int(
        "use_moves",
        "Controls if/when the mapper inserts move gates rather than swap gates "
        "to perform routing. If \"no\", swap gates are always used. Otherwise, "
        "a move gate is used if the other qubit has been initialized, or if "
        "initializing it only extends the circuit by the given number of "
        "cycles. \"yes\" implies this limit is 0 cycles.",
        "yes",
        0, utils::MAX, {"no", "yes"}
    );

    options.add_bool(
        "reverse_swap_if_better",
        "Controls whether the mapper will reverse the operands for a swap "
        "gate when reversal improves the schedule. This assumes that the "
        "second operand is used earlier than the first operand.",
        true
    );

    //========================================================================//
    // Options for the embedded schedulers                                    //
    //========================================================================//

    options.add_bool(
        "commute_multi_qubit",
        "Whether to consider commutation rules for the CZ and CNOT quantum "
        "gates.",
        false
    );

    options.add_bool(
        "commute_single_qubit",
        "Whether to consider commutation rules for single-qubit X and Z "
        "rotations.",
        false
    );

    options.add_bool(
        "print_dot_graphs",
        "Whether to print dot graphs of the schedules created using the "
        "embedded scheduler.",
        false
    );

    //========================================================================//
    // Options for the MIP initial placement engine                           //
    //========================================================================//

    options.add_bool(
        "enable_mip_placer",
        "Controls whether the MIP-based initial placement algorithm should be "
        "run before resorting to heuristic mapping.",
        false
    );

    options.add_int(
        "mip_horizon",
        "This controls how many two-qubit gates the MIP-based initial placement "
        "algorithm considers for each kernel (if enabled). If 0 or unspecified, "
        "all gates are considered.",
        "0", 0, utils::MAX
    );

}

/**
 * Builds the options structure for the mapper.
 */
pmgr::pass_types::NodeType RouteQubitsPass::on_construct(
    const utils::Ptr<const pmgr::PassFactory> &factory,
    utils::List<pmgr::PassRef> &passes,
    pmgr::condition::Ref &condition
) {
    (void)factory;
    (void)passes;
    (void)condition;

    // Build the options structure for the mapper.
    parsed_options.emplace();

    auto heuristic = options["heuristic"].as_str();
    if (heuristic == "base") {
        parsed_options->heuristic = detail::Heuristic::BASE;
    } else if (heuristic == "baserc") {
        parsed_options->heuristic = detail::Heuristic::BASE_RC;
    } else if (heuristic == "minextend") {
        parsed_options->heuristic = detail::Heuristic::MIN_EXTEND;
    } else if (heuristic == "minextendrc") {
        parsed_options->heuristic = detail::Heuristic::MIN_EXTEND_RC;
    } else if (heuristic == "maxfidelity") {
        parsed_options->heuristic = detail::Heuristic::MAX_FIDELITY;
    } else {
        QL_ASSERT(false);
    }

    parsed_options->initialize_one_to_one = options["initialize_one_to_one"].as_bool();
    parsed_options->assume_initialized = options["assume_initialized"].as_bool();
    parsed_options->assume_prep_only_initializes = options["assume_prep_only_initializes"].as_bool();

    auto lookahead_mode = options["lookahead_mode"].as_str();
    if (lookahead_mode == "no") {
        parsed_options->lookahead_mode = detail::LookaheadMode::DISABLED;
    } else if (lookahead_mode == "1qfirst") {
        parsed_options->lookahead_mode = detail::LookaheadMode::ONE_QUBIT_GATE_FIRST;
    } else if (lookahead_mode == "noroutingfirst") {
        parsed_options->lookahead_mode = detail::LookaheadMode::NO_ROUTING_FIRST;
    } else if (lookahead_mode == "all") {
        parsed_options->lookahead_mode = detail::LookaheadMode::ALL;
    } else {
        QL_ASSERT(false);
    }

    auto path_selection_mode = options["path_selection_mode"].as_str();
    if (path_selection_mode == "all") {
        parsed_options->path_selection_mode = detail::PathSelectionMode::ALL;
    } else if (path_selection_mode == "borders") {
        parsed_options->path_selection_mode = detail::PathSelectionMode::BORDERS;
    } else {
        QL_ASSERT(false);
    }

    auto swap_selection_mode = options["swap_selection_mode"].as_str();
    if (swap_selection_mode == "one") {
        parsed_options->swap_selection_mode = detail::SwapSelectionMode::ONE;
    } else if (path_selection_mode == "all") {
        parsed_options->swap_selection_mode = detail::SwapSelectionMode::ALL;
    } else if (path_selection_mode == "earliest") {
        parsed_options->swap_selection_mode = detail::SwapSelectionMode::EARLIEST;
    } else {
        QL_ASSERT(false);
    }

    parsed_options->recurse_nn_two_qubit = options["recurse_nn_two_qubit"].as_bool();

    if (options["recursion_depth_limit"].as_str() == "inf") {
        parsed_options->recursion_depth_limit = utils::MAX;
    } else {
        parsed_options->recursion_depth_limit = options["recursion_depth_limit"].as_uint();
    }

    parsed_options->recursion_width_limit = options["recursion_width_limit"].as_real();

    auto tie_break_method = options["tie_break_method"].as_str();
    if (tie_break_method == "first") {
        parsed_options->tie_break_method = detail::TieBreakMethod::FIRST;
    } else if (tie_break_method == "last") {
        parsed_options->tie_break_method = detail::TieBreakMethod::LAST;
    } else if (tie_break_method == "random") {
        parsed_options->tie_break_method = detail::TieBreakMethod::RANDOM;
    } else if (tie_break_method == "critical") {
        parsed_options->tie_break_method = detail::TieBreakMethod::CRITICAL;
    } else {
        QL_ASSERT(false);
    }

    auto use_moves = options["use_moves"].as_str();
    if (use_moves == "no") {
        parsed_options->use_move_gates = false;
    } else if (use_moves == "yes") {
        parsed_options->use_move_gates = true;
        parsed_options->max_move_penalty = 0;
    } else {
        parsed_options->use_move_gates = true;
        parsed_options->max_move_penalty = utils::parse_uint(use_moves);
    }

    parsed_options->reverse_swap_if_better = options["reverse_swap_if_better"].as_bool();
    parsed_options->commute_multi_qubit = options["commute_multi_qubit"].as_bool();
    parsed_options->commute_single_qubit = options["commute_single_qubit"].as_bool();
    parsed_options->print_dot_graphs = options["print_dot_graphs"].as_bool();
    parsed_options->enable_mip_placer = options["enable_mip_placer"].as_bool();
    parsed_options->mip_horizon = options["mip_horizon"].as_uint();

    return pmgr::pass_types::NodeType::NORMAL;
}

/**
 * Runs the qubit router.
 */
utils::Int RouteQubitsPass::run(
    const ir::ProgramRef &program,
    const ir::KernelRef &kernel,
    const pmgr::pass_types::Context &context
) const {

    // Update options from context.
    parsed_options->output_prefix = context.output_prefix;

    return 0;
}

} // namespace route
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
