/** \file
 * Alter implementation.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/vec.h"
#include "ql/utils/list.h"
#include "ql/ir/compat/compat.h"
#include "ql/rmgr/manager.h"
#include "options.h"
#include "free_cycle.h"
#include "past.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace map {
namespace detail {

/**
 * Alter: one alternative way to make two real qbits (operands of a 2-qubit
 * gate) nearest neighbor (NN).
 *
 * Of these two qubits, the first qubit is called the source, the second is
 * called the target qubit. The Alter stores a series of real qubit indices;
 * qubits/indices are equivalent to the nodes in the grid. An Alter represents
 * a 2-qubit gate and a path through the grid from source to target qubit, with
 * each hop between qubits/nodes only between neighboring nodes in the grid. The
 * intention is that all but one of the hops translate into swaps, and that the
 * one hop that remains will be the place to do the 2-qubit gate.
 *
 * Actually, the Alter goes through several stages.
 *
 *  - First, for the given 2-qubit gate that is stored in targetgp, while
 *    finding a path from its source to its target, the current path is kept in
 *    total. from_source, from_target, past and score are not used; past is a
 *    clone of the main past.
 *  - Paths are found starting from the source node, and aiming to reach the
 *    target node, each time adding one additional hop to the path. from_source,
 *    from_target, and score are still empty and not used.
 *  - Each time another continuation of the path is found, the current Alter is
 *    cloned and the difference continuation represented in the total attribute;
 *    it all starts with an empty Alter. from_source, from_target, and score are
 *    still empty and not used.
 *  - Once all alternative total paths for the 2-qubit gate from source to
 *    target have been found, each of these is split again in all possible ways
 *    (to ILP overlap swaps from source and target); the split is the place
 *    where the two-qubit gate is put.
 *  - The alternative splits are made separate Alters and for each of these the
 *    two partial paths are stored in fromSource and fromTarget. A partial path
 *    stores its starting and end nodes (so contains 1 hop less than its
 *    length). The partial path of the target operand is reversed, so it starts
 *    at the target qubit.
 *  - We add swaps to past following the recipe in fromSource and fromTarget.
 *    This extends past. Also, we compute score as the latency extension caused
 *    by these swaps.
 *
 * At the end, we have a list of Alters, each with a private Past, and a private
 * latency extension. The partial paths represent lists of swaps to be inserted.
 * The initial two-qubit gate gets the qubits at the ends of the partial paths
 * as operands. The main selection criterium from the Alters is to select the
 * one with the minimum latency extension. Having done that, the other Alters
 * can be discarded, and the selected one committed to the main Past.
 */
class Alter {
public:

    /**
     * Descriptions of resources for scheduling.
     */
    ir::compat::PlatformRef platform;

    /**
     * Kernel pointer to allow calling kernel private methods.
     */
    ir::compat::KernelRef kernel;

    /**
     * Reference to the parsed mapper pass options record.
     */
    OptionsRef options;

    /**
     * Number of qubits.
     */
    utils::UInt nq;

    /**
     * Cycle time, multiplier from cycles to nanoseconds.
     */
    utils::UInt ct;

    /**
     * The gate that this variation aims to make nearest-neighbor.
     */
    ir::compat::GateRef target_gate;

    /**
     * The full path, including source and target nodes.
     */
    utils::Vec<utils::UInt> total;

    /**
     * The partial path after split, starting at source.
     */
    utils::Vec<utils::UInt> from_source;

    /**
     * The partial path after split, starting at target, backward.
     */
    utils::Vec<utils::UInt> from_target;

    /**
     * Cloned main past, extended with swaps from this path.
     */
    Past past;

    /**
     * The latency extension caused by the path.
     */
    utils::Real score;

    /**
     * Initially false, true after assignment to score.
     */
    utils::Bool score_valid;

    /**
     * This should only be called after a virgin construction and not after
     * cloning a path.
     */
    void initialize(const ir::compat::KernelRef &k, const OptionsRef &opt);

    /**
     * Prints the state of this Alter, prefixed by s.
     */
    void print(const utils::Str &s) const;

    /**
     * Prints the state of this Alter, prefixed by s, only when the logging
     * verbosity is at least debug.
     */
    void debug_print(const utils::Str &s) const;

    /**
     * Prints a state of a whole list of Alters, prefixed by s.
     */
    static void print(const utils::Str &s, const utils::List<Alter> &la);

    /**
     * Prints a state of a whole list of Alters, prefixed by s, only when the
     * logging verbosity is at least debug.
     */
    static void debug_print(const utils::Str &s, const utils::List<Alter> &la);

    /**
     * Adds a node to the path in front, extending its length with one.
     */
    void add_to_front(utils::UInt q);

    /**
     * Add swap gates for the current path to the given past, up to the maximum
     * specified by the swap selection mode. This past can be a path-local one
     * or the main past. After having added them, schedule the result into that
     * past.
     */
    void add_swaps(Past &past, SwapSelectionMode mode) const;

    /**
     * Compute cycle extension of the current alternative in curr_past relative
     * to the given base past.
     *
     * extend can be called in a deep exploration where pasts have been
     * extended, each one on top of a previous one, starting from the base past.
     * The curr_past here is the last extended one, i.e. on top of which this
     * extension should be done; the base_past is the ultimate base past
     * relative to which the total extension is to be computed.
     *
     * Do this by adding the swaps described by this alternative to an
     * alternative-local copy of the current past. Keep this resulting past in
     * the current alternative (for later use). Compute the total extension of
     * all pasts relative to the base past, and store this extension in the
     * alternative's score for later use.
     */
    void extend(const Past &curr_past, const Past &base_past);

    /**
     * Split the path. Starting from the representation in the total attribute,
     * generate all split path variations where each path is split once at any
     * hop in it. The intention is that the mapped two-qubit gate can be placed
     * at the position of that hop. All result paths are added/appended to the
     * given result list.
     *
     * When at the hop of a split a two-qubit gate cannot be placed, the split
     * is not done there. This means at the end that, when all hops are
     * inter-core, no split is added to the result.
     *
     * distance=5   means length=6  means 4 swaps + 1 CZ gate, e.g.
     * index in total:      0           1           2           length-3        length-2        length-1
     * qubit:               2   ->      5   ->      7   ->      3       ->      1       CZ      4
     */
    void split(utils::List<Alter> &result) const;

};

} // namespace detail
} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
