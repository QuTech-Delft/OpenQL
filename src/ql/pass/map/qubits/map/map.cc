/** \file
 * Defines the qubit router pass.
 */

#include "ql/pass/map/qubits/map/map.h"

#include "detail/mapper.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace map {

/**
 * Dumps docs for the qubit mapper.
 */
void MapQubitsPass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix, R"(
    The purpose of this pass is to ensure that the qubit connectivity
    constraints are met for all multi-qubit gates in each kernel. This is done
    by optionally applying a mixed integer linear programming algorithm to look
    for a perfect solution that does not require routing or figure out a good
    initial qubit placement, and then by heuristically inserting swap/move gates
    to change the mapping on the fly as needed. Finally, it decomposes all gates
    in the circuit to primitive gates.

    NOTE: the substeps of this pass will probably be subdivided into individual
    passes in the future.

    WARNING: this pass currently operates purely on a per-kernel basis. Because
    it may adjust the qubit mapping from input to output, a program consisting
    of multiple kernels that maintains a quantum state between the kernels may
    be silently destroyed.

    * Initial placement *

      This step attempts to find a single mapping of the virtual qubits of a
      circuit to the real qubits of the platform's qubit topology that minimizes
      the sum of the distances between the two mapped operands of all
      two-qubit gates in the circuit. The distance between two real qubits is
      the minimum number of swaps that is required to move the state of one of
      the two qubits to the other. It employs a Mixed Integer Linear Programming
      (MIP) algorithm to solve this, modelled as a Quadratic Assignment Problem.
      If enabled, this step may find a mapping that is optimal for the whole
      circuit, but because its time-complexity is exponential with respect to
      the size of the circuit, this may take quite some computer time. Also, the
      result is only really useful when in the mapping found all mapped operands
      of two-qubit gates are nearest-neighbor (i.e. distance 1). So, there is no
      guarantee for success: it may take too long and the result may not be
      optimal.

      NOTE: availability of this step depends on the build configuration of
      OpenQL due to license conflicts with the library used for solving the
      MIP problem. If it is not included, the step is effectively no-op, and
      a warning message will be printed.
)" R"(
    * Heuristic routing *

      This step essentially transforms the program by iterating over its gates
      from front to back and inserting `swap` or `move` gates when needed.
      Whenever it does this, it updates its internal virtual to real qubit
      mapping. While iterating, the virtual qubit indices of the incoming gates
      are replaced with real qubit indices, i.e. those defined in the topology
      section of the platform.

      Some platforms have gates for which parameters differ based on the qubits
      they operate on. For example, `cz q0, q1` may have a different duration
      than `cz q2, q3`, and `cz q0, q2` may not even exist because of
      topological constraints. However, rules like this make no sense when the
      cz gate is still using virtual qubit indices: it's perfectly fine for the
      user to do `cz q0, q2` at the input if the mapper is enabled.

      To account for this, the mapper will look for an alternative gate
      definition when it converts the virtual qubit indices to real qubit
      indices: specifically, it will look for a gate with `_real` or `_prim`
      (see also the primitive decomposition step) appended to the original gate
      name. For example, `cz q0, q2` may, after routing, be transformed to
      `cz_real q2, q3`. This allows you to define `cz` using a generalized gate
      definition (i.e. independent on qubit operands), and `cz_real` as a set of
      specialized gates as required by the platform.

      NOTE: the resolution order is `*_prim`, `*_real`, and finally just the
      original gate name. Thus, if you don't need this functionality, you don't
      need to define any `*_real` gates.

      Because the mapper inserts swap and/or move gates, it is important that
      these gates are actually defined in the configuration file (usually by
      means of a decomposition rule). The semantics for them must be as follows.

       - `swap x, y` or `swap_real x, y`: must apply a complete swap gate to the
         given qubits to exchange their state. If in the final decomposition one
         of the operands is used before the other, the second operand (`y`) is
         expected to be used first for the `reverse_swap_if_better` option to
         work right.

       - `move x, y` or `move_real x, y`: if `use_moves` is enabled, the mapper
         will attempt to use these gates instead of `swap`/`swap_real` if it
         knows that the `y` qubit is in the `|0>` state (or it can initialize
         it as such) and the result is better (or not sufficiently worse) than
         using a normal swap. Such a move gate can be implemented with two CNOTs
         instead of three.

      The order in which non-nearest-neighbor two-qubit gates are routed, the
      route taken for them, and where along the route the actual two-qubit gate
      is performed, is determined heuristically. The way in which this is done
      is controlled by the various options for this pass; it can be made really
      simple by just iterating over the circuit in the specified order and just
      choosing a random routing alternative whenever routing is needed, or more
      intelligent methods can be used at the cost of execution time and memory
      usage (the latter especially when a lot of alternative solutions are
      generated before a choice is made). Based on these options, time and space
      complexity can be anywhere from linear to exponential!
)" R"(
    * Decomposition into primitives *

      As a final step, the mapper will try to decompose the "real" gates (i.e.
      gates with qubit operands referring to real qubits) generated by the
      previous step into primitive gates, as actually executable by the
      target architecture. It does this by attempting to suffix the name of
      each gate with `_prim`. Thus, if you define a decomposition rule named
      `cz_prim` rather than `cz`, this rule will only be applied after mapping.
    )");
}

/**
 * Returns a user-friendly type name for this pass.
 */
utils::Str MapQubitsPass::get_friendly_type() const {
    return "Mapper";
}

/**
 * Constructs a qubit mapper.
 */
MapQubitsPass::MapQubitsPass(
    const utils::Ptr<const pmgr::Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::ProgramTransformation(pass_factory, instance_name, type_name) {

    //========================================================================//
    // Options for the initial virtual to real qubit map                      //
    //========================================================================//

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

    //========================================================================//
    // Options controlling the heuristic routing algorithm                    //
    //========================================================================//

    options.add_enum(
        "route_heuristic",
        "Controls which heuristic the router should use when selecting between "
        "possible routing operations. `base` and `base_rc` are the simplest "
        "forms: all routes are considered equally `good`, so the tie-breaking "
        "strategy is just applied immediately. `minextend` and "
        "`minextendrc` are way more involved (but also take longer to compute): "
        "these options will speculate what each option will do in terms of "
        "extending the duration of the circuit, optionally recursively, to find "
        "the best alternatives in terms of circuit duration within some"
        "lookahead window. The existence of the `rc` suffix specifies whether "
        "the internal scheduling for fitness determination should be done with "
        "or without resource constraints. `maxfidelity` is not supported "
        "in this build of OpenQL.",
        "base",
        {"base", "baserc", "minextend", "minextendrc", "maxfidelity"}
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
        "`route_heuristic`. For `1qfirst`, the dependency graph is used to "
        "greedily map all single-qubit gates, before proceeding with mapping "
        "the most critical two-qubit gate. If this gate is not nearest-neighbor, "
        "it is routed the same way as for `no`. `noroutingfirst` works the same, "
        "but also greedily maps two-qubit gates that don't require any routing "
        "regardless of criticality, before routing the most critical "
        "non-nearest-neighbor two-qubit gate. Finally, `all` works the same as "
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
        "configuration file.",
        "all",
        {"all", "borders"}
    );

    options.add_enum(
        "swap_selection_mode",
        "This controls how routing interacts with speculation. When `all`, all"
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
        "NOTE: this is an advanced/unstable option that influences "
        "`lookahead_mode` in a complex way; don't use it unless you know what "
        "you're doing. May be removed or changed in a later version of OpenQL."
    );

    options.add_int(
        "recursion_depth_limit",
        "Controls the maximum recursion depth while searching for alternative "
        "mapping solutions. "
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
        "write_dot_graphs",
        "Whether to print dot graphs of the schedules created using the "
        "embedded scheduler.",
        false
    );

}

/**
 * Builds the options structure for the mapper.
 */
pmgr::pass_types::NodeType MapQubitsPass::on_construct(
    const utils::Ptr<const pmgr::Factory> &factory,
    utils::List<pmgr::PassRef> &passes,
    pmgr::condition::Ref &condition
) {
    (void)factory;
    (void)passes;
    (void)condition;

    // Build the options structure for the mapper.
    parsed_options.emplace();

    parsed_options->initialize_one_to_one = options["initialize_one_to_one"].as_bool();
    parsed_options->assume_initialized = options["assume_initialized"].as_bool();
    parsed_options->assume_prep_only_initializes = options["assume_prep_only_initializes"].as_bool();
    parsed_options->enable_mip_placer = options["enable_mip_placer"].as_bool();
    parsed_options->mip_horizon = options["mip_horizon"].as_uint();

    auto route_heuristic = options["route_heuristic"].as_str();
    if (route_heuristic == "base") {
        parsed_options->heuristic = detail::Heuristic::BASE;
    } else if (route_heuristic == "baserc") {
        parsed_options->heuristic = detail::Heuristic::BASE_RC;
    } else if (route_heuristic == "minextend") {
        parsed_options->heuristic = detail::Heuristic::MIN_EXTEND;
    } else if (route_heuristic == "minextendrc") {
        parsed_options->heuristic = detail::Heuristic::MIN_EXTEND_RC;
    } else if (route_heuristic == "maxfidelity") {
        parsed_options->heuristic = detail::Heuristic::MAX_FIDELITY;
    } else {
        QL_ASSERT(false);
    }

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

    return pmgr::pass_types::NodeType::NORMAL;
}

/**
 * Runs the qubit mapper.
 */
utils::Int MapQubitsPass::run(
    const ir::compat::ProgramRef &program,
    const pmgr::pass_types::Context &context
) const {

    // Update options from context.
    parsed_options->output_prefix = context.output_prefix;

    // Run mapping.
    detail::Mapper().map(program, parsed_options.as_const());

    return 0;
}

} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
