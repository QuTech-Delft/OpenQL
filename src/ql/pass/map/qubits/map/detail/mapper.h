#pragma once

#include <random>
#include "ql/utils/progress.h"
#include "ql/ir/ir.h"
#include "ql/com/map/qubit_mapping.h"
#include "options.h"
#include "free_cycle.h"
#include "past.h"
#include "alter.h"
#include "future.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace map {
namespace detail {

/**
 * Strategy options for finding routing paths.
 */
enum class PathStrategy {

    /**
     * Consider all shortest path alternatives.
     */
    ALL,

    /**
     * Only consider the shortest path along the left side of the rectangle of
     * the source and target qubit.
     */
    LEFT,

    /**
     * Only consider the shortest path along the right side of the rectangle of
     * the source and target qubit.
     */
    RIGHT,

    /**
     * Consider the shortest paths along both the left and right side of the
     * rectangle of the source and target qubit.
     */
    LEFT_RIGHT,

    /**
     * Consider all path alternatives, but randomize the order of the generated
     * paths. This is useful when the amount of generated alternative paths
     * needs to be limited for scalability.
     */
    RANDOM

};

std::ostream &operator<<(std::ostream &os, PathStrategy p);

/**
 * Mapper: map operands of gates and insert swaps so that two-qubit gate
 * operands are nearest-neighbor (NN).
 *
 * All gates must be unary or two-qubit gates. The operands are virtual qubit
 * indices. After mapping, all virtual qubit operands have been mapped to real
 * qubit operands, and all 2-qubit gates have nearest-neighbor operands.
 * 
 * The router can add swap or move gates to accomplish this, while preserving
 * circuit semantics.
 *
 * While mapping, the relationship between virtual and real qubits is tracked by
 * a QubitMapping object. This tracks two things:
 *
 *  - The actual virtual to real qubit map `v2r`. The
 *    map is initialized one to one (virtual qubit index i maps to real qubit index i)
 *    before routing.
 *
 *  - The liveness/state of each real qubit. This can be none (the state is
 *    garbage), initialized (the state is |0>), or live (the state is anything
 *    in use by the block). The states start out as none or initialized,
 *    depending on whether the mapper is configured such that it's allowed to
 *    assume that the qubits have already been initialized before the start of
 *    the block. Any quantum gate presented to the mapper by the input circuit
 *    puts the state of its qubits into the live state, except prepz if the
 *    mapper is configured such that it treats it as an initialization. The
 *    initialized state is used to replace swap gates (3 CNOTs) with moves
 *    (2 CNOTs): if either qubit being routed through is initialized rather than
 *    live, a move is inserted (if moves are enabled at all via configuration)
 *    rather than a swap, and if either qubit being routed through is in the
 *    garbage state, the mapper will see if initializing it does not increase
 *    circuit length too much (the threshold is configurable).
 *
 * The mapping is done in the context of a graph of qubits defined by the
 * platform. The description of this graph/grid lives in platform->topology.
 *
 * The mapper/router currently supports only programs consisting of a single block.
 *
 * Anticipating inter-block mapping, the mapper maintains a block input
 * mapping coming from the context, and produces a block output mapping for the
 * context; the mapper updates the block's circuit from virtual to real.
 *
 * Inter-block control flow and consequent mapping dependence between blocks
 * is not implemented. TO BE DONE. The design of mapping multiple blocks is as
 * follows:
 *
 *  - Initially the program wide initial mapping is a 1 to 1 mapping of virtual
 *    to real qubits.
 *  - When starting to map a block, there is a set of already mapped blocks,
 *    and a set of not yet mapped blocks. Of each mapped block, there is an
 *    output mapping, i.e. the mapping of virts to reals with the rs per
 *    virtual. The current block has a set of blocks which are direct
 *    predecessor in the program's control flow, a subset of those direct
 *    predecessors thus has been mapped and another subset not mapped; the
 *    output mappings of the mapped predecessor blocks are input.
 *  - Unify these multiple input mappings to a single one; this may introduce
 *    swaps on the control flow edges. The result is the input mapping of the
 *    current block; keep it for later reference.
 *  - Use heuristics to map the input (or what initial placement left to do).
 *  - When done, keep the output mapping as the block's output mapping. For all
 *    mapped successor blocks, compute a transition from output to their input,
 *    and add it to the edge; the edge code must be optimized for:
 *     - being empty: nothing needs to be done
 *     - having a source with one succ; the edge code can be appended to that
 *       succ
 *     - having a target with one pred; the edge code can be prepended to that
 *       pred
 *     - otherwise, a separate intermediate block for the transition code must
 *       be created, and added.
 *
 * THE ABOVE INTER-BLOCK MAPPING IS NOT IMPLEMENTED.
 */
class Mapper {
public:
    Mapper(const ir::PlatformRef &p, const OptionsRef &o) :
            platform(p), options(o) {
        static constexpr utils::UInt seed = 123;
        rng.seed(seed);
    }

    /**
     * Routes and maps the given program. A single block is supported.
     * If the program consists of multiple blocks, a fatal error is raised.
     */
    void map(ir::ProgramRef program);

private:
    struct RoutingStatistics {
        utils::UInt num_swaps_added = 0;
        utils::UInt num_moves_added = 0;
    };

    ir::PlatformRef platform;
    ir::BlockBaseRef block;
    OptionsRef options;

    /**
     * Random-number generator for the "random" tie-breaking option.
     * The seed is constant so that the output of OpenQL is deterministic.
     */
    std::mt19937 rng;

    /**
     * Routing progress tracker.
     */
    utils::Progress routing_progress;

    /**
     * Qubit mapping before mapping, set by map_block().
     */
    com::map::QubitMapping v2r_in;

    /**
     * Qubit mapping after mapping, set by route().
     */
    com::map::QubitMapping v2r_out;

    /**
     * Find shortest paths between src and tgt in the grid, bounded by a
     * particular strategy. path is a linked-list node representing the complete
     * path from the initial src qubit to src in reverse order, not including src;
     * it will be null for the initial call. budget is the maximum number of hops
     * allowed in the path from src and is at least distance to tgt, but can be
     * higher when not all hops qualify for doing a two-qubit gate or to find
     * more than just the shortest paths. This recursively calls itself with src
     * replaced with its neighbors (and additional bookkeeping) until src equals
     * tgt, adding all alternatives to the alters list as it goes. For each path,
     * the alters are further split into all feasible alternatives for the
     * location of the non-nearest-neighbor two-qubit gate that started the
     * routing request. If max_alters is nonzero, recursion will stop once the
     * total number of entries in alters reaches or surpasses the limit (it may
     * surpass due to the checks only happening before splitting).
     */
    utils::List<Alter> gen_shortest_paths(
        const ir::CustomInstructionRef &gate,
        std::list<utils::UInt> path,
        utils::UInt src,
        utils::UInt tgt,
        utils::UInt budget,
        utils::UInt max_alters,
        PathStrategy strategy
    );

    /**
     * Find shortest paths in the grid for making the given gate
     * nearest-neighbor, from qubit src to qubit tgt, with an alternative for
     * each one. This starts off the recursion done by the above overload of
     * gen_shortest_paths(), and then generates new alternatives for each
     * possible "split" of each path.
     *
     * Steps:
     *  - Compute budget. Usually it is distance but it can be higher such as
     *    for multi-core.
     *  - Reduce the number of paths depending on the path selection option.
     *  - When not all shortest paths found are valid, take these out.
     *  - Paths are further split because each split may give rise to a separate
     *    alternative. A split is a hop where the two-qubit gate is assumed to
     *    be done.
     *
     * The result is a list of alternatives suitable for being
     * evaluated for any routing metric.
     */
    utils::List<Alter> gen_shortest_paths(
        const ir::CustomInstructionRef &gate,
        utils::UInt src,
        utils::UInt tgt
    );

    /**
     * Return all possible alternatives for making the given gate
     * nearest-neighbor, starting from given past (with its mappings).
     */
    utils::List<Alter> gen_alters_gate(
        const ir::CustomInstructionRef &gate,
        Past &past
    );

    /**
     * Generates all possible variations of making the given gates
     * nearest-neighbor, starting from given past (with its mappings).
     * Depending on the lookahead option, only take the first (most
     * critical) gate, or take all gates and concatenate alternatives.
     */
    utils::List<Alter> gen_alters(const utils::List<ir::CustomInstructionRef> &gates, Past &past);

    /**
     * Chooses an Alter from the list based on the configured tie-breaking
     * strategy.
     */
    Alter tie_break_alter(utils::List<Alter> &alters, Future &future);

    /**
     * Map gate virtual operands wrt past's mapping, and add the gate to past
     * and its free cycle map.
     */
    void map_routed_gate(const ir::CustomInstructionRef &gate, Past &past, utils::Any<ir::Statement> *output_circuit = nullptr);

    /**
     * Commit the given Alter, generating swaps in the past and taking it out
     * of future. Depending on configuration, this might not
     * actually place the target gate for the given alternative yet, because
     * only part of the swap chain is generated; in this case, swaps are added
     * to past, but future is not updated.
     */
    void commit_alter(Alter &alter, Future &future, Past &past, utils::Any<ir::Statement> *output_circuit = nullptr);

    /**
     * Find gates available for scheduling that do not require routing and map them.
     * Returns remaining gates that require routing.
     */
    utils::List<ir::CustomInstructionRef> map_mappable_gates(
        Future &future,
        Past &past,
        utils::Bool also_nn_two_qubit_gates,
        utils::Any<ir::Statement> *output_circuit = nullptr
    );

    /**
     * Select an Alter based on the selected heuristic.
     *
     *  - If route_heuristic == "base", consider all alternatives as equivalent,
     *    and thus apply the tie-breaking strategy to all.
     *  - If route_heuristic == "minextend", prefer alternatives
     *    with the minimal cycle extension
     *    of the given past (or some factor of that amount, ordered by
     *    increasing cycle extension) and recurse. When the recursion depth
     *    limit is reached, apply the tie-breaking strategy.
     *
     * For recursion, past is the speculative past, and base_past is the past
     * we've already committed to, and should thus measure fitness against.
     */
    Alter select_alter(
        utils::List<Alter> &alters,
        Future &future,
        Past &past,
        Past &base_past,
        utils::UInt recursion_depth
    );

    /**
     * Process all gates in future and update past with routing result.
     */
    utils::Any<ir::Statement> route_gates(Future &future, Past &past);
    
    /**
     * Map/route the block wrt the virtual-to-real v2r qubit mapping.
     */
    RoutingStatistics route(ir::BlockBaseRef block, com::map::QubitMapping &v2r);

    /**
     * Runs routing for the given block.
     */
    RoutingStatistics map_block(ir::BlockBaseRef block);
};

} // namespace detail
} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
