#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/ptr.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace map {
namespace detail {

/**
 * The available heuristics for mapping. This controls which routing
 * alternatives are considered to be the best.
 */
enum class Heuristic {

    /**
     * Consider all alternatives as equivalent, unintelligently applying the
     * tie-breaking strategy to all options. No recursion is performed. Internal
     * gate scheduling is determined without resource
     * constraints. Gate scheduling is only used for choosing move vs swap, and
     * reversing swap operands.
     */
    BASE,

    /**
     * Favor alternatives with minimal cycle time extension when using
     * non-resource-constrained scheduling. When multiple (good) alternatives
     * exist, recursion/speculation is used to see which is best. The limits for
     * this recursion are controlled by recursion_depth_limit and
     * recursion_width_limit. When the limit is reached, the tie-breaking method
     * is applied to the best-scoring alternatives.
     */
    MIN_EXTEND

};

std::ostream &operator<<(std::ostream &os, Heuristic h);

/**
 * Controls the strategy for selecting the next gate(s) to map.
 *
 * TODO: document variants here.
 */
enum class LookaheadMode {
    DISABLED,
    ONE_QUBIT_GATE_FIRST,
    NO_ROUTING_FIRST,
    ALL
};

std::ostream &operator<<(std::ostream &os, LookaheadMode lm);

/**
 * Controls which paths are considered when routing.
 */
enum class PathSelectionMode {

    /**
     * Consider all possible paths.
     */

    ALL,
    /**
     * Favor routing along the borders of the rectangle defined by the source
     * and target qubit. Only supported when the qubits are given coordinates in
     * the topology section of the platform configuration file.
     */
    BORDERS,

    /**
     * Consider all possible paths, but randomize the order in which paths are
     * generated. This is useful when the amount of generated alternative paths
     * needs to be limited for scalability.
     */
    RANDOM

};

std::ostream &operator<<(std::ostream &os, PathSelectionMode psm);

/**
 * Swap selection mode.
 *
 * TODO: document better.
 */
enum class SwapSelectionMode {

    /**
     * Select only one swap.
     */
    ONE,

    /**
     * Select all swaps for one alternative.
     */
    ALL,

    /**
     * Select the earliest swap.
     */
    EARLIEST

};

std::ostream &operator<<(std::ostream &os, SwapSelectionMode ssm);

/**
 * Available methods for tie-breaking equally-scoring alternative mapping
 * solutions.
 */
enum class TieBreakMethod {

    /**
     * Select the first alternative.
     */
    FIRST,

    /**
     * Select the last alternative.
     */
    LAST,

    /**
     * Select a random alternative.
     */
    RANDOM,

    /**
     * Select the most critical alternative in terms of scheduling.
     */
    CRITICAL

};

std::ostream &operator<<(std::ostream &os, TieBreakMethod tbm);

/**
 * Main options structure.
 */
struct Options {

    /**
     * Prefix for writing output files.
     */
    utils::Str output_prefix;

    /**
     * Controls whether the mapper should assume that each qubit starts out
     * as zero at the start of the block, rather than with an undefined
     * state.
     */
    utils::Bool assume_initialized = false;

    /**
     * Controls whether the mapper may assume that a user-written prepz gate
     * actually leaves the qubit in the zero state, rather than any other
     * quantum state. This allows it to make some optimizations.
     */
    utils::Bool assume_prep_only_initializes = false;

    /**
     * Controls which heuristic the heuristic mapper is to use.
     */
    Heuristic heuristic = Heuristic::BASE;

    /**
     * Maximum number of alternative routing solutions to generate before
     * picking one via the heuristic and tie-breaking method. 0 means no limit.
     */
    utils::UInt max_alters = 0;

    /**
     * Controls how to tie-break equally-scoring alternative mapping solutions.
     */
    TieBreakMethod tie_break_method = TieBreakMethod::RANDOM;

    /**
     * Controls the strategy for selecting the next gate(s) to map.
     */
    LookaheadMode lookahead_mode = LookaheadMode::NO_ROUTING_FIRST;

    /**
     * Controls which paths are considered when routing.
     */
    PathSelectionMode path_selection_mode = PathSelectionMode::ALL;

    /**
     * Swap selection mode.
     */
    SwapSelectionMode swap_selection_mode = SwapSelectionMode::ALL;

    /**
     * Whether to recurse on nearest-neighbor two-qubit gates.
     */
    utils::Bool recurse_on_nn_two_qubit = false;

    /**
     * Controls the maximum recursion depth while searching for alternative
     * mapping solutions.
     */
    utils::UInt recursion_depth_limit = 0;

    /**
     * Limits how many alternative mapping solutions are considered as a
     * factor of the number of best-scoring alternatives, rounded up.
     */
    utils::Real recursion_width_factor = 0.0;

    /**
     * Adjustment factor for recursion_width_factor for each recursion level.
     * Can be reduced to limit the search space as recursion depth increases.
     */
    utils::Real recursion_width_exponent = 1.0;

    /**
     * Whether to use move gates if possible, instead of always using swap.
     */
    utils::Bool use_move_gates = true;

    /**
     * Maximum cycle penalty tolerated for qubit initialization for a move to
     * be inserted instead of a swap.
     */
    utils::UInt max_move_penalty = 0;

    /**
     * Reverse the operands for a swap gate if this improves timing. Relies on
     * the second operator being used before the first in the swap gate
     * decomposition.
     */
    utils::Bool reverse_swap_if_better = true;

    /**
     * Whether the embedded scheduler is allowed to commute CZ and CNOT gates.
     */
    utils::Bool commute_multi_qubit = false;

    /**
     * Whether the embedded scheduler is allowed to commute single-qubit X and
     * Z rotations.
     */
    utils::Bool commute_single_qubit = false;

    /**
     * Whether to print dot graphs of the schedules created using the embedded
     * scheduler.
     */
    utils::Bool write_dot_graphs = false;

    std::string decomposition_rule_name_pattern = "";

};

/**
 * Shared pointer reference to the options structure, to avoid having to copy
 * its contents around all the time.
 */
using OptionsRef = utils::Ptr<const Options>;

} // namespace detail
} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
