/** \file
 * OpenQL virtual to real qubit mapping and routing.
 */

#include "mapper.h"

#include <chrono>
#include "ql/utils/filesystem.h"
#include "ql/pass/ana/statistics/annotations.h"
#include "ql/pass/map/qubits/place_mip/detail/algorithm.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace map {
namespace detail {

/**
 * String conversion for PathStrategy.
 */
std::ostream &operator<<(std::ostream &os, PathStrategy p) {
    switch (p) {
        case PathStrategy::ALL:        os << "all";        break;
        case PathStrategy::LEFT:       os << "left";       break;
        case PathStrategy::RIGHT:      os << "right";      break;
        case PathStrategy::LEFT_RIGHT: os << "left-right"; break;
    }
    return os;
}


using namespace utils;
using namespace com;

/**
 * Find shortest paths between src and tgt in the grid, bounded by a
 * particular strategy. budget is the maximum number of hops allowed in the
 * path from src and is at least distance to tgt, but can be higher when not
 * all hops qualify for doing a two-qubit gate or to find more than just the
 * shortest paths. This recursively calls itself with src replaced with its
 * neighbors (and additional bookkeeping) until src equals tgt, adding all
 * alternatives to the alters list as it goes.
 */
void Mapper::gen_shortest_paths(
    const ir::compat::GateRef &gate,
    UInt src,
    UInt tgt,
    UInt budget,
    List<Alter> &alters,
    PathStrategy strategy
) {

    // List that will get the result of a recursive gen_shortest_paths() call.
    List<Alter> sub_alters;

    QL_DOUT("gen_shortest_paths: src=" << src << " tgt=" << tgt << " budget=" << budget << " which=" << strategy);
    QL_ASSERT(alters.empty());

    if (src == tgt) {

        // Found target qubit. Create a virgin Alter and initialize it to become
        // an empty path, then add src to this path (so that it becomes a
        // distance 0 path with one qubit, src) and add the Alter to the result
        // list.
        Alter a;
        a.initialize(kernel, options);
        a.target_gate = gate;
        a.add_to_front(src);
        alters.push_back(a);
        a.debug_print("... empty path after adding to result list");
        Alter::debug_print("... result list after adding empty path", alters);
        QL_DOUT("... will return now");
        return;
    }

    // Start looking around at neighbors for serious paths.
    UInt d = platform->topology->get_distance(src, tgt);
    QL_DOUT("gen_shortest_paths: distance(src=" << src << ", tgt=" << tgt << ") = " << d);
    QL_ASSERT(d >= 1);

    // Reduce neighbors nbs to those n continuing a path within budget.
    // src=>tgt is distance d, budget>=d is allowed, attempt src->n=>tgt
    // src->n is one hop, budget from n is one less so distance(n,tgt) <= budget-1 (i.e. distance < budget)
    // when budget==d, this defaults to distance(n,tgt) <= d-1
    auto neighbors = platform->topology->get_neighbors(src);
    neighbors.remove_if([this,budget,tgt](const UInt& n) { return platform->topology->get_distance(n, tgt) >= budget; });
    if (logger::log_level >= logger::LogLevel::LOG_DEBUG) {
        QL_DOUT("gen_shortest_paths: ... after reducing to steps within budget, nbl: ");
        for (auto dn : neighbors) {
            QL_DOUT("..." << dn << " ");
        }
    }

    // Rotate neighbor list nbl such that largest difference between angles of
    // adjacent elements is beyond back(). This only makes sense when there is
    // an underlying xy grid; when not, only the ALL strategy is supported.
    QL_ASSERT(platform->topology->has_coordinates() || strategy == PathStrategy::ALL);
    platform->topology->sort_neighbors_by_angle(src, neighbors);
    // subset to those neighbors that continue in direction(s) we want
    if (strategy == PathStrategy::LEFT) {
        neighbors.remove_if([neighbors](const UInt &n) { return n != neighbors.front(); } );
    } else if (strategy == PathStrategy::RIGHT) {
        neighbors.remove_if([neighbors](const UInt &n) { return n != neighbors.back(); } );
    } else if (strategy == PathStrategy::LEFT_RIGHT) {
        neighbors.remove_if([neighbors](const UInt &n) { return n != neighbors.front() && n != neighbors.back(); } );
    }

    QL_IF_LOG_DEBUG {
        QL_DOUT("gen_shortest_paths: ... after normalizing, before iterating, nbl: ");
        for (auto dn : neighbors) {
            QL_DOUT("..." << dn << " ");
        }
    }

    // For all resulting neighbors, find all continuations of a shortest path by
    // recursively calling ourselves.
    for (auto &n : neighbors) {
        PathStrategy new_strategy = strategy;

        // For each neighbor, only look in desired direction, if any.
        if (strategy == PathStrategy::LEFT_RIGHT && neighbors.size() != 1) {
            // When looking both left and right still, and there is a choice
            // now, split into left and right.
            if (n == neighbors.front()) {
                new_strategy = PathStrategy::LEFT;
            } else {
                new_strategy = PathStrategy::RIGHT;
            }
        }

        // Get list of possible paths in budget-1 from n to tgt in sub_alters.
        gen_shortest_paths(gate, n, tgt, budget - 1, sub_alters, new_strategy);

        // Move all of sub_alters to alters, and make sub_alters empty.
        alters.splice(alters.end(), sub_alters);

    }

    // alters now contains all paths starting from a neighbor of src to tgt
    // (within budget). Add src to front of all to-be-returned paths from src's
    // neighbors to tgt.
    for (auto &a : alters) {
        QL_DOUT("... gen_shortest_paths, about to add src=" << src << " in front of path");
        a.add_to_front(src);
    }
    QL_DOUT("... gen_shortest_paths: returning from call of: " << "src=" << src << " tgt=" << tgt << " budget=" << budget << " which=" << strategy);
}

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
void Mapper::gen_shortest_paths(const ir::compat::GateRef &gate, UInt src, UInt tgt, List<Alter> &alters) {

    // List that will hold all not-yet-split Alters directly from src to tgt.
    List<Alter> direct_paths;

    // Compute budget.
    UInt budget = platform->topology->get_min_hops(src, tgt);

    // Generate paths using the configured path selection strategy.
    if (options->path_selection_mode == PathSelectionMode::ALL) {
        gen_shortest_paths(gate, src, tgt, budget, direct_paths, PathStrategy::ALL);
    } else if (options->path_selection_mode == PathSelectionMode::BORDERS) {
        gen_shortest_paths(gate, src, tgt, budget, direct_paths, PathStrategy::LEFT_RIGHT);
    } else {
        QL_FATAL("Unknown value of path selection mode option " << options->path_selection_mode);
    }

    // Split the paths, outputting to the alters list.
    for (auto &a : direct_paths) {
        a.split(alters);
    }

}

/**
 * Generates all possible variations of making the given gate
 * nearest-neighbor, starting from given past (with its mappings), and
 * return the found variations by appending them to the given list of
 * Alters.
 */
void Mapper::gen_alters_gate(const ir::compat::GateRef &gate, List<Alter> &alters, Past &past) {

    // Interpret virtual operands in past's current map.
    auto &q = gate->operands;
    QL_ASSERT (q.size() == 2);
    UInt src = past.map_qubit(q[0]);
    UInt tgt = past.map_qubit(q[1]);

    QL_DOUT("gen_alters_gate: " << gate->qasm() << " in real (q" << src << ",q" << tgt << ") at get_min_hops=" << platform->topology->get_min_hops(src, tgt));
    past.debug_print_fc();

    // Find shortest paths from src to tgt, and split these.
    gen_shortest_paths(gate, src, tgt, alters);
    QL_ASSERT(!alters.empty());
    // Alter::DPRINT("... after GenShortestPaths", la);

}

/**
 * Generates all possible variations of making the given gates
 * nearest-neighbor, starting from given past (with its mappings), and
 * return the found variations by appending them to the given list of
 * Alters. Depending on the lookahead option, only take the first (most
 * critical) gate, or take all gates.
 */
void Mapper::gen_alters(
    const utils::List<ir::compat::GateRef> &gates,
    List<Alter> &alters,
    Past &past
) {
    if (options->lookahead_mode == LookaheadMode::ALL) {

        // Create alternatives for each gate.
        QL_DOUT("gen_alters, " << gates.size() << " 2q gates; create an alternative for each");
        for (const auto &gate : gates) {

            // Generate all possible variations to make gate nearest-neighbor
            // in current v2r mapping ("past").
            QL_DOUT("gen_alters: create alternatives for: " << gate->qasm());
            gen_alters_gate(gate, alters, past);

        }

    } else {

        // Only take the first gate in in the list, the most critical one, and
        // generate alternatives for it.
        ir::compat::GateRef gate = gates.front();

        // Generate all possible variations to make gate nearest-neighbor, in
        // current v2r mapping ("past").
        QL_DOUT("gen_alters, " << gates.size() << " 2q gates; take first: " << gate->qasm());
        gen_alters_gate(gate, alters, past);

    }
}

/**
 * Seeds the random number generator with the current time in microseconds.
 */
void Mapper::random_init() {
    auto ts = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    // QL_DOUT("Seeding random generator with " << ts );
    rng.seed(ts);
}

/**
 * Chooses an Alter from the list based on the configured tie-breaking
 * strategy.
 */
Alter Mapper::tie_break_alter(List<Alter> &alters, Future &future) {
    QL_ASSERT(!alters.empty());

    if (alters.size() == 1) {
        return alters.front();
    }

    switch (options->tie_break_method) {
        case TieBreakMethod::CRITICAL: {
            List<ir::compat::GateRef> lag;
            for (auto &a : alters) {
                lag.push_back(a.target_gate);
            }
            ir::compat::GateRef gate = future.get_most_critical(lag);
            QL_ASSERT(!gate.empty());
            for (auto &a : alters) {
                if (a.target_gate.get_ptr() == gate.get_ptr()) {
                    // QL_DOUT(" ... took first alternative with most critical target gate");
                    return a;
                }
            }
            return alters.front();
        }

        case TieBreakMethod::RANDOM: {
            Alter res;
            std::uniform_int_distribution<> dis(0, (alters.size() - 1));
            UInt choice = dis(rng);
            UInt i = 0;
            for (auto &a : alters) {
                if (i == choice) {
                    res = a;
                    break;
                }
                i++;
            }
            // QL_DOUT(" ... took random draw " << choice << " from 0.." << (la.size()-1));
            return res;
        }

        case TieBreakMethod::LAST:
            return alters.back();

        case TieBreakMethod::FIRST:
        default:
            return alters.front();

    }
}

/**
 * Map the gate/operands of a gate that has been routed or doesn't require
 * routing.
 */
void Mapper::map_routed_gate(const ir::compat::GateRef &gate, Past &past) {
    QL_DOUT("map_routed_gate on virtual: " << gate->qasm());

    // make_real() maps the qubit operands of the gate and optionally updates
    // its gate name. When the gate name was updated, a new gate with that name
    // is created; when that new gate is a composite gate, it is immediately
    // decomposed (by gate creation). The resulting gate/expansion (anyhow a
    // sequence of gates) is collected in circuit.
    ir::compat::GateRefs gates;
    past.make_real(gate, gates);
    for (const auto &new_gate : gates) {
        QL_DOUT(" ... new mapped real gate, about to be added to past: " << new_gate->qasm());
        past.add_and_schedule(new_gate);
    }

}

/**
 * Commit the given Alter, generating swaps in the past and taking it out
 * of future when done with it. Depending on configuration, this might not
 * actually place the target gate for the given alternative yet, because
 * only part of the swap chain is generated; in this case, swaps are added
 * to past, but future is not updated.
 */
void Mapper::commit_alter(Alter &alter, Future &future, Past &past) {

    // The target two-qubit-gate, now not yet nearest-neighbor.
    ir::compat::GateRef target = alter.target_gate;
    alter.debug_print("... commit_alter, alternative to commit, will add swaps and then map target 2q gate");

    // Add swaps to the past to make the target gate nearest-neighbor.
    alter.add_swaps(past, options->swap_selection_mode);

    // When only some swaps were added (based on configuration), the target
    // might not yet be nearest-neighbor, so recheck.
    auto &q = target->operands;
    if (platform->topology->get_min_hops(past.map_qubit(q[0]), past.map_qubit(q[1])) == 1) {

        // Target is nearest-neighbor, so we;re done with this gate.
        // QL_DOUT("... CommitAlter, target 2q is NN, map it and done: " << resgp->qasm());
        map_routed_gate(target, past);
        future.completed_gate(target);

    } else {
        // QL_DOUT("... CommitAlter, target 2q is not NN yet, keep it: " << resgp->qasm());
    }

}

/**
 * Find gates available for scheduling that do not require routing, take
 * them out, and map them. Ultimately, either no available gates remain, or
 * only gates that require routing remain. Return false when no gates remain
 * at all, and true when any gates remain; those gates are returned in
 * the gates list.
 *
 * Behavior depends on the value of the lookahead_mode option and on
 * also_nn_two_qubit_gate. When the latter is true, and lookahead_mode is...
 *  - DISABLED: while next in circuit is non-quantum or single-qubit, map
 *    that gate. Return when we find a two-qubit gate (nearest-neighbor or
 *    not). In this case, get_non_quantum_gates only returns a non-quantum
 *    gate when it is next in circuit.
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
Bool Mapper::map_mappable_gates(
    Future &future,
    Past &past,
    List<ir::compat::GateRef> &gates,
    Bool also_nn_two_qubit_gates
) {

    // List of non-quantum gates in avlist.
    List<ir::compat::GateRef> av_non_quantum_gates;

    // List of (remaining) gates in avlist.
    List<ir::compat::GateRef> av_gates;

    QL_DOUT("map_mappable_gates entry");
    while (true) {

        // Handle non-quantum gates that need to be done first.
        if (future.get_non_quantum_gates(av_non_quantum_gates)) {

            // List of gates available for scheduling contains non-quantum
            // gates, and get_non_quantum_gates() indicates these (in
            // av_non_quantum_gates) must be done first.
            QL_DOUT("map_mappable_gates, there is a set of non-quantum gates");
            for (const auto &gate : av_non_quantum_gates) {

                // here add code to map qubit use of any non-quantum instruction????
                // dummy gates are nonq gates internal to OpenQL such as SOURCE/SINK; don't output them
                if (gate->type() != ir::compat::GateType::DUMMY) {
                    // past only can contain quantum gates, so non-quantum gates must by-pass Past
                    past.bypass(gate);    // this flushes past.lg first to outlg
                }
                future.completed_gate(gate); // so on avlist= nonNN2q -> NN2q -> 1q -> nonq: the nonq is done first
                QL_DOUT("map_mappable_gates, done with " << gate->qasm());

            }
            QL_DOUT("map_mappable_gates, done with set of non-quantum gates, continuing ...");
            continue;

        }

        // Get available gates.
        if (!future.get_gates(av_gates)) {

            // No more gates available for scheduling.
            QL_DOUT("map_mappable_gates, no gates anymore, return");
            gates.clear();
            return false;

        }

        // Quantum gates are available for scheduling, and
        // get_non_quantum_gates/get_gates indicate these (in av_gates) must be
        // done now. Look for gates that never require routing (single-qubit and
        // wait gates).
        Bool found = false;
        for (const auto &gate : av_gates) {
            if (gate->type() == ir::compat::GateType::WAIT || gate->operands.size() == 1) {

                // A quantum gate that never requires routing was found.
                map_routed_gate(gate, past);
                future.completed_gate(gate);
                found = true;
                // so on avlist= nonNN2q -> NN2q -> 1q: the 1q is done first
                break;

            }
        }
        if (found) {
            continue;
        }

        // av_gates only contains 2q gates, that may require routing.
        if (also_nn_two_qubit_gates) {

            // When there is a 2q in qlg that is mappable already, map it.
            // When there is more than one, take most critical one first
            // (because qlg is ordered, most critical first).
            for (const auto &gate : av_gates) {

                // Interpret virtual operands in current map.
                auto &q = gate->operands;
                UInt src = past.map_qubit(q[0]);
                UInt tgt = past.map_qubit(q[1]);

                // Find minimum number of hops between real counterparts.
                UInt d = platform->topology->get_min_hops(src, tgt);

                if (d == 1) {

                    // Just one hop, so gate is already nearest-neighbor and can
                    // be mapped.
                    QL_DOUT("map_mappable_gates, NN no routing: " << gate->qasm() << " in real (q" << src << ",q" << tgt << ")");
                    map_routed_gate(gate, past);
                    future.completed_gate(gate);
                    found = true;    // a 2q quantum gate was found that was mappable
                    // so on avlist= nonNN2q -> NN2q: the NN2q is done first
                    break;

                }
            }
            if (found) {

                // Found a mappable two-qubit gate and mapped it. Don't map more
                // mappable two-qubit gates (they might not be critical, the now
                // available single-qubit gates may hide a more critical
                // two-qubit gate), but deal with all available non-quantum and
                // single-qubit gates first, and only when non of those remain,
                // map a next mappable two-qubit (now most critical) gate.
                QL_DOUT("map_mappable_gates, found and mapped an easy quantum gate, continuing ...");
                continue;

            }
            QL_DOUT("map_mappable_gates, only nonNN 2q gates remain: ...");
        } else {
            QL_DOUT("map_mappable_gates, only 2q gates remain (nonNN and NN): ...");
        }

        // Only two-qubit gates remain in the available gate list (when
        // also_nn_two_qubit_gate these are furthermore necessarily
        // non-nearest-neighbor, otherwise they might also be nearest-neighbor).
        gates = av_gates;
        QL_IF_LOG_DEBUG {
            for (const auto &gate : gates) {
                QL_DOUT("... 2q gate returned: " << gate->qasm());
            }
        }

        return true;
    }
}

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
void Mapper::select_alter(
    List<Alter> &alters,
    Alter &result,
    Future &future,
    Past &past,
    Past &base_past,
    UInt recursion_depth
) {
    // alters are all alternatives we enter with. There must be at least one.
    QL_ASSERT(!alters.empty());

    QL_DOUT("select_alter ENTRY level=" << recursion_depth << " from " << alters.size() << " alternatives");

    // Handle the basic strategy, where we just tie-break on all alters without
    // recusing.
    if (options->heuristic == Heuristic::BASE || options->heuristic == Heuristic::BASE_RC) {
        Alter::debug_print(
            "... select_alter base (equally good/best) alternatives:", alters);
        result = tie_break_alter(alters, future);
        result.debug_print("... the selected Alter is");
        // QL_DOUT("SelectAlter DONE level=" << level << " from " << la.size() << " alternatives");
        return;
    }

    QL_ASSERT(
        options->heuristic == Heuristic::MIN_EXTEND ||
        options->heuristic == Heuristic::MIN_EXTEND_RC ||
        options->heuristic == Heuristic::MAX_FIDELITY
    );

    // Compute a score for each alternative relative to base_past, and sort the
    // alternatives based on it, minimum first.
    for (auto &a : alters) {
        a.debug_print("Considering extension by alternative: ...");
        a.extend(past, base_past);           // locally here, past will be cloned and kept in alter
        // and the extension stored into the a.score
    }
    alters.sort([this](const Alter &a1, const Alter &a2) { return a1.score < a2.score; });
    Alter::debug_print(
        "... select_alter sorted all entry alternatives after extension:", alters);

    // Reduce sorted list of alternatives to list of good alternatives, suitable
    // to find in recursion which is/are really best. This need not be only
    // those with minimum cycle extension; it can be more, based on the
    // recursion_width_factor option. However, this easily, as each level
    // exponentially builds more alternatives. To curb this,
    // recursion_width_exponent can be set to a value less than one, to
    // multiplicatively reduce the width for each recursion level.
    List<Alter> good_alters = alters;
    good_alters.remove_if([this,alters](const Alter& a) { return a.score != alters.front().score; });
    Real factor = options->recursion_width_factor * utils::pow(options->recursion_width_exponent, recursion_depth);
    Real keep_real = utils::max(1.0, utils::ceil(factor * good_alters.size()));
    UInt keep = (keep_real < static_cast<utils::Real>(utils::MAX)) ? static_cast<utils::UInt>(keep_real) : utils::MAX;
    if (keep != good_alters.size()) {
        if (keep < alters.size()) {
            good_alters = alters;
            List<Alter>::iterator ia;
            ia = good_alters.begin();
            advance(ia, keep);
            good_alters.erase(ia, good_alters.end());
        } else {
            good_alters = alters;
        }
    }
    // QL_DOUT("SelectAlter mapselectmaxwidth=" << mapselectmaxwidthopt << " level=" << level << " reduced la to gla");
    Alter::debug_print("... select_alter good alternatives before recursion:", good_alters);

    // When maxlevel has been reached, stop the recursion, and choose from the
    // best minextend/maxfidelity alternatives.
    if (recursion_depth >= options->recursion_depth_limit) {

        // Reduce list of good alternatives to list of minextend/maxfidelity
        // best alternatives (bla), and make a choice from that list to return
        // as result.
        List<Alter> best_alters = good_alters;
        best_alters.remove_if([this,good_alters](const Alter& a) { return a.score != good_alters.front().score; });
        Alter::debug_print(
            "... select_alter reduced to best alternatives to choose result from:",
            best_alters);
        result = tie_break_alter(best_alters, future);
        result.debug_print("... the selected Alter (STOPPING RECURSION) is");
        // QL_DOUT("SelectAlter DONE level=" << level << " from " << bla.size() << " best alternatives");
        return;

    }

    // Otherwise, prepare using recursion to choose from the good alternatives,
    // i.e. make a recursion step looking ahead to decide which alternative is
    // best.
    //
    // For each good alternative, lookahead for next non-nearest-neighbor
    // two-qubit gates, and compare them for their alternative mappings; the
    // lookahead alternative with the least overall extension (i.e. relative to
    // base_past) is chosen, and the current alternative on top of which it was
    // built is chosen at the current level, unwinding the recursion.
    //
    // Recursion could stop above because the maximum depth was reached, but it
    // can also stop because the end of the circuit has been reached (i.e. no
    // non-nearest-neighbor two-qubit gates remain.
    //
    // When there is only one good alternative, we still want to know its
    // minimum extension to compare with competitors, since that is not just a
    // local figure but the extension from base_past. So with only one
    // alternative we may still go into recursion below. This means that
    // recursion always goes to the maximum depth or to the end of the circuit.
    // This anomaly may need correction.
    // QL_DOUT("... SelectAlter level=" << level << " entering recursion with " << gla.size() << " good alternatives");
    for (auto &a : good_alters) {
        a.debug_print("... ... considering alternative:");
        Future sub_future = future; // copy!
        Past sub_past = past;       // copy!
        commit_alter(a, sub_future, sub_past);
        a.debug_print(
            "... ... committed this alternative first before recursion:");

        // Whether there are still non-nearest-neighbor two-qubit gates to map.
        Bool gates_remain;

        // List of non-nearest-neighbor two-qubit gates taken from the available
        // gate list, as returned from map_mappable_gates.
        List<ir::compat::GateRef> gates;

        // In recursion, look at recurse_on_nn_two_qubit option:
        // - map_mappable_gates with recurse_on_nn_two_qubit==true is greedy and
        //   immediately maps each single-qubit and nearest-neighbor two-qubit
        //   gate;
        // - map_mappable_gates with recurse_on_nn_two_qubit==false is not
        //   greedy, mapping only the single-qubit gates.
        //
        // When true and when the lookahead mode is NO_ROUTING_FIRST or ALL, let
        // map_mappable_gates stop mapping only on finding a non-nearest
        // two-qubit gate. Otherwise, let map_mappable_gates stop mapping on any
        // two-qubit gate. This creates more clear recursion: one two-qubit gate
        // at a time, instead of a possible empty set of nearest-neighbor
        // two-qubit gates followed by a non-nearest neighbor two-qubit gate.
        // When a nearest-neighbor gate is found, this is perfect; this is not
        // seen when immediately mapping all non-nearest two-qubit gates. So the
        // goal is to prove that recurse_on_nn_two_qubit should be no at this
        // place, in the recursion step, but not at level 0!
        Bool also_nn_two_qubit_gates = options->recurse_on_nn_two_qubit
                     && (
                         options->lookahead_mode == LookaheadMode::NO_ROUTING_FIRST
                         || options->lookahead_mode == LookaheadMode::ALL
                     );

        // Map all easy gates. Remainder is returned in gates.
        gates_remain = map_mappable_gates(sub_future, sub_past, gates, also_nn_two_qubit_gates);

        if (gates_remain) {

            // End of circuit not yet reached, recurse.
            QL_DOUT("... ... select_alter level=" << recursion_depth << ", committed + mapped easy gates, now facing " << gates.size() << " 2q gates to evaluate next");

            // Generate the next set of alternative routing actions.
            List<Alter> sub_alters;
            gen_alters(gates, sub_alters, sub_past);
            QL_DOUT("... ... select_alter level=" << recursion_depth << ", generated for these 2q gates " << sub_alters.size() << " alternatives; RECURSE ... ");

            // Select the best alternative from the list by recursion.
            Alter sub_result;
            select_alter(sub_alters, sub_result, sub_future, sub_past, base_past, recursion_depth + 1);
            sub_result.debug_print("... ... select_alter, generated for these 2q gates ... ; RECURSE DONE; resulting alternative ");

            // The extension of deep recursion is treated as extension at the
            // current level; by this an alternative that started bad may be
            // compensated by deeper alters.
            a.score = sub_result.score;

        } else {

            // Reached end of circuit while speculating.
            QL_DOUT("... ... select_alter level=" << recursion_depth << ", no gates to evaluate next; RECURSION BOTTOM");
            if (options->heuristic == Heuristic::MAX_FIDELITY) {
                QL_FATAL("Mapper option maxfidelity has been disabled");
                // a.score = quick_fidelity(past_copy.lg);
            } else {
                a.score = sub_past.get_max_free_cycle() -
                          base_past.get_max_free_cycle();
            }
            a.debug_print(
                "... ... select_alter, after committing this alternative, mapped easy gates, no gates to evaluate next; RECURSION BOTTOM");

        }
        a.debug_print("... ... DONE considering alternative:");
    }

    // Sort list of good alternatives on score resulting after recursion.
    good_alters.sort([this](const Alter &a1, const Alter &a2) { return a1.score < a2.score; });
    Alter::debug_print("... select_alter sorted alternatives after recursion:", good_alters);

    // Reduce list of good alternatives of before recursion to list of equally
    // minimal best alternatives now, and make a choice from that list to return
    // as result.
    List<Alter> best_alters = good_alters;
    best_alters.remove_if([this,good_alters](const Alter& a) { return a.score != good_alters.front().score; });
    Alter::debug_print("... select_alter equally best alternatives on return of RECURSION:", best_alters);
    result = tie_break_alter(best_alters, future);
    result.debug_print("... the selected Alter is");
    // QL_DOUT("... SelectAlter level=" << level << " selecting from " << bla.size() << " equally good alternatives above DONE");
    QL_DOUT("select_alter DONE level=" << recursion_depth << " from " << alters.size() << " alternatives");

}

/**
 * Given the states of past and future, map all mappable gates and find the
 * non-mappable ones. For those, evaluate what to do next and do it. During
 * recursion, comparison is done with the base past (bottom of recursion
 * stack), and past is the last past (top of recursion stack) relative to
 * which the mapping is done.
 */
void Mapper::map_gates(Future &future, Past &past, Past &base_past) {

    // List of non-mappable gates taken from avlist, as returned by
    // map_mappable_gates.
    List<ir::compat::GateRef> gates;

    Bool also_nn_two_qubit_gates = (
        options->lookahead_mode == LookaheadMode::NO_ROUTING_FIRST
        || options->lookahead_mode == LookaheadMode::ALL
    );

    // Handle all the gates one by one. map_mappable_gates returns false when no
    // gates remain.
    while (map_mappable_gates(future, past, gates, also_nn_two_qubit_gates)) {

        // All gates in the gates list are two-qubit quantum gates that cannot
        // be mapped yet. Select which one(s) to (partially) route, according to
        // one of the known strategies. The only requirement on the code below
        // is that at least something is done that decreases the problem.

        // Generate all alternative routes.
        List<Alter> alters;
        gen_alters(gates, alters, past);

        // Select the best one based on the configured strategy.
        Alter alter;
        select_alter(alters, alter, future, past, base_past, 0);

        // Commit to selected alternative. This adds all or just one swap
        // (depending on configuration) to THIS past, and schedules them/it in.
        commit_alter(alter, future, past);

    }
}

/**
 * Performs (initial) placement of the qubits.
 */
void Mapper::place(const ir::compat::KernelRef &k, com::QubitMapping &v2r) {

    if (options->enable_mip_placer) {
#ifdef INITIALPLACE
        QL_DOUT("InitialPlace: kernel=" << k->name << " timeout=" << options->mip_timeout << " horizon=" << options->mip_horizon << " [START]");

        place_mip::detail::Options ipopt;
        ipopt.map_all = options->initialize_one_to_one;
        ipopt.horizon = options->mip_horizon;
        ipopt.timeout = options->mip_timeout;

        place_mip::detail::Algorithm ip;
        auto ipok = ip.run(k, ipopt, v2r); // compute mapping (in v2r) using ip model, may fail
        QL_DOUT("InitialPlace: kernel=" << k->name << " timeout=" << options->mip_timeout << " horizon=" << options->mip_horizon << " result=" << ipok << " iptimetaken=" << ip.get_time_taken() << " seconds [DONE]");
#else // ifdef INITIALPLACE
        QL_DOUT("InitialPlace support disabled during OpenQL build [DONE]");
        QL_WOUT("InitialPlace support disabled during OpenQL build [DONE]");
#endif // ifdef INITIALPLACE
    }
    QL_IF_LOG_DEBUG {
        QL_DOUT("After InitialPlace");
        v2r.dump_state();
    }

}

/**
 * Map the kernel's circuit's gates in the provided context (v2r maps),
 * updating circuit and v2r maps.
 */
void Mapper::route(const ir::compat::KernelRef &k, QubitMapping &v2r) {

    // Future window, presents input in available list.
    Future future;

    // Past window, contains output schedule, storing all gates until taken out.
    Past past;

    // Scheduler instance (from src/scheduler.h) used for its dependency graph.
    utils::Ptr<Scheduler> sched;
    sched.emplace();

    // Initialize the future window with the incoming circuit.
    future.initialize(platform, options);
    future.set_kernel(k, sched);

    // Future has now copied kernel->c to private data, making kernel->c ready
    // for use by Past::new_gate(), for the kludge we need because gates can
    // only be constructed in the context of and at the end of a kernel.
    k->gates.reset();
    kernel = k;
    past.initialize(kernel, options);
    past.import_mapping(v2r);

    // Perform the actual mapping.
    map_gates(future, past, past);

    // Flush all gates to the output window.
    past.flush_all();

    // Copy the gates into the kernel's circuit.
    // mainPast.DPRINT("end mapping");
    QL_DOUT("... retrieving outCirc from mainPast.outlg; swapping outCirc with kernel.c, kernel.c contains output circuit");
    k->gates.reset();
    past.flush_to_circuit(k->gates);

    // The mapper also schedules internally, including any decompositions it
    // does to make things primitive. Thus, cycle numbers are now valid.
    k->cycles_valid = true;

    // Update the virtual to real qubit mapping to what it is after all the
    // kernel's routing actions.
    past.export_mapping(v2r);

    // Store statistics gathered by the past before it goes out of scope.
    num_swaps_added = past.get_num_swaps_added();
    num_moves_added = past.get_num_moves_added();

}

/**
 * Decomposes all gates in the circuit that have a definition with _prim
 * appended to its name. The mapper does this after routing.
 */
void Mapper::decompose_to_primitives(const ir::compat::KernelRef &k) {
    QL_DOUT("decompose_to_primitives circuit ...");

    // Copy to allow kernel.c use by Past.new_gate.
    ir::compat::GateRefs circuit = k->gates;
    k->gates.reset();

    // Output window in which gates are scheduled.
    Past past;
    past.initialize(k, options);

    for (const auto &gate : circuit) {

        // Decompose gate into prim_circuit. On failure, this copies the
        // original gate directly into it.
        ir::compat::GateRefs prim_gates;
        past.make_primitive(gate, prim_gates);

        // Schedule the potentially decomposed gates.
        for (const auto &prim_gate : prim_gates) {
            past.add_and_schedule(prim_gate);
        }

    }

    // Update the output circuit based on the scheduling result.
    past.flush_all();
    past.flush_to_circuit(k->gates);
    k->cycles_valid = true;

    QL_DOUT("decompose_to_primitives circuit [DONE]");
}

/**
 * Initialize the data structures in this class that don't change from
 * kernel to kernel.
 */
void Mapper::initialize(const ir::compat::PlatformRef &p, const OptionsRef &opt) {
    // QL_DOUT("Mapping initialization ...");
    // QL_DOUT("... Grid initialization: platform qubits->coordinates, ->neighbors, distance ...");
    platform = p;
    options = opt;
    nq = p->qubit_count;
    nc = p->creg_count;
    nb = p->breg_count;
    random_init();
    // QL_DOUT("... platform/real number of qubits=" << nq << ");
    cycle_time = p->cycle_time;

    // QL_DOUT("Mapping initialization [DONE]");
}

/**
 * Runs initial placement, routing, and decomposition to primitives for
 * the given kernel.
 *
 * TODO: this should be split up into multiple passes, but this is difficult
 *  right now because:
 *   - place() does not actually modify the kernel, it just outputs a map.
 *     There is no place for that in the IR (at least not right now), so
 *     at best it could add an annotation, which would be weird. It should
 *     just update the gates immediately, but this is also problematic right
 *     now, because merely making a gate triggers decompositions.
 *   - decompose_to_primitives() could be split off relatively easily, but
 *     there's no point in doing that now, because we want a generic
 *     decomposition pass anyway, and it needs the same kludges for adding gates
 *     as the mapper does, so it (ab)uses those and is thus linked to the mapper
 *     code.
 */
void Mapper::map_kernel(const ir::compat::KernelRef &k) {
    QL_DOUT("Mapping kernel " << k->name << " [START]");
    QL_DOUT("... kernel original virtual number of qubits=" << k->qubit_count);
    kernel.reset();            // no new_gates until kernel.c has been copied

    QL_DOUT("Mapper::Map before v2r.initialize: assume_initialized=" << options->assume_initialized);

    // TODO: unify all incoming v2rs into v2r to compute kernel input mapping.
    //  Right now there is no inter-kernel mapping yet, so just take the
    //  program's initial mapping for each kernel.
    QubitMapping v2r{
        nq,
        options->initialize_one_to_one,
        options->assume_initialized ? QubitState::INITIALIZED : QubitState::NONE
    };
    QL_IF_LOG_DEBUG {
        QL_DOUT("After initialization");
        v2r.dump_state();
    }

    // Save the input qubit map for reporting.
    v2r_in = v2r;

    // Perform placement.
    place(k, v2r);

    // Save the placed qubit map for reporting. This is the resulting qubit map
    // at the *start* of the kernel.
    v2r_ip = v2r;

    // Perform heuristic routing.
    QL_DOUT("Mapper::Map before route: assume_initialized=" << options->assume_initialized);
    route(k, v2r);        // updates kernel.c with swaps, maps all gates, updates v2r map
    QL_IF_LOG_DEBUG {
        QL_DOUT("After heuristics");
        v2r.dump_state();
    }

    // Save the routed qubit map for reporting. This is the resulting qubit map
    // at the *end* of the kernel.
    v2r_out = v2r;

    // Decompose to primitive instructions as specified in the config file.
    decompose_to_primitives(k);

    k->qubit_count = nq;       // bluntly copy nq (==#real qubits), so that all kernels get the same qubit_count
    k->creg_count = nc;        // same for number of cregs and bregs, although we don't really map those
    k->breg_count = nb;

    QL_DOUT("Mapping kernel " << k->name << " [DONE]");
}

/**
 * Runs mapping for the given program.
 *
 * TODO: inter-kernel mapping is NOT SUPPORTED; each kernel is mapped
 *  individually. That means that the resulting program is garbage if any
 *  quantum state was originally maintained from kernel to kernel!
 */
void Mapper::map(const ir::compat::ProgramRef &prog, const OptionsRef &opt) {

    // Shorthand.
    using pass::ana::statistics::AdditionalStats;

    // Perform program-wide initialization.
    initialize(prog->platform, opt);

    // Map kernel by kernel, adding statistics all the while.
    UInt total_swaps = 0;
    UInt total_moves = 0;
    Real total_time_taken = 0.0;
    for (const auto &k : prog->kernels) {
        QL_IOUT("Mapping kernel: " << k->name);

        // Start interval timer for measuring time taken for this kernel.
        Real time_taken = 0.0;
        using namespace std::chrono;
        high_resolution_clock::time_point t1 = high_resolution_clock::now();

        // Actually do the mapping.
        map_kernel(k);

        // Stop the interval timer.
        high_resolution_clock::time_point t2 = high_resolution_clock::now();
        duration<Real> time_span = t2 - t1;
        time_taken = time_span.count();

        // Push mapping statistics into the kernel.
        AdditionalStats::push(k, "swaps added: " + to_string(num_swaps_added));
        AdditionalStats::push(k, "of which moves added: " + to_string(num_moves_added));
        AdditionalStats::push(k, "virt2real map before mapper:" + to_string(v2r_in.get_virt_to_real()));
        AdditionalStats::push(k, "virt2real map after initial placement:" + to_string(v2r_ip.get_virt_to_real()));
        AdditionalStats::push(k, "virt2real map after mapper:" + to_string(v2r_out.get_virt_to_real()));
        AdditionalStats::push(k, "realqubit states before mapper:" + to_string(v2r_in.get_state()));
        AdditionalStats::push(k, "realqubit states after mapper:" + to_string(v2r_out.get_state()));
        AdditionalStats::push(k, "time taken: " + to_string(time_taken));

        // Update total statistical counters.
        total_swaps += num_swaps_added;
        total_moves += num_moves_added;
        total_time_taken += time_taken;

    }

    // Push mapping statistics into the program.
    AdditionalStats::push(prog, "Total no. of swaps: " + to_string(total_swaps));
    AdditionalStats::push(prog, "Total no. of moves of swaps: " + to_string(total_moves));
    AdditionalStats::push(prog, "Total time taken: " + to_string(total_time_taken));

    // Kernel qubit/creg/breg counts will have been updated to the platform
    // counts, so we need to do the same for the program.
    prog->qubit_count = platform->qubit_count;
    prog->creg_count = platform->creg_count;
    prog->breg_count = platform->breg_count;

}

} // namespace detail
} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
