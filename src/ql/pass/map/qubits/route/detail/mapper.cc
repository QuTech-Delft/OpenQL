/** \file
 * OpenQL virtual to real qubit mapping and routing.
 */

#include "mapper.h"

#include "ql/utils/filesystem.h"
#include "ql/com/options.h"
#include "ql/pass/map/qubits/place/detail/initial_place.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace route {
namespace detail {

using namespace utils;
using namespace com;

// access free cycle value of qubit q[i] or breg b[i-nq]
UInt &FreeCycle::operator[](UInt i) {
    return fcv[i];
}

const UInt &FreeCycle::operator[](UInt i) const {
    return fcv[i];
}

// explicit FreeCycle constructor
// needed for virgin construction
// default constructor was deleted because it cannot construct resource_manager_t without parameters
FreeCycle::FreeCycle() {
    QL_DOUT("Constructing FreeCycle");
}

void FreeCycle::Init(const plat::PlatformRef &p) {
    QL_DOUT("FreeCycle::Init()");
    auto rm = plat::resource::Manager::from_defaults(p);   // allocated here and copied below to rm because of platform parameter
                                                           // JvS: I have no idea what ^ means
    QL_DOUT("... created FreeCycle Init local resource_manager");
    platformp = p;
    nq = platformp->qubit_count;
    nb = platformp->breg_count;
    ct = platformp->cycle_time;
    QL_DOUT("... FreeCycle: nq=" << nq << ", nb=" << nb << ", ct=" << ct << "), initializing to all 0 cycles");
    fcv.clear();
    fcv.resize(nq+nb, 1);   // this 1 implies that cycle of first gate will be 1 and not 0; OpenQL convention!?!?
    QL_DOUT("... about to copy FreeCycle Init local resource_manager to FreeCycle member rm");
    rs = rm.build(plat::resource::Direction::FORWARD);
    QL_DOUT("... done copy FreeCycle Init local resource_manager to FreeCycle member rm");
}

// depth of the FreeCycle map
// equals the max of all entries minus the min of all entries
// not used yet; would be used to compute the max size of a top window on the past
UInt FreeCycle::Depth() const {
    return Max() - Min();
}

// min of the FreeCycle map equals the min of all entries;
UInt FreeCycle::Min() const {
    UInt  minFreeCycle = ir::MAX_CYCLE;
    for (const auto &v : fcv) {
        if (v < minFreeCycle) {
            minFreeCycle = v;
        }
    }
    return minFreeCycle;
}

// max of the FreeCycle map equals the max of all entries;
UInt FreeCycle::Max() const {
    UInt maxFreeCycle = 0;
    for (const auto &v : fcv) {
        if (maxFreeCycle < v) {
            maxFreeCycle = v;
        }
    }
    return maxFreeCycle;
}

void FreeCycle::DPRINT(const Str &s) const {
    if (logger::log_level >= logger::LogLevel::LOG_DEBUG) {
        Print(s);
    }
}

void FreeCycle::Print(const Str &s) const {
    UInt  minFreeCycle = Min();
    UInt  maxFreeCycle = Max();
    std::cout << "... FreeCycle" << s << ":";
    for (UInt i = 0; i < nq; i++) {
        UInt v = fcv[i];
        std::cout << " [" << i << "]=";
        if (v == minFreeCycle) {
            std::cout << "_";
        }
        if (v == maxFreeCycle) {
            std::cout << "^";
        }
        std::cout << v;
    }
    std::cout << std::endl;
    // rm.Print("... in FreeCycle: ");
}

// return whether gate with first operand qubit r0 can be scheduled earlier than with operand qubit r1
Bool FreeCycle::IsFirstOperandEarlier(UInt r0, UInt r1) const {
    QL_DOUT("... fcv[" << r0 << "]=" << fcv[r0] << " fcv[" << r1 << "]=" << fcv[r1] << " IsFirstOperandEarlier=" << (fcv[r0] < fcv[r1]));
    return fcv[r0] < fcv[r1];
}

// will a swap(fr0,fr1) start earlier than a swap(sr0,sr1)?
// is really a short-cut ignoring config file and perhaps several other details
Bool FreeCycle::IsFirstSwapEarliest(UInt fr0, UInt fr1, UInt sr0, UInt sr1) const {
    Str mapreverseswapopt = options::get("mapreverseswap");
    if (mapreverseswapopt == "yes") {
        if (fcv[fr0] < fcv[fr1]) {
            UInt  tmp = fr1; fr1 = fr0; fr0 = tmp;
        }
        if (fcv[sr0] < fcv[sr1]) {
            UInt  tmp = sr1; sr1 = sr0; sr0 = tmp;
        }
    }
    UInt startCycleFirstSwap = max(fcv[fr0]-1, fcv[fr1]);
    UInt startCycleSecondSwap = max(fcv[sr0]-1, fcv[sr1]);

    QL_DOUT("... fcv[" << fr0 << "]=" << fcv[fr0] << " fcv[" << fr1 << "]=" << fcv[fr1] << " start=" << startCycleFirstSwap << " fcv[" << sr0 << "]=" << fcv[sr0] << " fcv[" << sr1 << "]=" << fcv[sr1] << " start=" << startCycleSecondSwap << " IsFirstSwapEarliest=" << (startCycleFirstSwap < startCycleSecondSwap));
    return startCycleFirstSwap < startCycleSecondSwap;
}

// when we would schedule gate g, what would be its start cycle? return it
// gate operands are real qubit indices, measure assigned bregs or conditional bregs
// is purely functional, doesn't affect state
UInt FreeCycle::StartCycleNoRc(const ir::GateRef &g) const {
    UInt startCycle = 1;
    for (auto qreg : g->operands) {
        startCycle = max(startCycle, fcv[qreg]);
    }
    for (auto breg : g->breg_operands) {
        startCycle = max(startCycle, fcv[nq+breg]);
    }
    if (g->is_conditional()) {
        for (auto breg : g->cond_operands) {
            startCycle = max(startCycle, fcv[nq+breg]);
        }
    }
    QL_ASSERT (startCycle < ir::MAX_CYCLE);

    return startCycle;
}

// when we would schedule gate g, what would be its start cycle? return it
// gate operands are real qubit indices, measure assigned bregs or conditional bregs
// is purely functional, doesn't affect state
UInt FreeCycle::StartCycle(const ir::GateRef &g) const {
    UInt startCycle = StartCycleNoRc(g);

    auto mapopt = options::get("mapper");
    if (mapopt == "baserc" || mapopt == "minextendrc") {
        UInt baseStartCycle = startCycle;

        while (startCycle < ir::MAX_CYCLE) {
            // QL_DOUT("Startcycle for " << g->qasm() << ": available? at startCycle=" << startCycle);
            if (rs->available(startCycle, g)) {
                // QL_DOUT(" ... [" << startCycle << "] resources available for " << g->qasm());
                break;
            } else {
                // QL_DOUT(" ... [" << startCycle << "] Busy resource for " << g->qasm());
                startCycle++;
            }
        }
        if (baseStartCycle != startCycle) {
            // QL_DOUT(" ... from [" << baseStartCycle << "] to [" << startCycle-1 << "] busy resource(s) for " << g->qasm());
        }
    }
    QL_ASSERT (startCycle < ir::MAX_CYCLE);

    return startCycle;
}

// schedule gate g in the FreeCycle map
// gate operands are real qubit indices, measure assigned bregs or conditional bregs
// the FreeCycle map is updated, not the resource map for operands updated by the gate
// this is done, because AddNoRc is used to represent just gate dependences, avoiding a build of a dep graph
void FreeCycle::AddNoRc(const ir::GateRef &g, UInt startCycle) {
    UInt duration = (g->duration+ct-1)/ct;   // rounded-up unsigned integer division
    UInt freeCycle = startCycle + duration;
    for (auto qreg : g->operands) {
        fcv[qreg] = freeCycle;
    }
    for (auto breg : g->breg_operands) {
        fcv[nq+breg] = freeCycle;
    }
}

// schedule gate g in the FreeCycle and resource maps
// gate operands are real qubit indices, measure assigned bregs or conditional bregs
// both the FreeCycle map and the resource map are updated
// startcycle must be the result of an earlier StartCycle call (with rc!)
void FreeCycle::Add(const ir::GateRef &g, UInt startCycle) {
    AddNoRc(g, startCycle);

    auto mapopt = options::get("mapper");
    if (mapopt == "baserc" || mapopt == "minextendrc") {
        rs->reserve(startCycle, g);
    }
}

// explicit Past constructor
// needed for virgin construction
Past::Past() {
    QL_DOUT("Constructing Past");
}

// past initializer
void Past::Init(const plat::PlatformRef &p, const ir::KernelRef &k) {
    QL_DOUT("Past::Init");
    platformp = p;
    kernelp = k;

    nq = platformp->qubit_count;
    nb = platformp->breg_count;
    ct = platformp->cycle_time;

    QL_ASSERT(kernelp->c.empty());   // kernelp->c will be used by new_gate to return newly created gates into
    v2r.resize(                 // v2r initializtion until v2r is imported from context
        nq,
        com::options::get("mapinitone2one") == "yes",
        com::options::get("mapassumezeroinitstate") == "yes" ? QubitState::INITIALIZED : QubitState::NONE
    );
    fc.Init(platformp);         // fc starts off with all qubits free, is updated after schedule of each gate
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
        fc.Print("");
    }
}

void Past::FcPrint() const{
    fc.Print("");
}

void Past::Print(const Str &s) const {
    std::cout << "... Past " << s << ":";
    v2r.dump_state();
    fc.Print("");
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
            UInt tryStartCycle = tryfc.StartCycle(*trygp_it);
            tryfc.Add(*trygp_it, tryStartCycle);

            if (tryStartCycle < startCycle) {
                startCycle = tryStartCycle;
                gp_it = trygp_it;
            }
        }

        auto gp = *gp_it;

        // add this gate to the maps, scheduling the gate (doing the cycle assignment)
        // QL_DOUT("... add " << gp->qasm() << " startcycle=" << startCycle << " cycles=" << ((gp->duration+ct-1)/ct) );
        fc.Add(gp, startCycle);
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
        UInt tryStartCycle = tryfcinit.StartCycleNoRc(trygp);
        tryfcinit.AddNoRc(trygp, tryStartCycle);
    }
    for (auto &trygp : circ) {
        UInt tryStartCycle = tryfcinit.StartCycleNoRc(trygp);
        tryfcinit.AddNoRc(trygp, tryStartCycle);
    }
    initmax = tryfcinit.Max(); // this reflects the depth afterwards

    // then fake-schedule circ alone in a private freecyclemap
    UInt max;
    FreeCycle tryfc = fc;
    for (auto &trygp : circ) {
        UInt tryStartCycle = tryfc.StartCycleNoRc(trygp);
        tryfc.AddNoRc(trygp, tryStartCycle);
    }
    max = tryfc.Max();         // this reflects the depth afterwards

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
    return fc.IsFirstSwapEarliest(fr0, fr1, sr0, sr1);
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
    auto mapperopt = options::get("mapper");
    if (platformp->grid->is_inter_core_hop(r0, r1)) {
        if (mapperopt == "maxfidelity") {
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
        if (mapperopt == "maxfidelity") {
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
        Int threshold;
        Str mapusemovesopt = options::get("mapusemoves");
        if (mapusemovesopt == "yes") {
            threshold = 0;
        } else {
            threshold = atoi(mapusemovesopt.c_str());
        }
        if (InsertionCost(initcirc, circ) <= threshold) {
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
    Str mapusemovesopt = options::get("mapusemoves");
    if (mapusemovesopt != "no" && (v2r.get_state(r0) != QubitState::LIVE ||
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
        Str mapreverseswapopt = options::get("mapreverseswap");
        if (mapreverseswapopt == "yes") {
            // swap(r0,r1) is about to be generated
            // it is functionally symmetrical,
            // but in the implementation r1 starts 1 cycle earlier than r0 (we should derive this from json file ...)
            // so swap(r0,r1) with interchanged operands might get scheduled 1 cycle earlier;
            // when fcv[r0] < fcv[r1], r0 is free for use 1 cycle earlier than r1, so a reversal will help
            if (fc.IsFirstOperandEarlier(r0, r1)) {
                UInt  tmp = r1; r1 = r0; r0 = tmp;
                QL_DOUT("... reversed swap to become swap(q" << r0 << ",q" << r1 << ") ...");
            }
        }
        auto mapperopt = options::get("mapper");
        if (platformp->grid->is_inter_core_hop(r0, r1)) {
            if (mapperopt == "maxfidelity") {
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
            if (mapperopt == "maxfidelity") {
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
        auto mapprepinitsstateopt = options::get("mapprepinitsstate");
        if (mapprepinitsstateopt == "yes" && (gname == "prepz" || gname == "Prepz")) {
            v2r.set_state(qi, QubitState::INITIALIZED);
        } else {
            v2r.set_state(qi, QubitState::LIVE);
        }
    }

    auto mapperopt = options::get("mapper");
    Str real_gname = gname;
    if (mapperopt == "maxfidelity") {
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
    return fc.Max();
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

// explicit Alter constructor
// needed for virgin construction
Alter::Alter() {
    QL_DOUT("Constructing Alter");
}

// Alter initializer
// This should only be called after a virgin construction and not after cloning a path.
void Alter::Init(const plat::PlatformRef &p, const ir::KernelRef &k) {
    QL_DOUT("Alter::Init(number of qubits=" << p->qubit_count);
    platformp = p;
    kernelp = k;

    nq = platformp->qubit_count;
    ct = platformp->cycle_time;
    // total, fromSource and fromTarget start as empty vectors
    past.Init(platformp, kernelp);      // initializes past to empty
    didscore = false;                   // will not print score for now
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
void Alter::AddSwaps(Past &past, const Str &mapselectswapsopt) const {
    // QL_DOUT("Addswaps " << mapselectswapsopt);
    if (mapselectswapsopt == "one" || mapselectswapsopt == "all") {
        UInt  numberadded = 0;
        UInt  maxnumbertoadd = ("one"==mapselectswapsopt ? 1 : ir::MAX_CYCLE);

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
        QL_ASSERT("earliest" == mapselectswapsopt);
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
    AddSwaps(past, "all");
    // QL_DOUT("... done adding/scheduling swaps to alternative-local past");

    auto mapperopt = options::get("mapper");
    if (mapperopt == "maxfidelity") {
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

// just program wide initialization
void Future::Init(const plat::PlatformRef &p) {
    // QL_DOUT("Future::Init ...");
    platformp = p;
    // QL_DOUT("Future::Init [DONE]");
}

// Set/switch input to the provided circuit
// nq, nc and nb are parameters because nc/nb may not be provided by platform but by kernel
// the latter should be updated when mapping multiple kernels
void Future::SetCircuit(const ir::KernelRef &kernel, const utils::Ptr<Scheduler> &sched, UInt nq, UInt nc, UInt nb) {
    QL_DOUT("Future::SetCircuit ...");
    schedp = sched;
    Str maplookaheadopt = options::get("maplookahead");
    if (maplookaheadopt == "no") {
        input_gatepv = kernel->c;                               // copy to free original circuit to allow outputing to
        input_gatepp = input_gatepv.begin();                    // iterator set to start of input circuit copy
    } else {
        schedp->init(
            kernel,
            com::options::get("output_dir") + "/",
            com::options::get("scheduler_commute") == "yes",
            com::options::get("scheduler_commute_rotations") == "yes"
        );

        // and so also the original circuit can be output to after this
        for (auto &gp : kernel->c) {
            scheduled.set(gp) = false;   // none were scheduled
        }
        scheduled.set(schedp->instruction[schedp->s]) = false;      // also the dummy nodes not
        scheduled.set(schedp->instruction[schedp->t]) = false;
        avlist.clear();
        avlist.push_back(schedp->s);
        schedp->set_remaining(plat::resource::Direction::FORWARD);          // to know criticality

        if (options::get("print_dot_graphs") == "yes") {
            Str map_dot;
            StrStrm fname;

            schedp->get_dot(map_dot);

            fname << options::get("output_dir") << "/" << kernel->name << "_" << "mapper" << ".dot";
            QL_IOUT("writing " << "mapper" << " dependence graph dot file to '" << fname.str() << "' ...");
            OutFile(fname.str()).write(map_dot);
        }
    }
    QL_DOUT("Future::SetCircuit [DONE]");
}

// Get from avlist all gates that are non-quantum into nonqlg
// Non-quantum gates include: classical, and dummy (SOURCE/SINK)
// Return whether some non-quantum gate was found
Bool Future::GetNonQuantumGates(List<ir::GateRef> &nonqlg) const {
    nonqlg.clear();
    Str maplookaheadopt = options::get("maplookahead");
    if (maplookaheadopt == "no") {
        ir::GateRef gp = *input_gatepp;
        if (ir::Circuit::const_iterator(input_gatepp) != input_gatepv.end()) {
            if (
                gp->type() == ir::GateType::CLASSICAL
                || gp->type() == ir::GateType::DUMMY
            ) {
                nonqlg.push_back(gp);
            }
        }
    } else {
        for (auto n : avlist) {
            ir::GateRef gp = schedp->instruction[n];
            if (
                gp->type() == ir::GateType::CLASSICAL
                || gp->type() == ir::GateType::DUMMY
            ) {
                nonqlg.push_back(gp);
            }
        }
    }
    return !nonqlg.empty();
}

// Get all gates from avlist into qlg
// Return whether some gate was found
Bool Future::GetGates(List<ir::GateRef> &qlg) const {
    qlg.clear();
    Str maplookaheadopt = options::get("maplookahead");
    if (maplookaheadopt == "no") {
        if (input_gatepp != input_gatepv.end()) {
            ir::GateRef gp = *input_gatepp;
            if (gp->operands.size() > 2) {
                QL_FATAL(" gate: " << gp->qasm() << " has more than 2 operand qubits; please decompose such gates first before mapping.");
            }
            qlg.push_back(gp);
        }
    } else {
        for (auto n : avlist) {
            ir::GateRef gp = schedp->instruction[n];
            if (gp->operands.size() > 2) {
                QL_FATAL(" gate: " << gp->qasm() << " has more than 2 operand qubits; please decompose such gates first before mapping.");
            }
            qlg.push_back(gp);
        }
    }
    return !qlg.empty();
}

// Indicate that a gate currently in avlist has been mapped, can be taken out of the avlist
// and its successors can be made available
void Future::DoneGate(const ir::GateRef &gp) {
    Str maplookaheadopt = options::get("maplookahead");
    if (maplookaheadopt == "no") {
        input_gatepp = std::next(input_gatepp);
    } else {
        schedp->take_available(schedp->node.at(gp), avlist, scheduled,
                               plat::resource::Direction::FORWARD);
    }
}

// Return gp in lag that is most critical (provided lookahead is enabled)
// This is used in tiebreak, when every other option has failed to make a distinction.
ir::GateRef Future::MostCriticalIn(const List<ir::GateRef> &lag) const {
    Str maplookaheadopt = options::get("maplookahead");
    if (maplookaheadopt == "no") {
        return lag.front();
    } else {
        return schedp->find_mostcritical(lag);
    }
}

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
        a.Init(platformp, kernelp);
        a.targetgp = gp;
        a.Add2Front(src);
        resla.push_back(a);
        a.DPRINT("... empty path after adding to result list");
        Alter::DPRINT("... result list after adding empty path", resla);
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
    QL_ASSERT(platformp->grid->has_coordinates() || com::options::get("mappathselect") != "borders");
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
        a.Add2Front(src);
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
    Str mappathselectopt = options::get("mappathselect");
    if (mappathselectopt == "all") {
        GenShortestPaths(gp, src, tgt, budget, directla, wp_all_shortest);
    } else if (mappathselectopt == "borders") {
        GenShortestPaths(gp, src, tgt, budget, directla, wp_leftright_shortest);
    } else {
        QL_FATAL("Unknown value of mapppathselect option " << mappathselectopt);
    }

    // QL_DOUT("about to split the paths");
    for (auto &a : directla) {
        a.Split(resla);
    }
    // Alter::DPRINT("... after generating and splitting the paths", resla);
}

// Generate all possible variations of making gp NN, starting from given past (with its mappings),
// and return the found variations by appending them to the given list of Alters, la
void Mapper::GenAltersGate(const ir::GateRef &gp, List<Alter> &la, Past &past) {
    auto&   q = gp->operands;
    QL_ASSERT (q.size() == 2);
    UInt  src = past.MapQubit(q[0]);  // interpret virtual operands in past's current map
    UInt  tgt = past.MapQubit(q[1]);
    QL_DOUT("GenAltersGate: " << gp->qasm() << " in real (q" << src << ",q" << tgt << ") at get_min_hops=" << platformp->grid->get_min_hops(src, tgt));
    past.DFcPrint();

    GenShortestPaths(gp, src, tgt, la);// find shortest paths from src to tgt, and split these
    QL_ASSERT(la.size() != 0);
    // Alter::DPRINT("... after GenShortestPaths", la);
}

// Generate all possible variations of making gates in lg NN, starting from given past (with its mappings),
// and return the found variations by appending them to the given list of Alters, la
// Depending on maplookahead only take first (most critical) gate or take all gates.
void Mapper::GenAlters(const List<ir::GateRef> &lg, List<Alter> &la, Past &past) {
    Str maplookaheadopt = options::get("maplookahead");
    if (maplookaheadopt == "all") {
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

    Str maptiebreakopt = options::get("maptiebreak");
    if (maptiebreakopt == "critical") {
        List<ir::GateRef> lag;
        for (auto &a : la) {
            lag.push_back(a.targetgp);
        }
        ir::GateRef gp = future.MostCriticalIn(lag);
        QL_ASSERT(!gp.empty());
        for (auto &a : la) {
            if (a.targetgp.get_ptr() == gp.get_ptr()) {
                // QL_DOUT(" ... took first alternative with most critical target gate");
                return a;
            }
        }
        return la.front();
    }

    if (maptiebreakopt == "random") {
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

    if (maptiebreakopt == "last") {
        // QL_DOUT(" ... took last " << " from 0.." << (la.size()-1));
        return la.back();
    }

    if (maptiebreakopt == "first") {
        // QL_DOUT(" ... took first " << " from 0.." << (la.size()-1));
        return la.front();
    }

    return la.front();  // to shut up gcc
}

// Map the gate/operands of a gate that has been routed or doesn't require routing
void Mapper::MapRoutedGate(ir::GateRef &gp, Past &past) {
    QL_DOUT("MapRoutedGate on virtual: " << gp->qasm() );

    // MakeReal of this gate maps its qubit operands and optionally updates its gate name
    // when the gate name was updated, a new gate with that name is created;
    // when that new gate is a composite gate, it is immediately decomposed (by gate creation)
    // the resulting gate/expansion (anyhow a sequence of gates) is collected in circ
    ir::Circuit circ;   // result of MakeReal
    past.MakeReal(gp, circ);
    for (auto newgp : circ)
    {
        QL_DOUT(" ... new mapped real gate, about to be added to past: " << newgp->qasm() );
        past.AddAndSchedule(newgp);
    }
}

// commit Alter resa
// generating swaps in past
// and taking it out of future when done with it
void Mapper::CommitAlter(Alter &resa, Future &future, Past &past) {
    ir::GateRef resgp = resa.targetgp;   // and the 2q target gate then in resgp
    resa.DPRINT("... CommitAlter, alternative to commit, will add swaps and then map target 2q gate");

    Str mapselectswapsopt = options::get("mapselectswaps");
    resa.AddSwaps(past, mapselectswapsopt);

    // when only some swaps were added, the resgp might not yet be NN, so recheck
    auto &q = resgp->operands;
    if (platformp->grid->get_min_hops(past.MapQubit(q[0]), past.MapQubit(q[1])) == 1) {
        // resgp is NN: so done with this 2q gate
        // QL_DOUT("... CommitAlter, target 2q is NN, map it and done: " << resgp->qasm());
        MapRoutedGate(resgp, past);     // the 2q target gate is NN now and thus can be mapped
        future.DoneGate(resgp);         // and then taken out of future
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
        if (future.GetNonQuantumGates(nonqlg)) {
            // avlist contains non-quantum gates
            // and GetNonQuantumGates indicates these (in nonqlg) must be done first
            QL_DOUT("MapMappableGates, there is a set of non-quantum gates");
            for (auto gp : nonqlg) {
                // here add code to map qubit use of any non-quantum instruction????
                // dummy gates are nonq gates internal to OpenQL such as SOURCE/SINK; don't output them
                if (gp->type() != ir::GateType::DUMMY) {
                    // past only can contain quantum gates, so non-quantum gates must by-pass Past
                    past.ByPass(gp);    // this flushes past.lg first to outlg
                }
                future.DoneGate(gp); // so on avlist= nonNN2q -> NN2q -> 1q -> nonq: the nonq is done first
                QL_DOUT("MapMappableGates, done with " << gp->qasm());
            }
            QL_DOUT("MapMappableGates, done with set of non-quantum gates, continuing ...");
            continue;
        }
        if (!future.GetGates(qlg)) {
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
                future.DoneGate(gp);
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
                UInt  src = past.MapQubit(q[0]);      // interpret virtual operands in current map
                UInt  tgt = past.MapQubit(q[1]);
                UInt  d = platformp->grid->get_min_hops(src, tgt);    // and find minimum number of hops between real counterparts
                if (d == 1) {
                    QL_DOUT("MapMappableGates, NN no routing: " << gp->qasm() << " in real (q" << src << ",q" << tgt << ")");
                    MapRoutedGate(gp, past);
                    future.DoneGate(gp);
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
void Mapper::SelectAlter(List<Alter> &la, Alter &resa, Future &future, Past &past, Past &basePast, Int level) {
    // la are all alternatives we enter with
    QL_ASSERT(!la.empty());  // so there is always a result Alter

    List<Alter> gla;       // good alternative subset of la, suitable to go in recursion with
    List<Alter> bla;       // best alternative subset of gla, suitable to choose result from

    QL_DOUT("SelectAlter ENTRY level=" << level << " from " << la.size() << " alternatives");
    auto mapperopt = options::get("mapper");
    if (mapperopt == "base" || mapperopt == "baserc") {
        Alter::DPRINT("... SelectAlter base (equally good/best) alternatives:", la);
        resa = ChooseAlter(la, future);
        resa.DPRINT("... the selected Alter is");
        // QL_DOUT("SelectAlter DONE level=" << level << " from " << la.size() << " alternatives");
        return;
    }
    QL_ASSERT(mapperopt == "minextend" || mapperopt == "minextendrc" || mapperopt == "maxfidelity");

    // Compute a.score of each alternative relative to basePast, and sort la on it, minimum first
    for (auto &a : la) {
        a.DPRINT("Considering extension by alternative: ...");
        a.Extend(past, basePast);           // locally here, past will be cloned and kept in alter
        // and the extension stored into the a.score
    }
    la.sort([this](const Alter &a1, const Alter &a2) { return a1.score < a2.score; });
    Alter::DPRINT("... SelectAlter sorted all entry alternatives after extension:", la);

    // Reduce sorted list of alternatives (la) to list of good alternatives (gla)
    // suitable to find in recursion which is/are really best;
    // this need not be only those with minimum extend, it can be more.
    // With option mapselectmaxwidth="min" in which "min" stands for minimal number, we get just those minimal ones.
    // With other option values, we are more forgiving but that easily lets the number of alternatives explode.
    gla = la;
    gla.remove_if([this,la](const Alter& a) { return a.score != la.front().score; });
    UInt las = la.size();
    UInt glas = gla.size();
    auto mapselectmaxwidthopt = options::get("mapselectmaxwidth");
    if ("min" != mapselectmaxwidthopt) {
        UInt keep = 1;
        if (mapselectmaxwidthopt == "minplusone") {
            keep = glas+1;
        } else if (mapselectmaxwidthopt == "minplushalfmin") {
            keep = glas+glas/2;
        } else if (mapselectmaxwidthopt == "minplusmin") {
            keep = glas*2;
        } else if (mapselectmaxwidthopt == "all") {
            keep = las;
        } if (keep < las) {
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
    Alter::DPRINT("... SelectAlter good alternatives before recursion:", gla);

    // Prepare for recursion;
    // option mapselectmaxlevel indicates the maximum level of recursion (0 is no recursion)
    auto mapselectmaxlevelstring = options::get("mapselectmaxlevel");
    Int mapselectmaxlevel = (mapselectmaxlevelstring == "inf") ? ir::MAX_CYCLE : parse_int(mapselectmaxlevelstring);

    // When maxlevel has been reached, stop the recursion, and choose from the best minextend/maxfidelity alternatives
    if (level >= mapselectmaxlevel) {
        // Reduce list of good alternatives (gla) to list of minextend/maxfidelity best alternatives (bla)
        // and make a choice from that list to return as result
        bla = gla;
        bla.remove_if([this,gla](const Alter& a) { return a.score != gla.front().score; });
        Alter::DPRINT("... SelectAlter reduced to best alternatives to choose result from:", bla);
        resa = ChooseAlter(bla, future);
        resa.DPRINT("... the selected Alter (STOPPING RECURSION) is");
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
        a.DPRINT("... ... considering alternative:");
        Future future_copy = future;            // copy!
        Past   past_copy = past;                // copy!
        CommitAlter(a, future_copy, past_copy);
        a.DPRINT("... ... committed this alternative first before recursion:");

        Bool    havegates;                  // are there still non-NN 2q gates to map?
        List<ir::GateRef> lg;            // list of non-NN 2q gates taken from avlist, as returned from MapMappableGates
        Str maplookaheadopt = options::get("maplookahead");
        Str maprecNN2qopt = options::get("maprecNN2q");
        // In recursion, look at option maprecNN2q:
        // - MapMappableGates with alsoNN2q==true is greedy and immediately maps each 1q and NN 2q gate
        // - MapMappableGates with alsoNN2q==false is not greedy, maps all 1q gates but not the (NN) 2q gates
        //
        // when yes and when maplookaheadopt is noroutingfirst or all, let MapMappableGates stop mapping only on nonNN2q
        // when no, let MapMappableGates stop mapping on any 2q
        // This creates more clear recursion: one 2q at a time instead of a possible empty set of NN2qs followed by a nonNN2q;
        // also when a NN2q is found, this is perfect; this is not seen when immediately mapping all NN2qs.
        // So goal is to prove that maprecNN2q should be no at this place, in the recursion step, but not at level 0!
        Bool alsoNN2q = (maprecNN2qopt == "yes") && (maplookaheadopt == "noroutingfirst" || maplookaheadopt == "all");
        havegates = MapMappableGates(future_copy, past_copy, lg, alsoNN2q); // map all easy gates; remainder returned in lg

        if (havegates) {
            QL_DOUT("... ... SelectAlter level=" << level << ", committed + mapped easy gates, now facing " << lg.size() << " 2q gates to evaluate next");
            List<Alter> la;                // list that will hold all variations, as returned by GenAlters
            GenAlters(lg, la, past_copy);       // gen all possible variations to make gates in lg NN, in current past.v2r mapping
            QL_DOUT("... ... SelectAlter level=" << level << ", generated for these 2q gates " << la.size() << " alternatives; RECURSE ... ");
            Alter resa;                         // result alternative selected and returned by next SelectAlter call
            SelectAlter(la, resa, future_copy, past_copy, basePast, level+1); // recurse, best in resa ...
            resa.DPRINT("... ... SelectAlter, generated for these 2q gates ... ; RECURSE DONE; resulting alternative ");
            a.score = resa.score;               // extension of deep recursion is treated as extension at current level,
            // by this an alternative started bad may be compensated by deeper alts
        } else {
            QL_DOUT("... ... SelectAlter level=" << level << ", no gates to evaluate next; RECURSION BOTTOM");
            auto mapperopt = options::get("mapper");
            if (mapperopt == "maxfidelity") {
                QL_FATAL("Mapper option maxfidelity has been disabled");
                // a.score = quick_fidelity(past_copy.lg);
            } else {
                a.score = past_copy.MaxFreeCycle() - basePast.MaxFreeCycle();
            }
            a.DPRINT("... ... SelectAlter, after committing this alternative, mapped easy gates, no gates to evaluate next; RECURSION BOTTOM");
        }
        a.DPRINT("... ... DONE considering alternative:");
    }
    // Sort list of good alternatives (gla) on score resulting after recursion
    gla.sort([this](const Alter &a1, const Alter &a2) { return a1.score < a2.score; });
    Alter::DPRINT("... SelectAlter sorted alternatives after recursion:", gla);

    // Reduce list of good alternatives (gla) of before recursion to list of equally minimal best alternatives now (bla)
    // and make a choice from that list to return as result
    bla = gla;
    bla.remove_if([this,gla](const Alter& a) { return a.score != gla.front().score; });
    Alter::DPRINT("... SelectAlter equally best alternatives on return of RECURSION:", bla);
    resa = ChooseAlter(bla, future);
    resa.DPRINT("... the selected Alter is");
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
    Str maplookaheadopt = options::get("maplookahead");
    Bool alsoNN2q = (maplookaheadopt == "noroutingfirst" || maplookaheadopt == "all");
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

    future.Init(platformp);
    future.SetCircuit(kernel, sched, nq, nc, nb); // constructs depgraph, initializes avlist, ready for producing gates
    kernel->c.reset();      // future has copied kernel.c to private data; kernel.c ready for use by new_gate
    kernelp = kernel;      // keep kernel to call kernelp->gate() inside Past.new_gate(), to create new gates

    mainPast.Init(platformp, kernelp);  // mainPast and Past clones inside Alters ready for generating output schedules into
    mainPast.ImportV2r(v2r);    // give it the current mapping/state
    // mainPast.DPRINT("start mapping");

    MapGates(future, mainPast, mainPast);
    mainPast.FlushAll();                // all output to mainPast.outlg, the output window of mainPast

    // mainPast.DPRINT("end mapping");

    QL_DOUT("... retrieving outCirc from mainPast.outlg; swapping outCirc with kernel.c, kernel.c contains output circuit");
    ir::Circuit outCirc;
    mainPast.Out(outCirc);                          // copy (final part of) mainPast's output window into this outCirc
    kernel->c.get_vec().swap(outCirc.get_vec());    // and then to kernel.c
    kernel->cycles_valid = true;                    // decomposition was scheduled in; see Past.Add() and Past.Schedule()
    mainPast.ExportV2r(v2r);
    nswapsadded = mainPast.NumberOfSwapsAdded();
    nmovesadded = mainPast.NumberOfMovesAdded();
}

// decompose all gates that have a definition with _prim appended to its name
void Mapper::MakePrimitives(const ir::KernelRef &kernel) {
    QL_DOUT("MakePrimitives circuit ...");

    ir::Circuit input_gatepv = kernel->c;       // copy to allow kernel.c use by Past.new_gate
    kernel->c.reset();                          // kernel.c ready for use by new_gate

    Past mainPast;                              // output window in which gates are scheduled
    mainPast.Init(platformp, kernelp);

    for (auto & gp : input_gatepv) {
        ir::Circuit tmpCirc;
        mainPast.MakePrimitive(gp, tmpCirc);    // decompose gp into tmpCirc; on failure, copy gp into tmpCirc
        for (auto newgp : tmpCirc) {
            mainPast.AddAndSchedule(newgp);     // decomposition is scheduled in gate by gate
        }
    }
    mainPast.FlushAll();

    ir::Circuit outCirc;                        // ultimate output gate stream
    mainPast.Out(outCirc);
    kernel->c.get_vec().swap(outCirc.get_vec());
    kernel->cycles_valid = true;                 // decomposition was scheduled in above

    QL_DOUT("MakePrimitives circuit [DONE]");
}

// map kernel's circuit, main mapper entry once per kernel
void Mapper::Map(const ir::KernelRef &kernel) {
    QL_DOUT("Mapping kernel " << kernel->name << " [START]");
    QL_DOUT("... kernel original virtual number of qubits=" << kernel->qubit_count);
    kernelp.reset();            // no new_gates until kernel.c has been copied

    auto mapassumezeroinitstateopt = options::get("mapassumezeroinitstate");
    QL_DOUT("Mapper::Map before v2r.Init: mapassumezeroinitstateopt=" << mapassumezeroinitstateopt);

    // unify all incoming v2rs into v2r to compute kernel input mapping;
    // but until inter-kernel mapping is implemented, take program initial mapping for it
    QubitMapping v2r{            // current mapping while mapping this kernel
        nq,
        com::options::get("mapinitone2one") == "yes",
        com::options::get("mapassumezeroinitstate") == "yes" ? QubitState::INITIALIZED : QubitState::NONE
    };
    QL_IF_LOG_DEBUG {
        QL_DOUT("After initialization");
        v2r.dump_state();
    }

    v2r_in = v2r;  // for reporting

    Str initialplaceopt = options::get("initialplace");
    if (initialplaceopt != "no") {
#ifdef INITIALPLACE
        Str initialplace2qhorizonopt = options::get("initialplace2qhorizon");
        QL_DOUT("InitialPlace: kernel=" << kernel->name << " initialplace=" << initialplaceopt << " initialplace2qhorizon=" << initialplace2qhorizonopt << " [START]");
        using namespace place::detail;
        InitialPlace    ip;             // initial placer facility
        ipr_t           ipok;           // one of several ip result possibilities
        Real          iptimetaken;      // time solving the initial placement took, in seconds

        ip.Init(platformp);
        ip.Place(kernel->c, v2r, ipok, iptimetaken, initialplaceopt); // compute mapping (in v2r) using ip model, may fail
        QL_DOUT("InitialPlace: kernel=" << kernel->name << " initialplace=" << initialplaceopt << " initialplace2qhorizon=" << initialplace2qhorizonopt << " result=" << ip.ipr2string(ipok) << " iptimetaken=" << iptimetaken << " seconds [DONE]");
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

    QL_DOUT("Mapper::Map before MapCircuit: mapassumezeroinitstateopt=" << mapassumezeroinitstateopt);

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
void Mapper::Init(const plat::PlatformRef &p) {
    // QL_DOUT("Mapping initialization ...");
    // QL_DOUT("... Grid initialization: platform qubits->coordinates, ->neighbors, distance ...");
    platformp = p;
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
