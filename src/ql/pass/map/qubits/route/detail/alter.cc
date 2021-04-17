/** \file
 * Alter implementation.
 */

#include "alter.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace route {
namespace detail {

using namespace utils;
using namespace com;

// explicit Alter constructor
// needed for virgin construction
Alter::Alter() {
    QL_DOUT("Constructing Alter");
}

// Alter initializer
// This should only be called after a virgin construction and not after cloning a path.
void Alter::Init(const plat::PlatformRef &p, const ir::KernelRef &k, const OptionsRef &opt) {
    QL_DOUT("Alter::Init(number of qubits=" << p->qubit_count);
    platformp = p;
    kernelp = k;
    options = opt;

    nq = platformp->qubit_count;
    ct = platformp->cycle_time;
    // total, fromSource and fromTarget start as empty vectors
    past.Init(platformp, kernelp, options); // initializes past to empty
    didscore = false;                       // will not print score for now
}

// printing facilities of Paths
// print path as hd followed by [0->1->2]
// and then followed by "implying" swap(q0,q1) swap(q1,q2)
void Alter::partialPrint(const Str &hd, const Vec<UInt> &pp) {
    if (!pp.empty()) {
        Int started = 0;
        for (auto &ppe : pp) {
            if (started == 0) {
                started = 1;
                std::cout << hd << "[";
            } else {
                std::cout << "->";
            }
            std::cout << ppe;
        }
        if (started == 1) {
            std::cout << "]";
//          if (pp.size() >= 2) {
//              std::cout << " implying:";
//              for (UInt i = 0; i < pp.size()-1; i++) {
//                  std::cout << " swap(q" << pp[i] << ",q" << pp[i+1] << ")";
//              }
//          }
        }
    }
}

void Alter::DPRINT(const Str &s) const {
    if (logger::log_level >= logger::LogLevel::LOG_DEBUG) {
        Print(s);
    }
}

void Alter::Print(const Str &s) const {
    // std::cout << s << "- " << targetgp->qasm();
    std::cout << s << "- " << targetgp->qasm();
    if (fromSource.empty() && fromTarget.empty()) {
        partialPrint(", total path:", total);
    } else {
        partialPrint(", path from source:", fromSource);
        partialPrint(", from target:", fromTarget);
    }
    if (didscore) {
        std::cout << ", score=" << score;
    }
    // past.Print("past in Alter");
    std::cout << std::endl;
}

void Alter::DPRINT(const Str &s, const Vec<Alter> &va) {
    if (logger::log_level >= logger::LogLevel::LOG_DEBUG) {
        Print(s, va);
    }
}

void Alter::Print(const Str &s, const Vec<Alter> &va) {
    Int started = 0;
    for (auto &a : va) {
        if (started == 0) {
            started = 1;
            std::cout << s << "[" << va.size() << "]={" << std::endl;
        }
        a.Print("");
    }
    if (started == 1) {
        std::cout << "}" << std::endl;
    }
}

void Alter::DPRINT(const Str &s, const List<Alter> &la) {
    if (logger::log_level >= logger::LogLevel::LOG_DEBUG) {
        Print(s, la);
    }
}

void Alter::Print(const Str &s, const List<Alter> &la) {
    Int started = 0;
    for (auto &a : la) {
        if (started == 0) {
            started = 1;
            std::cout << s << "[" << la.size() << "]={" << std::endl;
        }
        a.Print("");
    }
    if (started == 1) {
        std::cout << "}" << std::endl;
    }
}

// add a node to the path in front, extending its length with one
void Alter::Add2Front(UInt q) {
    total.insert(total.begin(), q); // hopelessly inefficient
}

// add to a max of maxnumbertoadd swap gates for the current path to the given past
// this past can be a path-local one or the main past
// after having added them, schedule the result into that past
void Alter::AddSwaps(Past &past, SwapSelectionMode mapselectswapsopt) const {
    // QL_DOUT("Addswaps " << mapselectswapsopt);
    if (
        mapselectswapsopt == SwapSelectionMode::ONE ||
        mapselectswapsopt == SwapSelectionMode::ALL
    ) {
        UInt  numberadded = 0;
        UInt  maxnumbertoadd = (mapselectswapsopt == SwapSelectionMode::ONE ? 1 : ir::MAX_CYCLE);

        UInt  fromSourceQ;
        UInt  toSourceQ;
        fromSourceQ = fromSource[0];
        for (UInt i = 1; i < fromSource.size() && numberadded < maxnumbertoadd; i++) {
            toSourceQ = fromSource[i];
            past.AddSwap(fromSourceQ, toSourceQ);
            fromSourceQ = toSourceQ;
            numberadded++;
        }

        UInt  fromTargetQ;
        UInt  toTargetQ;
        fromTargetQ = fromTarget[0];
        for (UInt i = 1; i < fromTarget.size() && numberadded < maxnumbertoadd; i++) {
            toTargetQ = fromTarget[i];
            past.AddSwap(fromTargetQ, toTargetQ);
            fromTargetQ = toTargetQ;
            numberadded++;
        }
    } else {
        QL_ASSERT(mapselectswapsopt == SwapSelectionMode::EARLIEST);
        if (fromSource.size() >= 2 && fromTarget.size() >= 2) {
            if (past.IsFirstSwapEarliest(fromSource[0], fromSource[1], fromTarget[0], fromTarget[1])) {
                past.AddSwap(fromSource[0], fromSource[1]);
            } else {
                past.AddSwap(fromTarget[0], fromTarget[1]);
            }
        } else if (fromSource.size() >= 2) {
            past.AddSwap(fromSource[0], fromSource[1]);
        } else if (fromTarget.size() >= 2) {
            past.AddSwap(fromTarget[0], fromTarget[1]);
        }
    }

    past.Schedule();
}

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
void Alter::Extend(const Past &currPast, const Past &basePast) {
    // QL_DOUT("... clone past, add swaps, compute overall score and keep it all in current alternative");
    past = currPast;   // explicitly clone currPast to an alternative-local copy of it, Alter.past
    // QL_DOUT("... adding swaps to alternative-local past ...");
    AddSwaps(past, SwapSelectionMode::ALL);
    // QL_DOUT("... done adding/scheduling swaps to alternative-local past");

    if (options->heuristic == Heuristic::MAX_FIDELITY) {
        QL_FATAL("Mapper option maxfidelity has been disabled");
        // score = quick_fidelity(past.lg);
    } else {
        score = past.MaxFreeCycle() - basePast.MaxFreeCycle();
    }
    didscore = true;
}

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
void Alter::Split(List<Alter> &resla) const {
    // QL_DOUT("Split ...");

    UInt length = total.size();
    QL_ASSERT (length >= 2);   // distance >= 1 so path at least: source -> target
    for (UInt rightopi = length - 1; rightopi >= 1; rightopi--) {
        UInt leftopi = rightopi - 1;
        QL_ASSERT (leftopi >= 0);
        // QL_DOUT("... leftopi=" << leftopi);
        // leftopi is the index in total that holds the qubit that becomes the left operand of the gate
        // rightopi is the index in total that holds the qubit that becomes the right operand of the gate
        // rightopi == leftopi + 1
        if (platformp->grid->is_inter_core_hop(total[leftopi], total[rightopi])) {
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
        UInt fromi, toi;

        na.fromSource.resize(leftopi+1);
        // QL_DOUT("... fromSource size=" << na.fromSource.size());
        for (fromi = 0, toi = 0; fromi <= leftopi; fromi++, toi++) {
            // QL_DOUT("... fromSource: fromi=" << fromi << " toi=" << toi);
            na.fromSource[toi] = na.total[fromi];
        }

        na.fromTarget.resize(length-leftopi-1);
        // QL_DOUT("... fromTarget size=" << na.fromTarget.size());
        for (fromi = length-1, toi = 0; fromi > leftopi; fromi--, toi++) {
            // QL_DOUT("... fromTarget: fromi=" << fromi << " toi=" << toi);
            na.fromTarget[toi] = na.total[fromi];
        }

        // na.DPRINT("... copy of alter after split");
        resla.push_back(na);
        // QL_DOUT("... added to result list");
        // DPRINT("... current alter after split");
    }
}

} // namespace detail
} // namespace route
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
