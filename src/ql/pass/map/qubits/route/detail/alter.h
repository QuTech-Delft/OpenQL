/** \file
 * Alter implementation.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/vec.h"
#include "ql/utils/list.h"
#include "ql/plat/platform.h"
#include "ql/plat/resource/manager.h"
#include "ql/ir/ir.h"
#include "options.h"
#include "free_cycle.h"
#include "past.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace route {
namespace detail {

// =========================================================================================
// Alter: one alternative way to make two real qbits (operands of a 2-qubit gate) nearest neighbor (NN);
// of these two qubits, the first qubit is called the source, the second is called the target qubit.
// The Alter stores a series of real qubit indices; qubits/indices are equivalent to the nodes in the grid.
// An Alter represents a 2-qubit gate and a path through the grid from source to target qubit, with each hop between
// qubits/nodes only between neighboring nodes in the grid; the intention is that all but one hops
// translate into swaps and that one hop remains that will be the place to do the 2-qubit gate.
//
// Actually, the Alter goes through several stages:
// - first, for the given 2-qubit gate that is stored in targetgp,
//   while finding a path from its source to its target, the current path is kept in total;
//   fromSource, fromTarget, past and score are not used; past is a clone of the main past
// - paths are found starting from the source node, and aiming to reach the target node,
//   each time adding one additional hop to the path
//   fromSource, fromTarget, and score are still empty and not used
// - each time another continuation of the path is found, the current Alter is cloned
//   and the difference continuation represented in the total attribute; it all starts with an empty Alter
//   fromSource, fromTarget, and score are still empty and not used
// - once all alternative total paths for the 2-qubit gate from source to target have been found
//   each of these is split again in all possible ways (to ILP overlap swaps from source and target);
//   the split is the place where the two-qubit gate is put
// - the alternative splits are made separate Alters and for each
//   of these the two partial paths are stored in fromSource and fromTarget;
//   a partial path stores its starting and end nodes (so contains 1 hop less than its length);
//   the partial path of the target operand is reversed, so starts at the target qubit
// - then we add swaps to past following the recipee in fromSource and fromTarget; this extends past;
//   also we compute score as the latency extension caused by these swaps
//
// At the end, we have a list of Alters, each with a private Past, and a private latency extension.
// The partial paths represent lists of swaps to be inserted.
// The initial two-qubit gate gets the qubits at the ends of the partial paths as operands.
// The main selection criterium from the Alters is to select the one with the minimum latency extension.
// Having done that, the other Alters can be discarded and the selected one committed to the main Past.
class Alter {
public:
    plat::PlatformRef       platformp;   // descriptions of resources for scheduling
    ir::KernelRef           kernelp;     // kernel pointer to allow calling kernel private methods
    OptionsRef              options;     // parsed mapper pass options
    utils::UInt             nq;          // width of Past and Virt2Real map is number of real qubits
    utils::UInt             ct;          // cycle time, multiplier from cycles to nano-seconds

    ir::GateRef             targetgp;    // gate that this variation aims to make NN
    utils::Vec<utils::UInt> total;       // full path, including source and target nodes
    utils::Vec<utils::UInt> fromSource;  // partial path after split, starting at source
    utils::Vec<utils::UInt> fromTarget;  // partial path after split, starting at target, backward

    Past                    past;        // cloned main past, extended with swaps from this path
    utils::Real             score;       // e.g. latency extension caused by the path
    utils::Bool             didscore;    // initially false, true after assignment to score

    // explicit Alter constructor
    // needed for virgin construction
    Alter();

    // Alter initializer
    // This should only be called after a virgin construction and not after cloning a path.
    void Init(const plat::PlatformRef &p, const ir::KernelRef &k, const OptionsRef &opt);

    // printing facilities of Paths
    // print path as hd followed by [0->1->2]
    // and then followed by "implying" swap(q0,q1) swap(q1,q2)
    static void partialPrint(const utils::Str &hd, const utils::Vec<utils::UInt> &pp);

    void DPRINT(const utils::Str &s) const;
    void Print(const utils::Str &s) const;

    static void DPRINT(const utils::Str &s, const utils::Vec<Alter> &va);
    static void Print(const utils::Str &s, const utils::Vec<Alter> &va);

    static void DPRINT(const utils::Str &s, const utils::List<Alter> &la);
    static void Print(const utils::Str &s, const utils::List<Alter> &la);

    // add a node to the path in front, extending its length with one
    void Add2Front(utils::UInt q);

    // add to a max of maxnumbertoadd swap gates for the current path to the given past
    // this past can be a path-local one or the main past
    // after having added them, schedule the result into that past
    void AddSwaps(Past &past, SwapSelectionMode mapselectswapsopt) const;

    // compute cycle extension of the current alternative in prevPast relative to the given base past
    //
    // Extend can be called in a deep exploration where pasts have been extended
    // each one on top of a previous one, starting from the base past;
    // the currPast here is the last extended one, i.e. on top of which this extension should be done;
    // the basePast is the ultimate base past relative to which the total extension is to be computed.
    //
    // Do this by adding the swaps described by this alternative
    // to an alternative-local copy of the current past;
    // keep this resulting past in the current alternative (for later use);
    // compute the total extension of all pasts relative to the base past
    // and store this extension in the alternative's score for later use
    void Extend(const Past &currPast, const Past &basePast);

    // split the path
    // starting from the representation in the total attribute,
    // generate all split path variations where each path is split once at any hop in it
    // the intention is that the mapped two-qubit gate can be placed at the position of that hop
    // all result paths are added/appended to the given result list
    //
    // when at the hop of a split a two-qubit gate cannot be placed, the split is not done there
    // this means at the end that, when all hops are inter-core, no split is added to the result
    //
    // distance=5   means length=6  means 4 swaps + 1 CZ gate, e.g.
    // index in total:      0           1           2           length-3        length-2        length-1
    // qubit:               2   ->      5   ->      7   ->      3       ->      1       CZ      4
    void Split(utils::List<Alter> &resla) const;

};

} // namespace detail
} // namespace route
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
