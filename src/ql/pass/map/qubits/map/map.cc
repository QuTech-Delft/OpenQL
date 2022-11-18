#include "ql/pass/map/qubits/map/map.h"

#include "detail/mapper.h"
#include "ql/pmgr/factory.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace map {

bool MapQubitsPass::is_pass_registered = pmgr::Factory::register_pass<MapQubitsPass>("map.qubits.Map");

void MapQubitsPass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix, R"(
    The purpose of this pass is to ensure that the qubit connectivity
    constraints are met for all multi-qubit gates in a block. This is done
    by heuristically inserting swap/move gates to route gates as needed.

    WARNING: this pass will currently succeed only on programs consisting of a single block.
)" R"(
      This pass iterates over the program and inserts `swap` or `move` gates when needed.
      Whenever it does this, it updates its internal virtual to real qubit
      mapping. While iterating, the virtual qubit indices of the incoming gates
      are replaced with real qubit indices, i.e. those defined in the topology
      section of the platform.

      Because the mapper inserts swap and/or move gates, it is important that
      these gates are actually defined in the configuration file (usually by
      means of a decomposition rule). The semantics for them must be as follows.

       - `swap x, y`: must apply a complete swap gate to the
         given qubits to exchange their state. If in the final decomposition one
         of the operands is used before the other, the second operand (`y`) is
         expected to be used first for the `reverse_swap_if_better` option to
         work right.

       - `move x, y`: if `use_moves` is enabled, the mapper
         will attempt to use these gates instead of `swap` if it
         knows that the `y` qubit is in the `|0>` state (or it can initialize
         it as such with a `prepz` gate) and the result is better
         (or not sufficiently worse) than using a normal swap.
         Such a move gate can be implemented with two CNOTs instead of three.

      The order in which non-nearest-neighbor two-qubit gates are routed, the
      route taken for them, and where along the route the actual two-qubit gate
      is performed, is determined heuristically. The way in which this is done
      is controlled by the various options for this pass; it can be made really
      simple by just iterating over the circuit in the specified order and just
      choosing a random routing alternative whenever routing is needed, or more
      intelligent methods can be used at the cost of execution time and memory
      usage (the latter especially when a lot of alternative solutions are
      generated before a choice is made). Based on these options, time and space
      complexity can be anywhere from linear to exponential!)");
}

utils::Str MapQubitsPass::get_friendly_type() const {
    return "Mapper";
}

MapQubitsPass::MapQubitsPass(
    const utils::Ptr<const pmgr::Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::Transformation(pass_factory, instance_name, type_name) {

    //========================================================================//
    // Options for the initial virtual to real qubit map                      //
    //========================================================================//

    options.add_bool(
        "assume_initialized",
        "Controls whether the mapper should assume that each qubit starts out "
        "as zero at the start of the block, rather than with an undefined "
        "state. If so, it does not need to use prepz to initialize qubits before "
        "attempting a move instead of swap."
    );

    options.add_bool(
        "assume_prep_only_initializes",
        "Controls whether the mapper may assume that a user-written prepz gate "
        "actually leaves the qubit in the zero state, rather than any other "
        "quantum state. This allows it to make some optimizations, namely to use move "
        "instead of swap."
    );

    //========================================================================//
    // Options controlling the heuristic routing algorithm                    //
    //========================================================================//

    options.add_enum(
        "route_heuristic",
        "Controls which heuristic the router should use when selecting between "
        "possible routing operations. `base` is the simplest "
        "forms: all routes are considered equally `good`, so the tie-breaking "
        "strategy is just applied immediately. `minextend` "
        "is way more involved (but also take longer to compute): "
        "this option will speculate what each option will do in terms of "
        "extending the duration of the circuit, optionally recursively, to find "
        "the best alternatives in terms of circuit duration within some "
        "lookahead window.",
        "base",
        {"base", "minextend"}
    );

    options.add_int(
        "max_alternative_routes",
        "Controls the maximum number of alternative routing solutions to "
        "generate before applying the heuristic and/or tie-breaking method to "
        "choose one. Leave unspecified or set to 0 to disable this limit.",
        "0",
        0, utils::MAX
    );

    options.add_enum(
        "tie_break_method",
        "Controls how to tie-break equally-scoring alternative mapping "
        "solutions. `first` and `last` choose respectively the first and "
        "last solution in the list (assuming the qubits have planar coordinates "
        "specified in the topology section, `first` selects the left-most "
        "alternative with the two-qubit gate near target, and `last` selects "
        "the right-most alternative with the two-qubit gate near source; when "
        "no coordinates are given the choice is undefined, though "
        "deterministic), `random` uses random number generation to "
        "select an alternative, and `critical` favors the alternative that "
        "maps the most critical gate as determined by the scheduler (if any).",
        "random",
        {"first", "last", "random", "critical"}
    );

    options.add_enum(
        "lookahead_mode",
        "Controls the strategy for selecting the next gate(s) to map. When `no`, "
        "just map the gates in the order of the circuit, disregarding "
        "commutation as allowed by the circuit's dependency graph. Single-qubit "
        "and nearest-neighbor two-qubit gates are mapped trivially; "
        "non-nearest-neighbor gates are mapped when encountered by generating "
        "alternative routing solutions and picking the best one via "
        "`route_heuristic`. For `noroutingfirst`, the dependency graph is used to "
        "greedily map all available single-qubit gates, before proceeding with mapping "
        "two-qubit gates that don't require any routing. If such gate is not nearest-neighbor, "
        "it is routed the same way as for `no`. Finally, `all` works the same as "
        "`noroutingfirst`, but instead of considering only routing alternatives "
        "for the most critical non-nearest-neighbor two-qubit gate, alternatives "
        "are generated for *all* available non-nearest-neighbor two-qubit gates, "
        "thus ignoring criticality and relying only on `route_heuristic` (which "
        "may be better depending on the heuristic chosen, but will cost execution "
        "time).",
        "noroutingfirst",
        {"no", "1qfirst", "noroutingfirst", "all"}
    );

    options.add_enum(
        "path_selection_mode",
        "Controls whether to consider all paths from a source to destination "
        "qubit while routing, or to favor routing along the route search space. "
        "The latter is only supported and sensible when the qubits are given "
        "planar coordinates in the topology section of the platform "
        "configuration file. Both `all` and `random` consider all paths, but "
        "for the latter the order in which the paths are generated is shuffled, "
        "which is useful to reduce bias when `max_alternative_routes` is used.",
        "all",
        {"all", "borders", "random"}
    );

    options.add_enum(
        "swap_selection_mode",
        "This controls how routing interacts with speculation. When `all`, all "
        "swaps for a particular routing option are committed immediately, before "
        "trying anything else. When `one`, only the first swap in the route "
        "from source to target qubit is committed. When `earliest`, the swap "
        "that can be done at the earliest point is selected, which might be "
        "the one swapping the source or target qubit.",
        "all",
        {"one", "all", "earliest"}
    );

    options.add_bool(
        "recurse_on_nn_two_qubit",
        "When a nearest-neighbor two-qubit gate is the next gate to be "
        "mapped, this controls whether the mapper will speculate on adding it "
        "now or later, or if it will add it immediately without speculation. "
        "This option has no effect when `route_heuristic` is `base`. "
        "NOTE: this is an advanced/unstable option that influences "
        "`lookahead_mode` in a complex way; don't use it unless you know what "
        "you're doing. May be removed or changed in a later version of OpenQL."
    );

    options.add_int(
        "recursion_depth_limit",
        "Controls the maximum recursion depth while searching for alternative "
        "mapping solutions. "
        "This option has no effect when `route_heuristic` is `base`. "
        "NOTE: this is an advanced/unstable option; don't use it unless you "
        "know what you're doing. May be removed or changed in a later version "
        "of OpenQL.",
        "0",
        0, utils::MAX, {"inf"}
    );

    options.add_real(
        "recursion_width_factor",
        "Limits how many alternative mapping solutions are considered as a "
        "factor of the number of best-scoring alternatives, rounded up. "
        "This option has no effect when `route_heuristic` is `base`. "
        "NOTE: this is an advanced/unstable option; don't use it unless you "
        "know what you're doing. May be removed or changed in a later version "
        "of OpenQL.",
        "1",
        0.0, utils::INF
    );

    options.add_real(
        "recursion_width_exponent",
        "Adjustment for recursion_width_factor based on the current recursion "
        "depth. For each additional level of recursion, the effective width "
        "factor is multiplied by this number. "
        "This option has no effect when `route_heuristic` is `base`. "
        "NOTE: this is an advanced/unstable option; don't use it unless you "
        "know what you're doing. May be removed or changed in a later version "
        "of OpenQL.",
        "1",
        0.0, 1.0
    );

    options.add_int(
        "use_moves",
        "Controls if/when the mapper inserts move gates rather than swap gates "
        "to perform routing. If `no`, swap gates are always used. Otherwise, "
        "a move gate is used if the other qubit has been initialized, or if "
        "initializing it only extends the circuit by the given number of "
        "cycles. `yes` implies this limit is 0 cycles.",
        "yes",
        0, utils::MAX, {"no", "yes"}
    );

    options.add_bool(
        "reverse_swap_if_better",
        "Controls whether the mapper will reverse the operands for a swap "
        "gate when reversal improves the schedule. NOTE: this currently assumes "
        "that the second qubit operand of the swap gate decomposition in the "
        "platform configuration file is used before than the first operand; if "
        "this is not the case, enabling this will worsen the routing result "
        "rather than improve it.",
        true
    );

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
        "write_dot_graphs",
        "Whether to print the data dependency graph as dot format, when "
        "using the `noroutingfirst` or `all` lookahead modes.",
        false
    );
    
    options.add_str(
        "decomposition_rule_name_pattern",
        "A regex pattern to select by name which decomposition rule should be applied "
        "to mapped instruction before scheduling them.",
        ""
    );
}

pmgr::pass_types::NodeType MapQubitsPass::on_construct(
    const utils::Ptr<const pmgr::Factory> &factory,
    utils::List<pmgr::PassRef> &passes,
    pmgr::condition::Ref &condition
) {
    (void)factory;
    (void)passes;
    (void)condition;

    parsed_options.emplace();

    parsed_options->assume_initialized = options["assume_initialized"].as_bool();
    parsed_options->assume_prep_only_initializes = options["assume_prep_only_initializes"].as_bool();

    auto route_heuristic = options["route_heuristic"].as_str();
    if (route_heuristic == "base") {
        parsed_options->heuristic = detail::Heuristic::BASE;
    } else if (route_heuristic == "minextend") {
        parsed_options->heuristic = detail::Heuristic::MIN_EXTEND;
    } else {
        QL_ASSERT(false);
    }

    parsed_options->max_alters = options["max_alternative_routes"].as_uint();

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
    } else if (path_selection_mode == "random") {
        parsed_options->path_selection_mode = detail::PathSelectionMode::RANDOM;
    } else {
        QL_ASSERT(false);
    }

    auto swap_selection_mode = options["swap_selection_mode"].as_str();
    if (swap_selection_mode == "one") {
        parsed_options->swap_selection_mode = detail::SwapSelectionMode::ONE;
    } else if (swap_selection_mode == "all") {
        parsed_options->swap_selection_mode = detail::SwapSelectionMode::ALL;
    } else if (swap_selection_mode == "earliest") {
        parsed_options->swap_selection_mode = detail::SwapSelectionMode::EARLIEST;
    } else {
        QL_ASSERT(false);
    }

    parsed_options->recurse_on_nn_two_qubit = options["recurse_on_nn_two_qubit"].as_bool();

    if (options["recursion_depth_limit"].as_str() == "inf") {
        parsed_options->recursion_depth_limit = utils::MAX;
    } else {
        parsed_options->recursion_depth_limit = options["recursion_depth_limit"].as_uint();
    }

    parsed_options->recursion_width_factor = options["recursion_width_factor"].as_real();
    parsed_options->recursion_width_exponent = options["recursion_width_exponent"].as_real();

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
    parsed_options->write_dot_graphs = options["write_dot_graphs"].as_bool();

    parsed_options->decomposition_rule_name_pattern = options["decomposition_rule_name_pattern"].as_str();

    return pmgr::pass_types::NodeType::NORMAL;
}

utils::Int MapQubitsPass::run(
    const ir::Ref &ir,
    const pmgr::pass_types::Context &context
) const {
    parsed_options->output_prefix = context.output_prefix;
    detail::Mapper(ir->platform, parsed_options.as_const()).map(ir->program);
    return 0;
}

} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
