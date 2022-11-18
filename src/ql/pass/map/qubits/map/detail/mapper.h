/** \file
 * OpenQL virtual to real qubit mapping and routing.
 */

#pragma once

#include <random>
#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/vec.h"
#include "ql/utils/list.h"
#include "ql/utils/map.h"
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

// Note on the use of constructors and Init functions for classes of the mapper
// -----------------------------------------------------------------------------
// Almost all classes of the mapper have one or more members that require
// initialization using a value that was passed on to the Mapper.initialize
// function as a parameter (i.e. platform, cycle_time). Dealing with those
// initializations in the nested constructors was cumbersome. Hence, the
// constructors create just skeleton objects which need explicit initialization
// before use. Such initialization is provided by a class local initialize
// function for a virgin object, or by copying an existing object to it.
// The constructors are trivial by this and can be synthesized by default.
//
// Construction of skeleton objects requires the used classes to provide such
// (non-parameterized) constructors.

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

/**
 * String conversion for PathStrategy.
 */
std::ostream &operator<<(std::ostream &os, PathStrategy p);

/**
 * Mapper: map operands of gates and insert swaps so that two-qubit gate
 * operands are nearest-neighbor (NN).
 *
 * All gates must be unary or two-qubit gates. The operands are virtual qubit
 * indices. After mapping, all virtual qubit operands have been mapped to real
 * qubit operands.
 *
 * For the mapper to work, the number of virtual qubits (nvq) must be less equal
 * to the number of real qubits (nrq): nvq <= nrq; the mapper assumes that the
 * virtual qubit operands (vqi) are encoded as a number 0 <= vqi < nvq, and that
 * the real qubit operands (rqi) are encoded as a number 0 <= rqi < nrq. The nrq
 * is given by the platform, nvq is given by the kernel. The mapper ignores the
 * latter (0 <= vqi < nvq was tested when creating the gates), and assumes vqi,
 * nvq, rqi and nrq to be of the same type (utils::UInt) 0<=qi<nrq. Because of
 * this, it makes no difference between nvq and nrq, and refers to both as nq,
 * and initializes the latter from the platform. All maps mapping virtual and
 * real qubits to something are of size nq.
 *
 * Mapping maps each used virtual qubit to a real qubit index, but which one
 * that is, may change. For a 2-qubit gate its operands should be
 * nearest-neighbor; when its virtual operand qubits are not mapping to nearest
 * neighbors, that should be accomplished by moving/swapping the virtual qubits
 * from their current real qubits to real qubits that are nearest neighbors:
 * those moves/swaps are inserted just before that 2-qubit gate. Anyhow, the
 * virtual operand qubits of gates must be mapped to the real ones, holding
 * their state.
 *
 * While mapping, the relationship between virtual and real qubits is tracked by
 * a QubitMapping object. This tracks two things.
 *
 *  - The actual virtual to real qubit map `v2r`. The
 *    map is initialized one to one (virtual qubit index i maps to real qubit index i)
 *    before routing. Which real qubit a virtual qubit maps to is accessed with operator[].
 *    The reverse operation also exists (get_virtual()).
 *    At any time, the virtual to real and reverse maps are 1-1 for all qubits.
 *
 *  - The liveness/state of each real qubit. This can be none (the state is
 *    garbage), initialized (the state is |0>), or live (the state is anything
 *    in use by the kernel). The states start out as none or initialized,
 *    depending on whether the mapper is configured such that it's allowed to
 *    assume that the qubits have already been initialized before the start of
 *    the kernel. Any quantum gate presented to the mapper by the input circuit
 *    puts the state of its qubits into the live state, except prepz if the
 *    mapper is configured such that it treats it as an initialization. The
 *    initialized state is used to replace swap gates (3 CNOTs) with moves
 *    (2 CNOTs): if either qubit being routed through is initialized rather than
 *    live, a move is inserted (if moves are enabled at all via configuration)
 *    rather than a swap, and if either qubit being routed through is in the
 *    garbage state, the mapper will see if initializing it does not increase
 *    circuit length too much (the threshold is configurable).
 *
 * Classical registers are ignored by the mapper currently. TO BE DONE.
 *
 * The mapping is done in the context of a grid of qubits defined by the given
 * platform. The information about this grid lives in platform.topology.
 *
 * Each kernel in the program is independently mapped (see the map_kernel
 * method), ignoring inter-kernel control flow and thereby the requirement to
 * pass on the current mapping.
 *
 * Anticipating inter-kernel mapping, the mapper maintains a kernel input
 * mapping coming from the context, and produces a kernel output mapping for the
 * context; the mapper updates the kernel's circuit from virtual to real.
 *
 * Without inter-kernel control flow, the flow is as follows.
 *  - Mapping starts from a 1 to 1 mapping of virtual to real qubits (the kernel
 *    input mapping), in which all virtual qubits are initialized to a fixed
 *    constant state (|0>/inited), suitable for replacing swap by move.
 *  - Use heuristics to map the input,
 *    mapping the virtual gates to (sets of) real gates, and outputing the new
 *    map and the new virtuals' state
 *
 * Inter-kernel control flow and consequent mapping dependence between kernels
 * is not implemented. TO BE DONE. The design of mapping multiple kernels is as
 * follows (TO BE ADAPTED TO NEW REALSTATE).
 *
 *  - Initially the program wide initial mapping is a 1 to 1 mapping of virtual
 *    to real qubits.
 *  - When starting to map a kernel, there is a set of already mapped kernels,
 *    and a set of not yet mapped kernels. Of each mapped kernel, there is an
 *    output mapping, i.e. the mapping of virts to reals with the rs per
 *    virtual. The current kernel has a set of kernels which are direct
 *    predecessor in the program's control flow, a subset of those direct
 *    predecessors thus has been mapped and another subset not mapped; the
 *    output mappings of the mapped predecessor kernels are input.
 *  - Unify these multiple input mappings to a single one; this may introduce
 *    swaps on the control flow edges. The result is the input mapping of the
 *    current kernel; keep it for later reference.
 *  - Use heuristics to map the input (or what initial placement left to do).
 *  - When done, keep the output mapping as the kernel's output mapping. For all
 *    mapped successor kernels, compute a transition from output to their input,
 *    and add it to the edge; the edge code must be optimized for:
 *     - being empty: nothing needs to be done
 *     - having a source with one succ; the edge code can be appended to that
 *       succ
 *     - having a target with one pred; the edge code can be prepended to that
 *       pred
 *     - otherwise, a separate intermediate kernel for the transition code must
 *       be created, and added.
 *
 * THE ABOVE INTER-KERNEL MAPPING IS NOT IMPLEMENTED.
 *
 * The Mapper's main entry is map_kernel which manages the input and output
 * streams of QASM instructions. It selects the
 * quantum gates from it, and maps these in the context of what was mapped
 * before (the Past). Each gate is separately mapped in MapGate in the main
 * Past's context.
 */
class Mapper {
private:

    /**
     * Current platform: topology and gate definitions.
     */
    ir::PlatformRef platform;

    /**
     * (copy of) current kernel (class) with free private circuit and methods.
     * Primarily used to create gates in Past; Past is part of Mapper and of
     * each Alter.
     */
    ir::BlockBaseRef block;

    /**
     * Parsed mapper pass options structure.
     */
    OptionsRef options;

    /**
     * Random-number generator for the "random" tie-breaking option.
     */
    std::mt19937 rng;

    /**
     * Routing progress tracker.
     */
    utils::Progress routing_progress;

    /**
     * Number of swaps added (including moves) to the most recently mapped
     * kernel, set by map_kernel().
     */
    utils::UInt num_swaps_added;

    /**
     * Number of moves added to the most recently mapped kernel, set by
     * map_kernel().
     */
    utils::UInt num_moves_added;

    /**
     * Qubit mapping before mapping, set by map_kernel().
     */
    com::map::QubitMapping v2r_in;

    /**
     * Qubit mapping after mapping, set by map_kernel().
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
        utils::RawPtr<Path> path,
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
     *    be done. After splitting each alternative contains two lists, one
     *    before and one after (reversed) the envisioned two-qubit gate; all
     *    result alternatives are such that a two-qubit gate can be placed at
     *    the split
     *
     * The end result is a list of alternatives (in alters) suitable for being
     * evaluated for any routing metric.
     */
    utils::List<Alter> gen_shortest_paths(
        const ir::CustomInstructionRef &gate,
        utils::UInt src,
        utils::UInt tgt
    );

    /**
     * Generates all possible variations of making the given gate
     * nearest-neighbor, starting from given past (with its mappings), and
     * return the found variations by appending them to the given list of
     * Alters.
     */
    utils::List<Alter> gen_alters_gate(
        const ir::CustomInstructionRef &gate,
        Past &past
    );

    /**
     * Generates all possible variations of making the given gates
     * nearest-neighbor, starting from given past (with its mappings), and
     * return the found variations by appending them to the given list of
     * Alters. Depending on the lookahead option, only take the first (most
     * critical) gate, or take all gates.
     */
    utils::List<Alter> gen_alters(const utils::List<ir::CustomInstructionRef> &gates, Past &past);

    /**
     * Seeds the random number generator with the current time in microseconds.
     */
    void random_init();

    /**
     * Chooses an Alter from the list based on the configured tie-breaking
     * strategy.
     */
    Alter tie_break_alter(utils::List<Alter> &alters, Future &future);

    /**
     * Map the gate/operands of a gate that has been routed or doesn't require
     * routing.
     */
    void map_routed_gate(const ir::CustomInstructionRef &gate, Past &past);

    /**
     * Commit the given Alter, generating swaps in the past and taking it out
     * of future when done with it. Depending on configuration, this might not
     * actually place the target gate for the given alternative yet, because
     * only part of the swap chain is generated; in this case, swaps are added
     * to past, but future is not updated.
     */
    void commit_alter(Alter &alter, Future &future, Past &past);

    /**
     * Find gates available for scheduling that do not require routing, take
     * them out, and map them. Ultimately, either no available gates remain, or
     * only gates that require routing remain. Return false when no gates remain
     * at all, and true when any gates remain; those gates are returned in
     * the gates list.
     *
     * Behavior depends on the value of the lookahead_mode option and on
     * also_nn_two_qubit_gate. When the latter is true, and lookahead_mode is...
     *  - DISABLED: while next instruction in circuit has strictly less than 2 qubit
     *    operands, output that gate.
     *    Return when we find a two-qubit gate (nearest-neighbor or not).
     *  - ONE_QUBIT_GATE_FIRST: while next in circuit is non-quantum or
     *    single-qubit, map that gate. Return the most critical two-qubit gate
     *    (nearest-neighbor or not).
     *  - NO_ROUTING_FIRST: while next in circuit is non-quantum, single-qubit,
     *    or nearest-neighbor two-qubit, map that gate. Return the most critical
     *    non-nearest-neighbor two-qubit gate.
     *  - ALL: while next in circuit is non-quantum, single-qubit, or
     *    nearest-neighbor two-qubit, map that gate. Return ALL
     *    non-nearest-neighbor two-qubit gates.
     *
     * When also_nn_two_qubit_gate is false, behavior is the same, except
     * nearest-neighbor two-qubit gates behave as if they're not
     * nearest-neighbor.
     */
    utils::List<ir::CustomInstructionRef> map_mappable_gates(
        Future &future,
        Past &past,
        utils::Bool also_nn_two_qubit_gates
    );

    /**
     * Select an Alter based on the selected heuristic.
     *
     *  - If BASE[_RC], consider all Alters as equivalent, and thus apply the
     *    tie-breaking strategy to all.
     *  - If MIN_EXTEND[_RC], consider Alters with the minimal cycle extension
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
     * Given the states of past and future, map all mappable gates and find the
     * non-mappable ones. For those, evaluate what to do next and do it. During
     * recursion, comparison is done with the base past (bottom of recursion
     * stack), and past is the last past (top of recursion stack) relative to
     * which the mapping is done.
     */
    utils::Any<ir::Statement> route_gates(Future &future, Past &past);
    
    /**
     * Map the kernel's circuit's gates in the provided context (v2r maps),
     * updating circuit and v2r maps.
     */
    void route(ir::BlockBaseRef block, com::map::QubitMapping &v2r);

    /**
     * Initialize the data structures in this class that don't change from
     * kernel to kernel.
     */
    void initialize(const ir::PlatformRef &p, const OptionsRef &opt);

    /**
     * Runs routing and decomposition to primitives for
     * the given kernel.
     */
    void map_block(ir::BlockBaseRef block);

public:

    /**
     * Runs mapping for the given program.
     *
     * TODO: inter-kernel mapping is NOT SUPPORTED; each kernel is mapped
     *  individually. That means that the resulting program is garbage if any
     *  quantum state was originally maintained from kernel to kernel!
     */
    void map(const ir::Ref &ir, const OptionsRef &opt);
};

} // namespace detail
} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
