/** \file
 * Past implementation.
 */

#include "past.h"

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

// explicit Past constructor
// needed for virgin construction
Past::Past() {
    QL_DOUT("Constructing Past");
}

// past initializer
void Past::Init(const plat::PlatformRef &p, const ir::KernelRef &k, const OptionsRef &opt) {
    QL_DOUT("Past::Init");
    platformp = p;
    kernelp = k;
    options = opt;

    nq = platformp->qubit_count;
    nb = platformp->breg_count;
    ct = platformp->cycle_time;

    QL_ASSERT(kernelp->c.empty());   // kernelp->c will be used by new_gate to return newly created gates into
    v2r.resize(                 // v2r initializtion until v2r is imported from context
        nq,
        options->initialize_one_to_one,
        options->assume_initialized ? QubitState::INITIALIZED : QubitState::NONE
    );
    fc.initialize(platformp, options);// fc starts off with all qubits free, is updated after schedule of each gate
    waitinglg.clear();          // no gates pending to be scheduled in; Add of gate to past entered here
    lg.clear();                 // no gates scheduled yet in this past; after schedule of gate, it gets here
    outlg.clear();              // no gates output yet by flushing from or bypassing this past
    nswapsadded = 0;            // no swaps or moves added yet to this past; AddSwap adds one here
    nmovesadded = 0;            // no moves added yet to this past; AddSwap may add one here
    cycle.clear();              // no gates have cycles assigned in this past; scheduling gate updates this
}

// import Past's v2r from v2r_value
void Past::ImportV2r(const QubitMapping &v2r_value) {
    v2r = v2r_value;
}

// export Past's v2r into v2r_destination
void Past::ExportV2r(QubitMapping &v2r_destination) const {
    v2r_destination = v2r;
}

void Past::DFcPrint() const {
    if (logger::log_level >= logger::LogLevel::LOG_DEBUG) {
        fc.print("");
    }
}

void Past::FcPrint() const{
    fc.print("");
}

void Past::Print(const Str &s) const {
    std::cout << "... Past " << s << ":";
    v2r.dump_state();
    fc.print("");
    // QL_DOUT("... list of gates in past");
    for (auto &gp : lg) {
        QL_DOUT("[" << cycle.at(gp) << "] " << gp->qasm());
    }
}

// all gates in past.waitinglg are scheduled here into past.lg
// note that these gates all are mapped and so have real operand qubit indices
// the FreeCycle map reflects for each qubit the first free cycle
// all new gates, now in waitinglist, get such a cycle assigned below, increased gradually, until definitive
void Past::Schedule() {
    // the copy includes the resource manager.
    // QL_DOUT("Schedule ...");

    while (!waitinglg.empty()) {
        UInt      startCycle = ir::MAX_CYCLE;
        utils::List<ir::GateRef>::iterator gp_it;

        // find the gate with the minimum startCycle
        //
        // IMPORTANT: this assumes that the waitinglg gates list is in topological order,
        // which is ok because the pair of swap lists use distict qubits and
        // the gates of each are added to the back of the list in the order of execution.
        // Using tryfc.Add, the tryfc (try FreeCycle map) reflects the earliest startCycle per qubit,
        // and so dependences are respected, so we can find the gate that can start first ...
        // Note that tryfc includes the free cycle vector AND the resource map,
        // so using tryfc.StartCycle/tryfc.Add we get a realistic ASAP rc schedule.
        // We use a copy of fc and not fc itself, since the latter reflects the really scheduled gates
        // and that shouldn't be changed.
        //
        // This search is really a hack to avoid
        // the construction of a dependence graph and a set of schedulable gates
        FreeCycle   tryfc = fc;
        for (auto trygp_it = waitinglg.begin(); trygp_it != waitinglg.end(); ++trygp_it) {
            UInt tryStartCycle = tryfc.get_start_cycle(*trygp_it);
            tryfc.add(*trygp_it, tryStartCycle);

            if (tryStartCycle < startCycle) {
                startCycle = tryStartCycle;
                gp_it = trygp_it;
            }
        }

        auto gp = *gp_it;

        // add this gate to the maps, scheduling the gate (doing the cycle assignment)
        // QL_DOUT("... add " << gp->qasm() << " startcycle=" << startCycle << " cycles=" << ((gp->duration+ct-1)/ct) );
        fc.add(gp, startCycle);
        cycle.set(gp) = startCycle; // cycle[gp] is private to this past but gp->cycle is private to gp
        gp->cycle = startCycle; // so gp->cycle gets assigned for each alter' Past and finally definitively for mainPast
        // QL_DOUT("... set " << gp->qasm() << " at cycle " << startCycle);

        // insert gate gp in lg, the list of gates, in cycle[gp] order, and inside this order, as late as possible
        //
        // reverse iterate because the insertion is near the end of the list
        // insert so that cycle values are in order afterwards and the new one is nearest to the end
        auto rigp = lg.rbegin();
        Bool inserted = false;
        for (; rigp != lg.rend(); rigp++) {
            if (cycle.at(*rigp) <= startCycle) {
                // rigp.base() because insert doesn't work with reverse iteration
                // rigp.base points after the element that rigp is pointing at
                // which is lucky because insert only inserts before the given element
                // the end effect is inserting after rigp
                lg.insert(rigp.base(), gp);
                inserted = true;
                break;
            }
        }
        // when list was empty or no element was found, just put it in front
        if (!inserted) {
            lg.push_front(gp);
        }

        // having added it to the main list, remove it from the waiting list
        waitinglg.erase(gp_it);
    }

    // DPRINT("Schedule:");
}

// compute costs in cycle extension of optionally scheduling initcirc before the inevitable circ
Int Past::InsertionCost(const ir::Circuit &initcirc, const ir::Circuit &circ) const {
    // first fake-schedule initcirc followed by circ in a private freecyclemap
    UInt initmax;
    FreeCycle   tryfcinit = fc;
    for (auto &trygp : initcirc) {
        UInt tryStartCycle = tryfcinit.get_start_cycle_no_rc(trygp);
        tryfcinit.add_no_rc(trygp, tryStartCycle);
    }
    for (auto &trygp : circ) {
        UInt tryStartCycle = tryfcinit.get_start_cycle_no_rc(trygp);
        tryfcinit.add_no_rc(trygp, tryStartCycle);
    }
    initmax = tryfcinit.get_max(); // this reflects the depth afterwards

    // then fake-schedule circ alone in a private freecyclemap
    UInt max;
    FreeCycle tryfc = fc;
    for (auto &trygp : circ) {
        UInt tryStartCycle = tryfc.get_start_cycle_no_rc(trygp);
        tryfc.add_no_rc(trygp, tryStartCycle);
    }
    max = tryfc.get_max();         // this reflects the depth afterwards

    QL_DOUT("... scheduling init+circ => depth " << initmax << ", scheduling circ => depth " << max << ", init insertion cost " << (initmax - max));
    QL_ASSERT(initmax >= max);
    // scheduling initcirc would be for free when initmax == max, so the cost is (initmax - max)
    return (initmax - max);
}

// add the mapped gate to the current past
// means adding it to the current past's waiting list, waiting for it to be scheduled later
void Past::Add(const ir::GateRef &gp) {
    waitinglg.push_back(gp);
}

// create a new gate with given name and qubits
// return whether this was successful
// return the created gate(s) in circ (which is supposed to be empty on entry)
//
// since kernel.h only provides a gate interface as method of class quantum_kernel, that adds the gate to kernel.c,
// and we want the gate (or its decomposed sequence) here to be added to circ,
// the kludge is implemented to make sure that kernel.c (the current kernel's mapper input/output circuit)
// is available for this:
// in class Future, kernel.c is copied into the dependence graph or copied to a local circuit; and
// in Mapper::MapCircuit, a temporary local output circuit is used, which is written to kernel.c only at the very end
Bool Past::new_gate(
    ir::Circuit &circ,
    const Str &gname,
    const Vec<UInt> &qubits,
    const Vec<UInt> &cregs,
    UInt duration,
    Real angle,
    const Vec<UInt> &bregs,
    ir::ConditionType gcond,
    const Vec<UInt> &gcondregs
) const {
    Bool added;
    QL_ASSERT(circ.empty());
    QL_ASSERT(kernelp->c.empty());
    // create gate(s) in kernelp->c
    added = kernelp->gate_nonfatal(gname, qubits, cregs, duration, angle, bregs, gcond, gcondregs);
    circ = kernelp->c;
    kernelp->c.reset();
    for (auto gp : circ) {
        QL_DOUT("new_gate added: " << gp->qasm());
    }
    QL_ASSERT(!(added && circ.empty()));
    return added;
}

// return number of swaps added to this past
UInt Past::NumberOfSwapsAdded() const {
    return nswapsadded;
}

// return number of moves added to this past
UInt Past::NumberOfMovesAdded() const {
    return nmovesadded;
}

void Past::new_gate_exception(const Str &s) {
    QL_FATAL("gate is not supported by the target platform: '" << s << "'");
}

// will a swap(fr0,fr1) start earlier than a swap(sr0,sr1)?
// is really a short-cut ignoring config file and perhaps several other details
Bool Past::IsFirstSwapEarliest(UInt fr0, UInt fr1, UInt sr0, UInt sr1) const {
    return fc.is_first_swap_earliest(fr0, fr1, sr0, sr1);
}

// generate a move into circ with parameters r0 and r1 (which GenMove may reverse)
// whether this was successfully done can be seen from whether circ was extended
// please note that the reversal of operands may have been done also when GenMove was not successful
void Past::GenMove(ir::Circuit &circ, UInt &r0, UInt &r1) {
    if (v2r.get_state(r0) != QubitState::LIVE) {
        QL_ASSERT(v2r.get_state(r0) == QubitState::NONE ||
                      v2r.get_state(r0) == QubitState::INITIALIZED);
        // interchange r0 and r1, so that r1 (right-hand operand of move) will be the state-less one
        UInt  tmp = r1; r1 = r0; r0 = tmp;
        // QL_DOUT("... reversed operands for move to become move(q" << r0 << ",q" << r1 << ") ...");
    }
    QL_ASSERT(v2r.get_state(r0) == QubitState::LIVE);    // and r0 will be the one with state
    QL_ASSERT(v2r.get_state(r1) != QubitState::LIVE);    // and r1 will be the one without state (QubitState::NONE || QubitState::INITIALIZED)

    // first (optimistically) create the move circuit and add it to circ
    Bool created;
    if (platformp->grid->is_inter_core_hop(r0, r1)) {
        if (options->heuristic == Heuristic::MAX_FIDELITY) {
            created = new_gate(circ, "tmove_prim", {r0,r1});    // gates implementing tmove returned in circ
        } else {
            created = new_gate(circ, "tmove_real", {r0,r1});    // gates implementing tmove returned in circ
        }
        if (!created) {
            created = new_gate(circ, "tmove", {r0,r1});
            if (!created) {
                new_gate_exception("tmove or tmove_real");
            }
        }
    } else {
        if (options->heuristic == Heuristic::MAX_FIDELITY) {
            created = new_gate(circ, "move_prim", {r0,r1});    // gates implementing move returned in circ
        } else {
            created = new_gate(circ, "move_real", {r0,r1});    // gates implementing move returned in circ
        }
        if (!created) {
            created = new_gate(circ, "move", {r0,r1});
            if (!created) {
                new_gate_exception("move or move_real");
            }
        }
    }

    if (v2r.get_state(r1) == QubitState::NONE) {
        // r1 is not in inited state, generate in initcirc the circuit to do so
        // QL_DOUT("... initializing non-inited " << r1 << " to |0> (inited) state preferably using move_init ...");
        ir::Circuit initcirc;

        created = new_gate(initcirc, "move_init", {r1});
        if (!created) {
            created = new_gate(initcirc, "prepz", {r1});
            // if (created)
            // {
            //     created = new_gate(initcirc, "h", {r1});
            //     if (!created) new_gate_exception("h");
            // }
            if (!created) {
                new_gate_exception("move_init or prepz");
            }
        }

        // when difference in extending circuit after scheduling initcirc+circ or just circ
        // is less equal than threshold cycles (0 would mean scheduling initcirc was for free),
        // commit to it, otherwise abort
        if (InsertionCost(initcirc, circ) <= static_cast<utils::Int>(options->max_move_penalty)) {
            // so we go for it!
            // circ contains move; it must get the initcirc before it ...
            // do this by appending circ's gates to initcirc, and then swapping circ and initcirc content
            QL_DOUT("... initialization is for free, do it ...");
            for (auto &gp : circ) {
                initcirc.add(gp);
            }
            circ.get_vec().swap(initcirc.get_vec());
            v2r.set_state(r1, QubitState::INITIALIZED);
        } else {
            // undo damage done, will not do move but swap, i.e. nothing created thisfar
            QL_DOUT("... initialization extends circuit, don't do it ...");
            circ.reset();       // circ being cleared also indicates creation wasn't successful
        }
        // initcirc getting out-of-scope here so gets destroyed
    }
}

// generate a single swap/move with real operands and add it to the current past's waiting list;
// note that the swap/move may be implemented by a series of gates (circuit circ below),
// and that a swap/move essentially is a commutative operation, interchanging the states of the two qubits;
//
// a move is implemented by 2 CNOTs, while a swap by 3 CNOTs, provided the target qubit is in |0> (inited) state;
// so, when one of the operands is the current location of an unused virtual qubit,
// use a move with that location as 2nd operand,
// after first having initialized the target qubit in |0> (inited) state when that has not been done already;
// but this initialization must not extend the depth so can only be done when cycles for it are for free
void Past::AddSwap(UInt r0, UInt r1) {
    Bool created = false;

    QL_DOUT("... extending with swap(q" << r0 << ",q" << r1 << ") ...");
    QL_DOUT("... adding swap/move: " << v2r.real_to_string(r0) << ", " << v2r.real_to_string(r1));

    QL_ASSERT(v2r.get_state(r0) == QubitState::INITIALIZED ||
                  v2r.get_state(r0) == QubitState::NONE ||
                  v2r.get_state(r0) == QubitState::LIVE);
    QL_ASSERT(v2r.get_state(r1) == QubitState::INITIALIZED ||
                  v2r.get_state(r1) == QubitState::NONE ||
                  v2r.get_state(r1) == QubitState::LIVE);

    if (v2r.get_state(r0) != QubitState::LIVE &&
        v2r.get_state(r1) != QubitState::LIVE) {
        QL_DOUT("... no state in both operand of intended swap/move; don't add swap/move gates");
        v2r.swap(r0, r1);
        return;
    }

    // store the virtual qubits corresponding to each real qubit
    UInt v0 = v2r.get_virtual(r0);
    UInt v1 = v2r.get_virtual(r1);

    ir::Circuit circ;   // current kernel copy, clear circuit
    if (options->use_move_gates && (v2r.get_state(r0) != QubitState::LIVE ||
        v2r.get_state(r1) != QubitState::LIVE)) {
        GenMove(circ, r0, r1);
        created = circ.size()!=0;
        if (created) {
            // generated move
            // move is in circ, optionally with initialization in front of it
            // also rs of its 2nd operand is 'QubitState::INITIALIZED'
            // note that after swap/move, r0 will be in this state then
            nmovesadded++;                       // for reporting at the end
            QL_DOUT("... move(q" << r0 << ",q" << r1 << ") ...");
        } else {
            QL_DOUT("... move(q" << r0 << ",q" << r1 << ") cancelled, go for swap");
        }
    }
    if (!created) {
        // no move generated so do swap
        if (options->reverse_swap_if_better) {
            // swap(r0,r1) is about to be generated
            // it is functionally symmetrical,
            // but in the implementation r1 starts 1 cycle earlier than r0 (we should derive this from json file ...)
            // so swap(r0,r1) with interchanged operands might get scheduled 1 cycle earlier;
            // when fcv[r0] < fcv[r1], r0 is free for use 1 cycle earlier than r1, so a reversal will help
            if (fc.is_first_operand_earlier(r0, r1)) {
                UInt  tmp = r1; r1 = r0; r0 = tmp;
                QL_DOUT("... reversed swap to become swap(q" << r0 << ",q" << r1 << ") ...");
            }
        }
        if (platformp->grid->is_inter_core_hop(r0, r1)) {
            if (options->heuristic == Heuristic::MAX_FIDELITY) {
                created = new_gate(circ, "tswap_prim", {r0,r1});    // gates implementing tswap returned in circ
            } else {
                created = new_gate(circ, "tswap_real", {r0,r1});    // gates implementing tswap returned in circ
            }
            if (!created) {
                created = new_gate(circ, "tswap", {r0,r1});
                if (!created) {
                    new_gate_exception("tswap or tswap_real");
                }
            }
            QL_DOUT("... tswap(q" << r0 << ",q" << r1 << ") ...");
        } else {
            if (options->heuristic == Heuristic::MAX_FIDELITY) {
                created = new_gate(circ, "swap_prim", {r0,r1});    // gates implementing swap returned in circ
            } else {
                created = new_gate(circ, "swap_real", {r0,r1});    // gates implementing swap returned in circ
            }
            if (!created) {
                created = new_gate(circ, "swap", {r0,r1});
                if (!created) {
                    new_gate_exception("swap or swap_real");
                }
            }
            QL_DOUT("... swap(q" << r0 << ",q" << r1 << ") ...");
        }
    }
    nswapsadded++;                       // for reporting at the end

    // add each gate in the resulting circuit
    for (auto &gp : circ) {
        Add(gp);
        // each gate in circ is part of a swap or move, so add the parameters
        //TODO: uint to int conversion
        const ir::SwapParamaters swap_params {true, (Int) r0, (Int) r1, (Int) v1, (Int) v0};
        gp->swap_params = swap_params;
    }

    v2r.swap(r0, r1);        // reflect in v2r that r0 and r1 interchanged state, i.e. update the map to reflect the swap
}

// add the mapped gate (with real qubit indices as operands) to the past
// by adding it to the waitinglist and scheduling it into the past
void Past::AddAndSchedule(const ir::GateRef &gp) {
    Add(gp);
    Schedule();
}

// find real qubit index implementing virtual qubit index;
// if not yet mapped, allocate a new real qubit index and map to it
UInt Past::MapQubit(UInt v) {
    UInt  r = v2r[v];
    if (r == UNDEFINED_QUBIT) {
        r = v2r.allocate(v);
    }
    return r;
}

void Past::stripname(Str &name) {
    QL_DOUT("strip_name(name=" << name << ")");
    UInt p = name.find(" ");
    if (p != Str::npos) {
        name = name.substr(0,p);
    }
    QL_DOUT("... after strip_name name=" << name);
}

// MakeReal gp
// assume gp points to a virtual gate with virtual qubit indices as operands;
// when a gate can be created with the same name but with "_real" appended, with the real qubits as operands, then create that gate
// otherwise keep the old gate; replace the virtual qubit operands by the real qubit indices
// since creating a new gate may result in a decomposition to several gates, the result is returned as a circuit vector
//
// So each gate in the circuit (optionally) passes through the following phases:
// 1. it is created:
//      when a decomposition in config file, decompose immediately, otherwise just create (k.gate)
//      so we expect gates like: x, cz, cnot to be specified in config file;
//      on the resulting (decomposed) gates, the routing is done including depth/cost estimation
// 2a.if needed for mapping, swap/move is created:
//      first try creating swap_real/move_real as above, otherwise just swap/real (AddSwap)
//      so we expect gates like: swap_real, move_real to be specified in config file,
//      swap_real/move_real unlike swap/real allow immediate decomposition;
//      when no swap_real/move_real are specified, just swap/move must be present
//      and swap/move are created usually without decomposition;
//      on the resulting (decomposed) gates, the routing is done including depth/cost estimation;
//      when the resulting gates end in _prim, see step 3
// 2b.the resulting gates of step 1: map operands/gate:
//      first try creating gate_real as above, otherwise just gate (MakeReal)
//      gate_real unlike gate allows immediate decomposition;
//      when the resulting gates end in _prim, see step 3
// 3. make primitive gates:
//      for each gate try recreating it with _prim appended to its name, otherwise keep it; this decomposes those with corresponding _prim entries
// 4. final schedule:
//      the resulting gates are subject to final scheduling (the original resource-constrained scheduler)
void Past::MakeReal(ir::GateRef &gp, ir::Circuit &circ) {
    QL_DOUT("MakeReal: " << gp->qasm());

    Str gname = gp->name;
    stripname(gname);

    Vec<UInt> real_qubits = gp->operands;// starts off as copy of virtual qubits!
    for (auto &qi : real_qubits) {
        qi = MapQubit(qi);          // and now they are real
        if (options->assume_prep_only_initializes && (gname == "prepz" || gname == "Prepz")) {
            v2r.set_state(qi, QubitState::INITIALIZED);
        } else {
            v2r.set_state(qi, QubitState::LIVE);
        }
    }

    Str real_gname = gname;
    if (options->heuristic == Heuristic::MAX_FIDELITY) {
        QL_DOUT("MakeReal: with mapper==maxfidelity generate _prim");
        real_gname.append("_prim");
    } else {
        real_gname.append("_real");
    }

    Bool created = new_gate(
        circ,
        real_gname,
        real_qubits,
        gp->creg_operands,
        gp->duration,
        gp->angle,
        gp->breg_operands,
        gp->condition,
        gp->cond_operands
    );
    if (!created) {
        created = new_gate(
            circ,
            gname,
            real_qubits,
            gp->creg_operands,
            gp->duration,
            gp->angle,
            gp->breg_operands,
            gp->condition,
            gp->cond_operands
        );
        if (!created) {
            QL_FATAL("MakeReal: failed creating gate " << real_gname << " or " << gname);
        }
    }
    QL_DOUT("... MakeReal: new gate created for: " << real_gname << " or " << gname);

    if (gp->swap_params.part_of_swap) {
        QL_DOUT("original gate was swap/move, adding swap/move parameters for gates in decomposed circuit");
        for (ir::GateRef &gate : circ) {
            gate->swap_params = gp->swap_params;
        }
    }
}

// as mapper after-burner
// make primitives of all gates that also have an entry with _prim appended to its name
// and decomposing it according to the .json file gate decomposition
void Past::MakePrimitive(ir::GateRef &gp, ir::Circuit &circ) const {
    Str gname = gp->name;
    stripname(gname);
    Str prim_gname = gname;
    prim_gname.append("_prim");
    Bool created = new_gate(
        circ,
        prim_gname,
        gp->operands,
        gp->creg_operands,
        gp->duration,
        gp->angle,
        gp->breg_operands,
        gp->condition,
        gp->cond_operands
    );
    if (!created) {
        created = new_gate(
            circ,
            gname,
            gp->operands,
            gp->creg_operands,
            gp->duration,
            gp->angle,
            gp->breg_operands,
            gp->condition,
            gp->cond_operands
        );
        if (!created) {
            QL_FATAL("MakePrimtive: failed creating gate " << prim_gname << " or " << gname);
        }
    }
    QL_DOUT("... MakePrimtive: new gate created for: " << prim_gname << " or " << gname);

    if (gp->swap_params.part_of_swap) {
        QL_DOUT("original gate was swap/move, adding swap/move parameters for gates in decomposed circuit");
        for (ir::GateRef &gate : circ) {
            gate->swap_params = gp->swap_params;
        }
    }
}

UInt Past::MaxFreeCycle() const {
    return fc.get_max();
}

// nonq and q gates follow separate flows through Past:
// - q gates are put in waitinglg when added and then scheduled; and then ordered by cycle into lg
//      in lg they are waiting to be inspected and scheduled, until [too many are there,] a nonq comes or end-of-circuit
// - nonq gates first cause lg to be flushed/cleared to output before the nonq gate is output
// all gates in outlg are out of view for scheduling/mapping optimization and can be taken out to elsewhere
void Past::FlushAll() {
    for (auto &gp : lg) {
        outlg.push_back(gp);
    }
    lg.clear();         // so effectively, lg's content was moved to outlg

    // fc.Init(platformp, nb); // needed?
    // cycle.clear();      // needed?
    // cycle is initialized to empty map
    // is ok without windowing, but with window, just delete the ones outside the window
}

// gp as nonq gate immediately goes to outlg
void Past::ByPass(const ir::GateRef &gp) {
    if (!lg.empty()) {
        FlushAll();
    }
    outlg.push_back(gp);
}

// mainPast flushes outlg to parameter oc
void Past::Out(ir::Circuit &oc) {
    for (auto &gp : outlg) {
        oc.add(gp);
    }
    outlg.clear();
}

} // namespace detail
} // namespace route
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
