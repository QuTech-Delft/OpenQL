/** \file
 * OpenQL virtual to real qubit mapping and routing.
 */

#include "mapper.h"

#include "ql/utils/filesystem.h"
#include "ql/pass/map/qubits/place_mip/detail/algorithm.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace route {
namespace detail {

using namespace utils;
using namespace com;

// Find shortest paths between src and tgt in the grid, bounded by a particular strategy (which);
// budget is the maximum number of hops allowed in the path from src and is at least distance to tgt;
// it can be higher when not all hops qualify for doing a two-qubit gate or to find more than just the shortest paths.
void Mapper::GenShortestPaths(const ir::GateRef &gp, UInt src, UInt tgt, UInt budget, List<Alter> &resla, whichpaths_t which) {
    List<Alter> genla;    // list that will get the result of a recursive Gen call

    QL_DOUT("GenShortestPaths: src=" << src << " tgt=" << tgt << " budget=" << budget << " which=" << which);
    QL_ASSERT(resla.empty());

    if (src == tgt) {
        // found target
        // create a virgin Alter and initialize it to become an empty path
        // add src to this path (so that it becomes a distance 0 path with one qubit, src)
        // and add the Alter to the result list
        Alter a;
        a.initialize(kernelp, options);
        a.target_gate = gp;
        a.add_to_front(src);
        resla.push_back(a);
        a.debug_print("... empty path after adding to result list");
        Alter::debug_print("... result list after adding empty path", resla);
        QL_DOUT("... will return now");
        return;
    }

    // start looking around at neighbors for serious paths
    UInt d = platformp->grid->get_distance(src, tgt);
    QL_DOUT("GenShortestPaths: distance(src=" << src << ", tgt=" << tgt << ") = " << d);
    QL_ASSERT(d >= 1);

    // reduce neighbors nbs to those n continuing a path within budget
    // src=>tgt is distance d, budget>=d is allowed, attempt src->n=>tgt
    // src->n is one hop, budget from n is one less so distance(n,tgt) <= budget-1 (i.e. distance < budget)
    // when budget==d, this defaults to distance(n,tgt) <= d-1
    auto nbl = platformp->grid->get_neighbors(src);
    nbl.remove_if([this,budget,tgt](const UInt& n) { return platformp->grid->get_distance(n, tgt) >= budget; });
    if (logger::log_level >= logger::LogLevel::LOG_DEBUG) {
        QL_DOUT("GenShortestPaths: ... after reducing to steps within budget, nbl: ");
        for (auto dn : nbl) {
            QL_DOUT("..." << dn << " ");
        }
    }

    // rotate neighbor list nbl such that largest difference between angles of adjacent elements is beyond back()
    // this makes only sense when there is an underlying xy grid; when not, which can only be wp_all_shortest
    QL_ASSERT(platformp->grid->has_coordinates() || options->path_selection_mode != PathSelectionMode::BORDERS);
    platformp->grid->sort_neighbors_by_angle(src, nbl);
    // subset to those neighbors that continue in direction(s) we want
    if (which == wp_left_shortest) {
        nbl.remove_if( [nbl](const UInt& n) { return n != nbl.front(); } );
    } else if (which == wp_right_shortest) {
        nbl.remove_if( [nbl](const UInt& n) { return n != nbl.back(); } );
    } else if (which == wp_leftright_shortest) {
        nbl.remove_if( [nbl](const UInt& n) { return n != nbl.front() && n != nbl.back(); } );
    }

    if (logger::log_level >= logger::LogLevel::LOG_DEBUG) {
        QL_DOUT("GenShortestPaths: ... after normalizing, before iterating, nbl: ");
        for (auto dn : nbl) {
            QL_DOUT("..." << dn << " ");
        }
    }

    // for all resulting neighbors, find all continuations of a shortest path
    for (auto &n : nbl) {
        whichpaths_t newwhich = which;
        // but for each neighbor only look in desired direction, if any
        if (which == wp_leftright_shortest && nbl.size() != 1) {
            // when looking both left and right still, and there is a choice now, split into left and right
            if (n == nbl.front()) {
                newwhich = wp_left_shortest;
            } else {
                newwhich = wp_right_shortest;
            }
        }
        GenShortestPaths(gp, n, tgt, budget-1, genla, newwhich);  // get list of possible paths in budget-1 from n to tgt in genla
        resla.splice(resla.end(), genla);           // moves all of genla to resla; makes genla empty
    }
    // resla contains all paths starting from a neighbor of src, to tgt

    // add src to front of all to-be-returned paths from src's neighbors to tgt
    for (auto &a : resla) {
        QL_DOUT("... GenShortestPaths, about to add src=" << src << " in front of path");
        a.add_to_front(src);
    }
    QL_DOUT("... GenShortestPaths: returning from call of: " << "src=" << src << " tgt=" << tgt << " budget=" << budget << " which=" << which);
}

// Generate shortest paths in the grid for making gate gp NN, from qubit src to qubit tgt, with an alternative for each one
// - compute budget; usually it is distance but it can be higher such as for multi-core
// - reduce the number of paths depending on the mappathselect option
// - when not all shortest paths found are valid, take these out
// - paths are further split because each split may give rise to a separate alternative
//      a split is a hop where the two-qubit gate is assumed to be done;
//      and after splitting each alternative contains two lists,
//      one before and one after (reversed) the envisioned two-qubit gate;
//      all result alternatives are such that a two-qubit gate can be placed at the split
// End result is a list of alternatives (in resla) suitable for being evaluated for any routing metric.
void Mapper::GenShortestPaths(const ir::GateRef &gp, UInt src, UInt tgt, List<Alter> &resla) {
    List<Alter> directla;  // list that will hold all not-yet-split Alters directly from src to tgt

    UInt budget = platformp->grid->get_min_hops(src, tgt);
    if (options->path_selection_mode == PathSelectionMode::ALL) {
        GenShortestPaths(gp, src, tgt, budget, directla, wp_all_shortest);
    } else if (options->path_selection_mode == PathSelectionMode::BORDERS) {
        GenShortestPaths(gp, src, tgt, budget, directla, wp_leftright_shortest);
    } else {
        QL_FATAL("Unknown value of mapppathselect option " << options->path_selection_mode);
    }

    // QL_DOUT("about to split the paths");
    for (auto &a : directla) {
        a.split(resla);
    }
    // Alter::DPRINT("... after generating and splitting the paths", resla);
}

// Generate all possible variations of making gp NN, starting from given past (with its mappings),
// and return the found variations by appending them to the given list of Alters, la
void Mapper::GenAltersGate(const ir::GateRef &gp, List<Alter> &la, Past &past) {
    auto&   q = gp->operands;
    QL_ASSERT (q.size() == 2);
    UInt  src = past.map_qubit(q[0]);  // interpret virtual operands in past's current map
    UInt  tgt = past.map_qubit(q[1]);
    QL_DOUT("GenAltersGate: " << gp->qasm() << " in real (q" << src << ",q" << tgt << ") at get_min_hops=" << platformp->grid->get_min_hops(src, tgt));
    past.debug_print_fc();

    GenShortestPaths(gp, src, tgt, la);// find shortest paths from src to tgt, and split these
    QL_ASSERT(la.size() != 0);
    // Alter::DPRINT("... after GenShortestPaths", la);
}

// Generate all possible variations of making gates in lg NN, starting from given past (with its mappings),
// and return the found variations by appending them to the given list of Alters, la
// Depending on maplookahead only take first (most critical) gate or take all gates.
void Mapper::GenAlters(const List<ir::GateRef> &lg, List<Alter> &la, Past &past) {
    if (options->lookahead_mode == LookaheadMode::ALL) {
        // create alternatives for each gate in lg
        QL_DOUT("GenAlters, " << lg.size() << " 2q gates; create an alternative for each");
        for (auto gp : lg) {
            // gen alternatives for gp and add these to la
            QL_DOUT("GenAlters: create alternatives for: " << gp->qasm());
            GenAltersGate(gp, la, past);  // gen all possible variations to make gp NN, in current v2r mapping ("past")
        }
    } else {
        // only take the first gate in avlist, the most critical one, and generate alternatives for it
        ir::GateRef gp = lg.front();
        QL_DOUT("GenAlters, " << lg.size() << " 2q gates; take first: " << gp->qasm());
        GenAltersGate(gp, la, past);  // gen all possible variations to make gp NN, in current v2r mapping ("past")
    }
}

// start the random generator with a seed
// that is unique to the microsecond
void Mapper::RandomInit() {
    auto ts = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    // QL_DOUT("Seeding random generator with " << ts );
    gen.seed(ts);
}

// if the maptiebreak option indicates so,
// generate a random Int number in range 0..count-1 and use
// that to index in list of alternatives and to return that one,
// otherwise return a fixed one (front, back or first most critical one
Alter Mapper::ChooseAlter(List<Alter> &la, Future &future) {
    if (la.size() == 1) {
        return la.front();
    }

    switch (options->tie_break_method) {
        case TieBreakMethod::CRITICAL: {
            List<ir::GateRef> lag;
            for (auto &a : la) {
                lag.push_back(a.target_gate);
            }
            ir::GateRef gp = future.get_most_critical(lag);
            QL_ASSERT(!gp.empty());
            for (auto &a : la) {
                if (a.target_gate.get_ptr() == gp.get_ptr()) {
                    // QL_DOUT(" ... took first alternative with most critical target gate");
                    return a;
                }
            }
            return la.front();
        }

        case TieBreakMethod::RANDOM: {
            Alter res;
            std::uniform_int_distribution<> dis(0, (la.size()-1));
            UInt choice = dis(gen);
            UInt i = 0;
            for (auto &a : la) {
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
            return la.back();

        case TieBreakMethod::FIRST:
        default:
            return la.front();

    }
}

// Map the gate/operands of a gate that has been routed or doesn't require routing
void Mapper::MapRoutedGate(ir::GateRef &gp, Past &past) {
    QL_DOUT("MapRoutedGate on virtual: " << gp->qasm() );

    // MakeReal of this gate maps its qubit operands and optionally updates its gate name
    // when the gate name was updated, a new gate with that name is created;
    // when that new gate is a composite gate, it is immediately decomposed (by gate creation)
    // the resulting gate/expansion (anyhow a sequence of gates) is collected in circ
    ir::Circuit circ;   // result of MakeReal
    past.make_real(gp, circ);
    for (auto newgp : circ)
    {
        QL_DOUT(" ... new mapped real gate, about to be added to past: " << newgp->qasm() );
        past.add_and_schedule(newgp);
    }
}

// commit Alter resa
// generating swaps in past
// and taking it out of future when done with it
void Mapper::CommitAlter(Alter &resa, Future &future, Past &past) {
    ir::GateRef resgp = resa.target_gate;   // and the 2q target gate then in resgp
    resa.debug_print(
        "... CommitAlter, alternative to commit, will add swaps and then map target 2q gate");

    resa.add_swaps(past, options->swap_selection_mode);

    // when only some swaps were added, the resgp might not yet be NN, so recheck
    auto &q = resgp->operands;
    if (platformp->grid->get_min_hops(past.map_qubit(q[0]), past.map_qubit(q[1])) == 1) {
        // resgp is NN: so done with this 2q gate
        // QL_DOUT("... CommitAlter, target 2q is NN, map it and done: " << resgp->qasm());
        MapRoutedGate(resgp, past);     // the 2q target gate is NN now and thus can be mapped
        future.completed_gate(resgp);         // and then taken out of future
    } else {
        // QL_DOUT("... CommitAlter, target 2q is not NN yet, keep it: " << resgp->qasm());
    }
}

// Find gates in future.avlist that do not require routing, take them out and map them.
// Ultimately, no gates remain or only gates that require routing.
// Return false when no gates remain at all.
// Return true when any gates remain; those gates are returned in lg.
//
// Behavior depends on the value of option maplookahead and alsoNN2q parameter
// alsoNN2q is true:
//   maplookahead == "no":             while (next in circuit is nonq or 1q) map gate; return when it is 2q (maybe NN)
//                                     in this case, GetNonQuantumGates only returns a nonq when it is next in circuit
//              == "1qfirst":          while (nonq or 1q) map gate; return most critical 2q (maybe NN)
//              == "noroutingfirst":   while (nonq or 1q or 2qNN) map gate; return most critical 2q (nonNN)
//              == "all":              while (nonq or 1q or 2qNN) map gate; return all 2q (nonNN)
// alsoNN2q is false:
//   maplookahead == "no":             while (next in circuit is nonq or 1q) map gate; return when it is 2q (maybe NN)
//                                     in this case, GetNonQuantumGates only returns a nonq when it is next in circuit
//              == "1qfirst":          while (nonq or 1q) map gate; return most critical 2q (nonNN or NN)
//              == "noroutingfirst":   while (nonq or 1q) map gate; return most critical 2q (nonNN or NN)
//              == "all":              while (nonq or 1q) map gate; return all 2q (nonNN or NN)
//
Bool Mapper::MapMappableGates(Future &future, Past &past, List<ir::GateRef> &lg, Bool alsoNN2q) {
    List<ir::GateRef> nonqlg; // list of non-quantum gates in avlist
    List<ir::GateRef> qlg;    // list of (remaining) gates in avlist

    QL_DOUT("MapMappableGates entry");
    while (1) {
        if (future.get_non_quantum_gates(nonqlg)) {
            // avlist contains non-quantum gates
            // and GetNonQuantumGates indicates these (in nonqlg) must be done first
            QL_DOUT("MapMappableGates, there is a set of non-quantum gates");
            for (auto gp : nonqlg) {
                // here add code to map qubit use of any non-quantum instruction????
                // dummy gates are nonq gates internal to OpenQL such as SOURCE/SINK; don't output them
                if (gp->type() != ir::GateType::DUMMY) {
                    // past only can contain quantum gates, so non-quantum gates must by-pass Past
                    past.bypass(gp);    // this flushes past.lg first to outlg
                }
                future.completed_gate(gp); // so on avlist= nonNN2q -> NN2q -> 1q -> nonq: the nonq is done first
                QL_DOUT("MapMappableGates, done with " << gp->qasm());
            }
            QL_DOUT("MapMappableGates, done with set of non-quantum gates, continuing ...");
            continue;
        }
        if (!future.get_gates(qlg)) {
            QL_DOUT("MapMappableGates, no gates anymore, return");
            // avlist doesn't contain any gate
            lg.clear();
            return false;
        }

        // avlist contains quantum gates
        // and GetNonQuantumGates/GetGates indicate these (in qlg) must be done now
        Bool foundone = false;  // whether a quantum gate was found that never requires routing
        for (auto gp : qlg) {
            if (gp->type() == ir::GateType::WAIT || gp->operands.size() == 1) {
                // a quantum gate not requiring routing ever is found
                MapRoutedGate(gp, past);
                future.completed_gate(gp);
                foundone = true;    // a quantum gate was found that never requires routing
                // so on avlist= nonNN2q -> NN2q -> 1q: the 1q is done first
                break;
            }
        }
        if (foundone) {
            continue;
        }
        // qlg only contains 2q gates (that could require routing)
        if (alsoNN2q) {
            // when there is a 2q in qlg that is mappable already, map it
            // when more, take most critical one first (because qlg is ordered, most critical first)
            for (auto gp : qlg) {
                auto &q = gp->operands;
                UInt  src = past.map_qubit(q[0]);      // interpret virtual operands in current map
                UInt  tgt = past.map_qubit(q[1]);
                UInt  d = platformp->grid->get_min_hops(src, tgt);    // and find minimum number of hops between real counterparts
                if (d == 1) {
                    QL_DOUT("MapMappableGates, NN no routing: " << gp->qasm() << " in real (q" << src << ",q" << tgt << ")");
                    MapRoutedGate(gp, past);
                    future.completed_gate(gp);
                    foundone = true;    // a 2q quantum gate was found that was mappable
                    // so on avlist= nonNN2q -> NN2q: the NN2q is done first
                    break;
                }
            }
            if (foundone) {
                // found a mappable 2q one, and mapped it
                // don't map more mappable 2q ones
                // (they might not be critical, the now available 1q gates may hide a more critical 2q gate),
                // but deal with all available non-quantum, and 1q gates first,
                // and only when non of those remain, map a next mappable 2q (now most critical) one
                QL_DOUT("MapMappableGates, found and mapped an easy quantum gate, continuing ...");
                continue;
            }
            QL_DOUT("MapMappableGates, only nonNN 2q gates remain: ...");
        } else {
            QL_DOUT("MapMappableGates, only 2q gates remain (nonNN and NN): ...");
        }
        // avlist (qlg) only contains 2q gates (when alsoNN2q: only non-NN ones; otherwise also perhaps NN ones)
        lg = qlg;
        if (logger::log_level >= logger::LogLevel::LOG_DEBUG) {
            for (auto gp : lg) {
                QL_DOUT("... 2q gate returned: " << gp->qasm());
            }
        }
        return true;
    }
}

// select Alter determined by strategy defined by mapper options
// - if base[rc], select from whole list of Alters, of which all 'remain'
// - if minextend[rc], select Alter from list of Alters with minimal cycle extension of given past
//   when several remain with equal minimum extension, recurse to reduce this set of remaining ones
//   - level: level of recursion at which SelectAlter is called: 0 is base, 1 is 1st, etc.
//   - option mapselectmaxlevel: max level of recursion to use, where inf indicates no maximum
// - maptiebreak option indicates which one to take when several (still) remain
// result is returned in resa
void Mapper::SelectAlter(List<Alter> &la, Alter &resa, Future &future, Past &past, Past &basePast, UInt level) {
    // la are all alternatives we enter with
    QL_ASSERT(!la.empty());  // so there is always a result Alter

    List<Alter> gla;       // good alternative subset of la, suitable to go in recursion with
    List<Alter> bla;       // best alternative subset of gla, suitable to choose result from

    QL_DOUT("SelectAlter ENTRY level=" << level << " from " << la.size() << " alternatives");
    if (options->heuristic == Heuristic::BASE || options->heuristic == Heuristic::BASE_RC) {
        Alter::debug_print(
            "... SelectAlter base (equally good/best) alternatives:", la);
        resa = ChooseAlter(la, future);
        resa.debug_print("... the selected Alter is");
        // QL_DOUT("SelectAlter DONE level=" << level << " from " << la.size() << " alternatives");
        return;
    }
    QL_ASSERT(
        options->heuristic == Heuristic::MIN_EXTEND ||
        options->heuristic == Heuristic::MIN_EXTEND_RC ||
        options->heuristic == Heuristic::MAX_FIDELITY
    );

    // Compute a.score of each alternative relative to basePast, and sort la on it, minimum first
    for (auto &a : la) {
        a.debug_print("Considering extension by alternative: ...");
        a.extend(past, basePast);           // locally here, past will be cloned and kept in alter
        // and the extension stored into the a.score
    }
    la.sort([this](const Alter &a1, const Alter &a2) { return a1.score < a2.score; });
    Alter::debug_print(
        "... SelectAlter sorted all entry alternatives after extension:", la);

    // Reduce sorted list of alternatives (la) to list of good alternatives (gla)
    // suitable to find in recursion which is/are really best;
    // this need not be only those with minimum extend, it can be more.
    // With option mapselectmaxwidth="min" in which "min" stands for minimal number, we get just those minimal ones.
    // With other option values, we are more forgiving but that easily lets the number of alternatives explode.
    gla = la;
    gla.remove_if([this,la](const Alter& a) { return a.score != la.front().score; });
    UInt las = la.size();
    UInt glas = gla.size();
    Real keep_real = utils::max(1.0, utils::ceil(options->recursion_width_limit * glas));
    UInt keep = (keep_real < static_cast<utils::Real>(utils::MAX)) ? static_cast<utils::UInt>(keep_real) : utils::MAX;
    if (keep != glas) {
        if (keep < las) {
            gla = la;
            List<Alter>::iterator  ia;
            ia = gla.begin();
            advance(ia, keep);
            gla.erase(ia, gla.end());
        } else {
            gla = la;
        }
    }
    // QL_DOUT("SelectAlter mapselectmaxwidth=" << mapselectmaxwidthopt << " level=" << level << " reduced la to gla");
    Alter::debug_print("... SelectAlter good alternatives before recursion:",
                       gla);

    // When maxlevel has been reached, stop the recursion, and choose from the best minextend/maxfidelity alternatives
    if (level >= options->recursion_depth_limit) {
        // Reduce list of good alternatives (gla) to list of minextend/maxfidelity best alternatives (bla)
        // and make a choice from that list to return as result
        bla = gla;
        bla.remove_if([this,gla](const Alter& a) { return a.score != gla.front().score; });
        Alter::debug_print(
            "... SelectAlter reduced to best alternatives to choose result from:",
            bla);
        resa = ChooseAlter(bla, future);
        resa.debug_print("... the selected Alter (STOPPING RECURSION) is");
        // QL_DOUT("SelectAlter DONE level=" << level << " from " << bla.size() << " best alternatives");
        return;
    }

    // Otherwise, prepare using recursion to choose from the good alternatives,
    // i.e. make a recursion step looking ahead to decide which alternative is best
    //
    // For each alternative in gla,
    // lookahead for next non-NN2q gates, and comparing them for their alternative mappings;
    // the lookahead alternative with the least overall extension (i.e. relative to basePast) is chosen,
    // and the current alternative on top of which it was build
    // is chosen at the current level, unwinding the recursion.
    //
    // Recursion could stop above because:
    // - max level of recursion was reached
    // Recursion can stop here because of:
    // - end-of-circuit (no non-NN 2q gates remain).
    //
    // When gla.size() == 1, we still want to know its minimum extension, to compare with competitors,
    // since that is not just a local figure but the extension from basePast;
    // so indeed with only one alternative we may still go into recursion below.
    // This means that recursion always goes to maxlevel or end-of-circuit.
    // This anomaly may need correction.
    // QL_DOUT("... SelectAlter level=" << level << " entering recursion with " << gla.size() << " good alternatives");
    for (auto &a : gla) {
        a.debug_print("... ... considering alternative:");
        Future future_copy = future;            // copy!
        Past   past_copy = past;                // copy!
        CommitAlter(a, future_copy, past_copy);
        a.debug_print(
            "... ... committed this alternative first before recursion:");

        Bool    havegates;                  // are there still non-NN 2q gates to map?
        List<ir::GateRef> lg;            // list of non-NN 2q gates taken from avlist, as returned from MapMappableGates

        // In recursion, look at option maprecNN2q:
        // - MapMappableGates with alsoNN2q==true is greedy and immediately maps each 1q and NN 2q gate
        // - MapMappableGates with alsoNN2q==false is not greedy, maps all 1q gates but not the (NN) 2q gates
        //
        // when yes and when maplookaheadopt is noroutingfirst or all, let MapMappableGates stop mapping only on nonNN2q
        // when no, let MapMappableGates stop mapping on any 2q
        // This creates more clear recursion: one 2q at a time instead of a possible empty set of NN2qs followed by a nonNN2q;
        // also when a NN2q is found, this is perfect; this is not seen when immediately mapping all NN2qs.
        // So goal is to prove that maprecNN2q should be no at this place, in the recursion step, but not at level 0!
        Bool alsoNN2q = options->recurse_nn_two_qubit
                     && (
                         options->lookahead_mode == LookaheadMode::NO_ROUTING_FIRST
                         || options->lookahead_mode == LookaheadMode::ALL
                     );
        havegates = MapMappableGates(future_copy, past_copy, lg, alsoNN2q); // map all easy gates; remainder returned in lg

        if (havegates) {
            QL_DOUT("... ... SelectAlter level=" << level << ", committed + mapped easy gates, now facing " << lg.size() << " 2q gates to evaluate next");
            List<Alter> la;                // list that will hold all variations, as returned by GenAlters
            GenAlters(lg, la, past_copy);       // gen all possible variations to make gates in lg NN, in current past.v2r mapping
            QL_DOUT("... ... SelectAlter level=" << level << ", generated for these 2q gates " << la.size() << " alternatives; RECURSE ... ");
            Alter resa;                         // result alternative selected and returned by next SelectAlter call
            SelectAlter(la, resa, future_copy, past_copy, basePast, level+1); // recurse, best in resa ...
            resa.debug_print(
                "... ... SelectAlter, generated for these 2q gates ... ; RECURSE DONE; resulting alternative ");
            a.score = resa.score;               // extension of deep recursion is treated as extension at current level,
            // by this an alternative started bad may be compensated by deeper alts
        } else {
            QL_DOUT("... ... SelectAlter level=" << level << ", no gates to evaluate next; RECURSION BOTTOM");
            if (options->heuristic == Heuristic::MAX_FIDELITY) {
                QL_FATAL("Mapper option maxfidelity has been disabled");
                // a.score = quick_fidelity(past_copy.lg);
            } else {
                a.score = past_copy.get_max_free_cycle() -
                          basePast.get_max_free_cycle();
            }
            a.debug_print(
                "... ... SelectAlter, after committing this alternative, mapped easy gates, no gates to evaluate next; RECURSION BOTTOM");
        }
        a.debug_print("... ... DONE considering alternative:");
    }
    // Sort list of good alternatives (gla) on score resulting after recursion
    gla.sort([this](const Alter &a1, const Alter &a2) { return a1.score < a2.score; });
    Alter::debug_print("... SelectAlter sorted alternatives after recursion:",
                       gla);

    // Reduce list of good alternatives (gla) of before recursion to list of equally minimal best alternatives now (bla)
    // and make a choice from that list to return as result
    bla = gla;
    bla.remove_if([this,gla](const Alter& a) { return a.score != gla.front().score; });
    Alter::debug_print(
        "... SelectAlter equally best alternatives on return of RECURSION:",
        bla);
    resa = ChooseAlter(bla, future);
    resa.debug_print("... the selected Alter is");
    // QL_DOUT("... SelectAlter level=" << level << " selecting from " << bla.size() << " equally good alternatives above DONE");
    QL_DOUT("SelectAlter DONE level=" << level << " from " << la.size() << " alternatives");
}

// Given the states of past and future
// map all mappable gates and find the non-mappable ones
// for those evaluate what to do next and do it;
// during recursion, comparison is done with the base past (bottom of recursion stack),
// and past is the last past (top of recursion stack) relative to which the mapping is done.
void Mapper::MapGates(Future &future, Past &past, Past &basePast) {
    List<ir::GateRef> lg;              // list of non-mappable gates taken from avlist, as returned from MapMappableGates
    Bool alsoNN2q = (options->lookahead_mode == LookaheadMode::NO_ROUTING_FIRST || options->lookahead_mode == LookaheadMode::ALL);
    while (MapMappableGates(future, past, lg, alsoNN2q)) { // returns false when no gates remain
        // all gates in lg are two-qubit quantum gates that cannot be mapped
        // select which one(s) to (partially) route, according to one of the known strategies
        // the only requirement on the code below is that at least something is done that decreases the problem

        // generate all variations
        List<Alter> la;                // list that will hold all variations, as returned by GenAlters
        GenAlters(lg, la, past);            // gen all possible variations to make gates in lg NN, in current past.v2r mapping

        // select best one
        Alter resa;
        SelectAlter(la, resa, future, past, basePast, 0);
        // select one according to strategy specified by options; result in resa

        // commit to best one
        // add all or just one swap, as described by resa, to THIS past, and schedule them/it in
        CommitAlter(resa, future, past);
    }
}

// Map the circuit's gates in the provided context (v2r maps), updating circuit and v2r maps
void Mapper::MapCircuit(const ir::KernelRef &kernel, QubitMapping &v2r) {
    Future  future;         // future window, presents input in avlist
    Past    mainPast;       // past window, contains output schedule, storing all gates until taken out
    utils::Ptr<Scheduler> sched;
    sched.emplace();        // new scheduler instance (from src/scheduler.h) used for its dependence graph

    future.initialize(platformp, options);
    future.set_kernel(kernel, sched); // constructs depgraph, initializes avlist, ready for producing gates
    kernel->c.reset();      // future has copied kernel.c to private data; kernel.c ready for use by new_gate
    kernelp = kernel;      // keep kernel to call kernelp->gate() inside Past.new_gate(), to create new gates

    mainPast.initialize(kernelp, options);  // mainPast and Past clones inside Alters ready for generating output schedules into
    mainPast.import_mapping(v2r);    // give it the current mapping/state
    // mainPast.DPRINT("start mapping");

    MapGates(future, mainPast, mainPast);
    mainPast.flush_all();                // all output to mainPast.outlg, the output window of mainPast

    // mainPast.DPRINT("end mapping");

    QL_DOUT("... retrieving outCirc from mainPast.outlg; swapping outCirc with kernel.c, kernel.c contains output circuit");
    ir::Circuit outCirc;
    mainPast.flush_to_circuit(outCirc);                          // copy (final part of) mainPast's output window into this outCirc
    kernel->c.get_vec().swap(outCirc.get_vec());    // and then to kernel.c
    kernel->cycles_valid = true;                    // decomposition was scheduled in; see Past.Add() and Past.Schedule()
    mainPast.export_mapping(v2r);
    nswapsadded = mainPast.get_num_swaps_added();
    nmovesadded = mainPast.get_num_moves_added();
}

// decompose all gates that have a definition with _prim appended to its name
void Mapper::MakePrimitives(const ir::KernelRef &kernel) {
    QL_DOUT("MakePrimitives circuit ...");

    ir::Circuit input_gatepv = kernel->c;       // copy to allow kernel.c use by Past.new_gate
    kernel->c.reset();                          // kernel.c ready for use by new_gate

    Past mainPast;                              // output window in which gates are scheduled
    mainPast.initialize(kernelp, options);

    for (auto & gp : input_gatepv) {
        ir::Circuit tmpCirc;
        mainPast.make_primitive(gp, tmpCirc);    // decompose gp into tmpCirc; on failure, copy gp into tmpCirc
        for (auto newgp : tmpCirc) {
            mainPast.add_and_schedule(newgp);     // decomposition is scheduled in gate by gate
        }
    }
    mainPast.flush_all();

    ir::Circuit outCirc;                        // ultimate output gate stream
    mainPast.flush_to_circuit(outCirc);
    kernel->c.get_vec().swap(outCirc.get_vec());
    kernel->cycles_valid = true;                 // decomposition was scheduled in above

    QL_DOUT("MakePrimitives circuit [DONE]");
}

// map kernel's circuit, main mapper entry once per kernel
void Mapper::Map(const ir::KernelRef &kernel) {
    QL_DOUT("Mapping kernel " << kernel->name << " [START]");
    QL_DOUT("... kernel original virtual number of qubits=" << kernel->qubit_count);
    kernelp.reset();            // no new_gates until kernel.c has been copied

    QL_DOUT("Mapper::Map before v2r.initialize: assume_initialized=" << options->assume_initialized);

    // unify all incoming v2rs into v2r to compute kernel input mapping;
    // but until inter-kernel mapping is implemented, take program initial mapping for it
    QubitMapping v2r{            // current mapping while mapping this kernel
        nq,
        options->initialize_one_to_one,
        options->assume_initialized ? QubitState::INITIALIZED : QubitState::NONE
    };
    QL_IF_LOG_DEBUG {
        QL_DOUT("After initialization");
        v2r.dump_state();
    }

    v2r_in = v2r;  // for reporting

    if (options->enable_mip_placer) {
#ifdef INITIALPLACE
        QL_DOUT("InitialPlace: kernel=" << kernel->name << " timeout=" << options->mip_timeout << " horizon=" << options->mip_horizon << " [START]");

        place_mip::detail::Options ipopt;
        ipopt.map_all = options->initialize_one_to_one;
        ipopt.horizon = options->mip_horizon;
        ipopt.timeout = options->mip_timeout;

        place_mip::detail::Algorithm ip;
        auto ipok = ip.run(kernel, ipopt, v2r); // compute mapping (in v2r) using ip model, may fail
        QL_DOUT("InitialPlace: kernel=" << kernel->name << " timeout=" << options->mip_timeout << " horizon=" << options->mip_horizon << " result=" << ipok << " iptimetaken=" << ip.get_time_taken() << " seconds [DONE]");
#else // ifdef INITIALPLACE
        QL_DOUT("InitialPlace support disabled during OpenQL build [DONE]");
        QL_WOUT("InitialPlace support disabled during OpenQL build [DONE]");
#endif // ifdef INITIALPLACE
    }
    QL_IF_LOG_DEBUG {
        QL_DOUT("After InitialPlace");
        v2r.dump_state();
    }

    v2r_ip = v2r;  // for reporting

    QL_DOUT("Mapper::Map before MapCircuit: assume_initialized=" << options->assume_initialized);

    MapCircuit(kernel, v2r);        // updates kernel.c with swaps, maps all gates, updates v2r map
    QL_IF_LOG_DEBUG {
        QL_DOUT("After heuristics");
        v2r.dump_state();
    }

    MakePrimitives(kernel);         // decompose to primitives as specified in the config file

    kernel->qubit_count = nq;       // bluntly copy nq (==#real qubits), so that all kernels get the same qubit_count
    kernel->creg_count = nc;        // same for number of cregs and bregs, although we don't really map those
    kernel->breg_count = nb;
    v2r_out = v2r;                  // for reporting

    QL_DOUT("Mapping kernel " << kernel->name << " [DONE]");
}

// initialize mapper for whole program
// lots could be split off for the whole program, once that is needed
//
// initialization for a particular kernel is separate (in Map entry)
void Mapper::Init(const plat::PlatformRef &p, const OptionsRef &opt) {
    // QL_DOUT("Mapping initialization ...");
    // QL_DOUT("... Grid initialization: platform qubits->coordinates, ->neighbors, distance ...");
    platformp = p;
    options = opt;
    nq = p->qubit_count;
    nc = p->creg_count;
    nb = p->breg_count;
    RandomInit();
    // QL_DOUT("... platform/real number of qubits=" << nq << ");
    cycle_time = p->cycle_time;

    // QL_DOUT("Mapping initialization [DONE]");
}

} // namespace detail
} // namespace route
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
