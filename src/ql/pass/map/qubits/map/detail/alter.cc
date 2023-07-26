/** \file
 * Alter implementation.
 */

#include "alter.h"

// uncomment next line to enable multi-line dumping
// #define MULTI_LINE_LOG_DEBUG

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace map {
namespace detail {

/**
 * This should only be called after a virgin construction and not after
 * cloning a path.
 */
void Alter::initialize(const ir::compat::KernelRef &k, const OptionsRef &opt) {
    QL_DOUT("Alter::initialize(number of qubits=" << k->platform->qubit_count);
    platform = k->platform;
    kernel = k;
    options = opt;

    nq = platform->qubit_count;
    ct = platform->cycle_time;
    // total, fromSource and fromTarget start as empty vectors
    past.initialize(kernel, options); // initializes past to empty
    score_valid = false; // will not print score for now
}

/**
 * Print path as s followed by path of the form [0->1->2].
 */
static void partial_print(
    const utils::Str &s,
    const utils::Vec<utils::UInt> &path
) {
    if (!path.empty()) {
        std::cout << s << path.to_string("[", "->", "]");
    }
}

/**
 * Prints the state of this Alter, prefixed by s.
 */
void Alter::print(const utils::Str &s) const {
    // std::cout << s << "- " << targetgp->qasm();
    std::cout << s << "- " << target_gate->qasm();
    if (from_source.empty() && from_target.empty()) {
        partial_print(", total path:", total);
    } else {
        partial_print(", path from source:", from_source);
        partial_print(", from target:", from_target);
    }
    if (score_valid) {
        std::cout << ", score=" << score;
    }
    // past.Print("past in Alter");
    std::cout << std::endl;
}

/**
 * Prints the state of this Alter, prefixed by s, only when the logging
 * verbosity is at least debug.
 */
void Alter::debug_print(const utils::Str &) const {
#ifdef MULTI_LINE_LOG_DEBUG
    QL_IF_LOG_DEBUG {
        QL_DOUT("Printing current Alter's state: ");
        print(s);
    }
#else
    QL_DOUT("Printing current Alter's state (disabled)");
#endif
}

/**
 * Prints a state of a whole list of Alters, prefixed by s.
 */
void Alter::print(const utils::Str &s, const utils::List<Alter> &la) {
    utils::Int started = 0;
    for (auto &a : la) {
        if (started == 0) {
            started = 1;
            std::cout << s << "[" << la.size() << "]={" << std::endl;
        }
        a.print("");
    }
    if (started == 1) {
        std::cout << "}" << std::endl;
    }
}

/**
 * Prints a state of a whole list of Alters, prefixed by s, only when the
 * logging verbosity is at least debug.
 */
void Alter::debug_print(const utils::Str &, const utils::List<Alter> &) {
#ifdef MULTI_LINE_LOG_DEBUG
    QL_IF_LOG_DEBUG {
        QL_DOUT("Print list of Alters: ");
        print(s, la);
    }
#else
    QL_DOUT("Print list of Alters (disabled)");
#endif
}

/**
 * Adds a node to the path in front, extending its length with one.
 */
void Alter::add_to_front(utils::UInt q) {
    total.insert(total.begin(), q); // hopelessly inefficient
}

/**
 * Add swap gates for the current path to the given past, up to the maximum
 * specified by the swap selection mode. This past can be a path-local one
 * or the main past. After having added them, schedule the result into that
 * past.
 */
void Alter::add_swaps(Past &past, SwapSelectionMode mode) const {
    // QL_DOUT("Addswaps " << mapselectswapsopt);
    if (mode == SwapSelectionMode::ONE || mode == SwapSelectionMode::ALL) {
        utils::UInt num_added = 0;
        utils::UInt max_num_to_add = (mode == SwapSelectionMode::ONE ? 1 : ir::compat::MAX_CYCLE);

        utils::UInt from_source_qubit;
        utils::UInt to_source_qubit;
        from_source_qubit = from_source[0];
        for (utils::UInt i = 1; i < from_source.size() && num_added < max_num_to_add; i++) {
            to_source_qubit = from_source[i];
            past.add_swap(from_source_qubit, to_source_qubit);
            from_source_qubit = to_source_qubit;
            num_added++;
        }

        utils::UInt from_target_qubit;
        utils::UInt to_target_qubit;
        from_target_qubit = from_target[0];
        for (utils::UInt i = 1; i < from_target.size() && num_added < max_num_to_add; i++) {
            to_target_qubit = from_target[i];
            past.add_swap(from_target_qubit, to_target_qubit);
            from_target_qubit = to_target_qubit;
            num_added++;
        }
    } else {
        QL_ASSERT(mode == SwapSelectionMode::EARLIEST);
        if (from_source.size() >= 2 && from_target.size() >= 2) {
            if (past.is_first_swap_earliest(from_source[0], from_source[1],
                                            from_target[0], from_target[1])) {
                past.add_swap(from_source[0], from_source[1]);
            } else {
                past.add_swap(from_target[0], from_target[1]);
            }
        } else if (from_source.size() >= 2) {
            past.add_swap(from_source[0], from_source[1]);
        } else if (from_target.size() >= 2) {
            past.add_swap(from_target[0], from_target[1]);
        }
    }

    past.schedule();
}

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
void Alter::extend(const Past &curr_past, const Past &base_past) {
    // QL_DOUT("... clone past, add swaps, compute overall score and keep it all in current alternative");
    past = curr_past;   // explicitly clone currPast to an alternative-local copy of it, Alter.past
    // QL_DOUT("... adding swaps to alternative-local past ...");
    add_swaps(past, SwapSelectionMode::ALL);
    // QL_DOUT("... done adding/scheduling swaps to alternative-local past");

    if (options->heuristic == Heuristic::MAX_FIDELITY) {
        QL_FATAL("Mapper option maxfidelity has been disabled");
        // score = quick_fidelity(past.lg);
    } else {
        score = past.get_max_free_cycle() - base_past.get_max_free_cycle();
    }
    score_valid = true;
}

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
void Alter::split(utils::List<Alter> &result) const {
    // QL_DOUT("Split ...");

    utils::UInt length = total.size();
    QL_ASSERT (length >= 2);   // distance >= 1 so path at least: source -> target
    for (utils::UInt rightopi = length - 1; rightopi >= 1; rightopi--) {
        utils::UInt leftopi = rightopi - 1;
        QL_ASSERT (leftopi >= 0);
        // QL_DOUT("... leftopi=" << leftopi);
        // leftopi is the index in total that holds the qubit that becomes the left operand of the gate
        // rightopi is the index in total that holds the qubit that becomes the right operand of the gate
        // rightopi == leftopi + 1
        if (platform->topology->is_inter_core_hop(total[leftopi], total[rightopi])) {
            // an inter-core hop cannot execute a two-qubit gate, so is not a valid alternative
            // QL_DOUT("... skip inter-core hop from qubit=" << total[leftopi] << " to qubit=" << total[rightopi]);
            continue;
        }

        Alter    na = *this;      // na is local copy of the current path, including total
        // na = *this;            // na is local copy of the current path, including total
        // na.DPRINT("... copy of current alter");

        // fromSource will contain the path with qubits at indices 0 to leftopi
        // fromTarget will contain the path with qubits at indices rightopi to length-1, reversed
        //      reversal of fromTarget is done since swaps need to be generated starting at the target
        utils::UInt fromi, toi;

        na.from_source.resize(leftopi + 1);
        // QL_DOUT("... fromSource size=" << na.fromSource.size());
        for (fromi = 0, toi = 0; fromi <= leftopi; fromi++, toi++) {
            // QL_DOUT("... fromSource: fromi=" << fromi << " toi=" << toi);
            na.from_source[toi] = na.total[fromi];
        }

        na.from_target.resize(length - leftopi - 1);
        // QL_DOUT("... fromTarget size=" << na.fromTarget.size());
        for (fromi = length-1, toi = 0; fromi > leftopi; fromi--, toi++) {
            // QL_DOUT("... fromTarget: fromi=" << fromi << " toi=" << toi);
            na.from_target[toi] = na.total[fromi];
        }

        // na.DPRINT("... copy of alter after split");
        result.push_back(na);
        // QL_DOUT("... added to result list");
        // DPRINT("... current alter after split");
    }
}

} // namespace detail
} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
