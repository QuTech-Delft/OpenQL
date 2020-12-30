/** \file
 * OpenQL virtual to real qubit mapping and routing.
 */

#pragma once

#include <random>
#include <chrono>
#include <ctime>
#include <ratio>
#include "utils/map.h"
#include "utils/vec.h"
#include "utils/list.h"
#include "utils/str.h"
#include "utils/num.h"
#include "platform.h"
#include "kernel.h"
#include "resource_manager.h"
#include "gate.h"
#include "scheduler.h"
//#include "metrics.h"

namespace ql {
namespace mapper {

// Note on the use of constructors and Init functions for classes of the mapper
// -----------------------------------------------------------------------------
// Almost all classes of the mapper have one or more members that require initialization
// using a value that was passed on to the Mapper.Init function as a parameter (i.e. platform, cycle_time).
// Dealing with those initializations in the nested constructors was cumbersome.
// Hence, the constructors create just skeleton objects which need explicit initialization before use.
// Such initialization is provided by a class local Init function for a virgin object,
// or by copying an existing object to it.
// The constructors are trivial by this and can be synthesized by default.
//
// Construction of skeleton objects requires the used classes to provide such (non-parameterized) constructors.



// =========================================================================================
// Grid: definition and access functions to the grid of qubits that supports the real qubits.
// Maintain several maps to ease navigating in the grid; these are constant after initialization.

// Grid
//
// Config file definitions:
//  nq:                 hardware_settings.qubit_number
//  ncores:             hardware_settings.number_of_cores
//  topology.conn;      gc_specified/gc_full: topology.connectivity: how connectivity between qubits is specified
//  topology.form;      gf_xy/gf_irregular: topology.form: how relation between neighbors is specified
//  topology.x_size/y_size: x/y space, defines underlying grid (only gf_xy)
//  topology.qubits:    mapping of qubit to x/y coordinates (defines x[i]/y[i] for each qubit i) (only gf_xy)
//  topology.edges:     mapping of edge (physical connection between 2 qubits) to its src and dst qubits (defines nbs)
//
// Grid public members (apart from nq):
//  form:               how relation between neighbors is specified
//  Distance(qi,qj):    distance in physical connection hops from real qubit qi to real qubit qj;
//                      - computing it relies on nbs (and Floyd-Warshall) (gf_xy and gf_irregular)
//  nbs[qi]:            list of neighbor real qubits of real qubit qi
//                      - nbs can be derived from topology.edges (gf_xy and gf_irregular)
//  Normalize(qi, neighborlist):    rotate neighborlist such that largest angle diff around qi is behind last element
//                      relies on nbs, and x[i]/y[i] (gf_xy only)
//
// For an irregular grid form, only nq and edges (so nbs) need to be specified; distance is computed from nbs:
// - there is no underlying rectangular grid, so there are no defined x and y coordinates of qubits;
//   this means that Normalize as needed by mappathselect==borders cannot work
// Below, we support regular (xy) grids which need not be fully assigned; this requires edges (so nbs) to be defined,
//   from which distance is computed; also we have x/y coordinates per qubit specified in the configuration file
//   An underlying grid with x/y coordinates comes in use for:
//   - crossbars
//   - cclight qwg assignment (done in another manner now)
//   - when mappathselectopt==borders
//
// Not implemented:
// forms gf_cross and gf_plus: given x_size and y_size, the relations are implicitly defined by the internal
//      diagonal (gf_cross) or horizontal/vertical (gf_plus) connections between grid points (qubits);
//      with gf_cross only half the grid is occupied by a qubit; the grid point (0,0) doesn't have a qubit, (1,0) and (0,1) do;
//      topology.qubits and topology.edges need not be present in the configuration file;
//      Distance in both forms would be defined by a formula, not a function
typedef enum GridConnectivity {
    gc_specified,   // "specified": edges are specified in "edges" section
    gc_full         // "full": qubits are fully connected by edges
} gridconn_t;

typedef enum GridForms {
    gf_xy,          // nodes have explicit neighbor definitions, qubits have explicit x/y coordinates
    gf_irregular    // nodes have explicit neighbor definitions, qubits don't have x/y coordinates
} gridform_t;

class Grid {
public:
    const quantum_platform *platformp;    // current platform: topology
    utils::UInt nq;                       // number of qubits in the platform
    utils::UInt ncores;                   // number of cores in the platform
    // Grid configuration, all constant after initialization
    gridform_t form;                      // form of grid
    gridconn_t conn;                      // connectivity of grid
    utils::Int nx;                        // length of x dimension (x coordinates count 0..nx-1)
    utils::Int ny;                        // length of y dimension (y coordinates count 0..ny-1)

    typedef utils::List<utils::UInt> neighbors_t;  // neighbors is a list of qubits
    utils::Map<utils::UInt,neighbors_t> nbs;       // nbs[i] is list of neighbor qubits of qubit i
    utils::Map<utils::UInt,utils::Int> x;          // x[i] is x coordinate of qubit i
    utils::Map<utils::UInt,utils::Int> y;          // y[i] is y coordinate of qubit i
    utils::Vec<utils::Vec<utils::UInt>> dist;      // dist[i][j] is computed distance between qubits i and j;

    // Grid initializer
    // initialize mapper internal grid maps from configuration
    // this remains constant over multiple kernels on the same platform
    void Init(const quantum_platform *p);

    // core index from qubit index
    // when multi-core assumes full and uniform core connectivity
    utils::UInt CoreOf(utils::UInt qi) const;

    // inter-core hop from qs to qt?
    utils::Bool IsInterCoreHop(utils::UInt qs, utils::UInt qt) const;

    // distance between two qubits
    // formulae for convex (hole free) topologies with underlying grid and with bidirectional edges:
    //      gf_cross:   max( abs( x[to_realqi] - x[from_realqi] ), abs( y[to_realqi] - y[from_realqi] ))
    //      gf_plus:    abs( x[to_realqi] - x[from_realqi] ) + abs( y[to_realqi] - y[from_realqi] )
    // when the neighbor relation is defined (topology.edges in config file), Floyd-Warshall is used, which currently is always
    utils::UInt Distance(utils::UInt from_realqi, utils::UInt to_realqi) const;

    // coredistance between two qubits
    // when multi-core assumes full and uniform core connectivity
    utils::UInt CoreDistance(utils::UInt from_realqi, utils::UInt to_realqi) const;

    // minimum number of hops between two qubits is always >= distance(from, to)
    // and inside one core (or without multi-core) the minimum number of hops == distance
    //
    // however, in multi-core with inter-core hops, an inter-core hop cannot execute a 2qgate
    // so when the minimum number of hops are all inter-core hops (so distance(from,to) == coredistance(from,to))
    // and no 2qgate has been placed yet, then at least one additional inter-core hop is needed for the 2qgate,
    // the number of hops required being at least distance+1;
    //
    // we assume below that a valid path exists with distance+1 hops;
    // this fails when not all qubits in a core support connections to all other cores;
    // see the check in InitNbs
    utils::UInt MinHops(utils::UInt from_realqi, utils::UInt to_realqi) const;

    // return clockwise angle around (cx,cy) of (x,y) wrt vertical y axis with angle 0 at 12:00, 0<=angle<2*pi
    utils::Real Angle(utils::Int cx, utils::Int cy, utils::Int x, utils::Int y) const;

    // rotate neighbors list such that largest angle difference between adjacent elements is behind back;
    // this is needed when a given subset of variations from a node is wanted (mappathselect==borders);
    // and this can only be computed when there is an underlying x/y grid (so not for form==gf_irregular)
    void Normalize(utils::UInt src, neighbors_t &nbl) const;

    // Floyd-Warshall dist[i][j] = shortest distances between all nq qubits i and j
    void ComputeDist();

    void DPRINTGrid() const;
    void PrintGrid() const;

    // init multi-core attributes
    void InitCores();

    // init x, and y maps
    void InitXY();

    // init nbs map
    void InitNbs();

    void SortNbs();

};

// =========================================================================================
// Virt2Real: map of a virtual qubit index to its real qubit index
//
// Mapping maps each used virtual qubit to a real qubit index, but which one that is, may change.
// For a 2-qubit gate its operands should be nearest neighbor; when its virtual operand qubits
// are not mapping to nearest neighbors, that should be accomplished by moving/swapping
// the virtual qubits from their current real qubits to real qubits that are nearest neighbors:
// those moves/swaps are inserted just before that 2-qubit gate.
// Anyhow, the virtual operand qubits of gates must be mapped to the real ones, holding their state.
//
// The number of virtual qubits is less equal than the number of real qubits,
// so their indices use the same data type (utils::UInt) and the same range type 0<=index<nq.
//
// Virt2Real maintains two maps:
// - a map (v2rMap[]) for each virtual qubit that is in use to its current real qubit index.
//      Virtual qubits are in use as soon as they have been encountered as operands in the program.
//      When a virtual qubit is not in use, it maps to UNDEFINED_QUBIT, the undefined real index.
//      The reverse map (GetVirt()) is implemented by a reverse look-up:
//      when there is no virtual qubit that maps to a particular real qubit,
//      the reverse map maps the real qubit index to UNDEFINED_QUBIT, the undefined virtual index.
//      At any time, the virtual to real and reverse maps are 1-1 for qubits that are in use.
// - a map for each real qubit whether there is state in it, and, if so, which (rs[]).
//      When a gate (except for swap/move) has been executed on a real qubit,
//      its state becomes valuable and must be preserved (rs_hasstate below).
//      But before that, it can be in a garbage state (rs_nostate below) or in a known state (rs_wasinited below).
//      The latter is used to replace a swap using a real qubit with such state by a move, which is cheaper.
// There is no support yet to make a virtual qubit not in use (which could be after a measure),
// nor to bring a real qubit in the rs_wasinited or rs_nostate state (perhaps after measure or prep).
//
// Some special situations are worth mentioning:
// - while a virtual qubit is being swapped/moved near to an other one,
//      along the trip real qubits may be used which have no virtual qubit mapping to them;
//      a move can then be used which assumes the 2nd real operand in the |0> (inited) state, and leaves
//      the 1st real operand in that state (while the 2nd has assumed the state of the former 1st).
//      the mapper implementation assumes that all real qubits in the rs_wasinited state are in that state.
// - on program start, no virtual qubit has a mapping yet to a real qubit;
//      mapping is initialized while virtual qubits are encountered as operands.
// - with multiple kernels, kernels assume the (unified) mapping from their predecessors and leave
//      the result mapping to their successors in the kernels' Control Flow Graph;
//      i.e. Virt2Real is what is passed between kernels as dynamic state;
//      statically, the grid, the maximum number of real qubits and the current platform stay unchanged.
// - while evaluating sets of swaps/moves as variations to continue mapping, Virt2Real is passed along
//      to represent the mapping state after such swaps/moves where done; when deciding on a particular
//      variation, the v2r mapping in the mainPast is made to replect the swaps/moves done.
typedef enum realstate {
    rs_nostate,     // real qubit has no relevant state needing preservation, i.e. is garbage
    rs_wasinited,   // real qubit has initialized state suitable for replacing swap by move
    rs_hasstate     // real qubit has a unique state which must be preserved
} realstate_t;

const utils::UInt UNDEFINED_QUBIT = utils::MAX;


class Virt2Real {
private:

    utils::UInt             nq;     // size of the map; after initialization, will always be the same
    utils::Vec<utils::UInt> v2rMap; // v2rMap[virtual qubit index] -> real qubit index | UNDEFINED_QUBIT
    utils::Vec<realstate_t> rs;     // rs[real qubit index] -> {nostate|wasinited|hasstate}

public:

    // map real qubit to the virtual qubit index that is mapped to it (i.e. backward map);
    // when none, return UNDEFINED_QUBIT;
    // a second vector next to v2rMap (i.e. an r2vMap) would speed this up;
    utils::UInt GetVirt(utils::UInt r) const;
    realstate_t GetRs(utils::UInt q) const;
    void SetRs(utils::UInt q, realstate_t rsvalue);

    // expand to desired size
    //
    // mapping starts off undefined for all virtual qubits
    // (unless option mapinitone2one is set, then virtual qubit i maps to real qubit i for all qubits)
    //
    // real qubits are assumed to have a garbage state
    // (unless option mapassumezeroinitstate was set,
    //  then all real qubits are assumed to have a state suitable for replacing swap by move)
    //
    // the rs initializations are done only once, for a whole program
    void Init(utils::UInt n);

    // map virtual qubit index to real qubit index
    utils::UInt &operator[](utils::UInt v);
    const utils::UInt &operator[](utils::UInt v) const;

    // allocate a new real qubit for an unmapped virtual qubit v (i.e. v2rMap[v] == UNDEFINED_QUBIT);
    // note that this may consult the grid or future gates to find a best real
    // and thus should not be in Virt2Real but higher up
    utils::UInt AllocQubit(utils::UInt v);

    // r0 and r1 are real qubit indices;
    // by execution of a swap(r0,r1), their states are exchanged at runtime;
    // so when v0 was in r0 and v1 was in r1, then v0 is now in r1 and v1 is in r0;
    // update v2r accordingly
    void Swap(utils::UInt r0, utils::UInt r1);

    void DPRINTReal(utils::UInt r) const;
    void PrintReal(utils::UInt r) const;
    void PrintVirt(utils::UInt v) const;
    void DPRINTReal(const utils::Str &s, utils::UInt r0, utils::UInt r1) const;
    void PrintReal(const utils::Str &s, utils::UInt r0, utils::UInt r1) const;
    void DPRINT(const utils::Str &s) const;
    void Print(const utils::Str &s) const;
    void Export(utils::Vec<utils::UInt> &kv2rMap) const;
    void Export(utils::Vec<utils::Int> &krs) const;

};

// =========================================================================================
// FreeCycle: map each real qubit to the first cycle that it is free for use
//
// in scheduling gates, qubit dependencies cause latencies
// for each real qubit, the first cycle that it is free to use is the cycle that the
// last gate that was scheduled in the qubit, has just finished (i.e. in the previous cycle);
// the map serves as a summary to ease scheduling next gates
//
// likewise, while mapping, swaps are scheduled just before a non-NN two-qubit gate,
// moreover, such swaps may involve real qubits on the path between the real operand qubits of the gate,
// which may be different from the real operand qubits;
// the evaluation of which path of swaps is best is, among other data, based
// on which path causes the latency of the whole circuit to be extended the least;
// this latency extension is measured from the data in the FreeCycle map;
// so a FreeCycle map is part of each path of swaps that is evaluated for a particular non-NN 2-qubit gate
// next to a FreeCycle map that is part of the output stream (the main past)
//
// since gate durations are in nano-seconds, and one cycle is some fixed number of nano-seconds,
// the duration is converted to a rounded-up number of cycles when computing the added latency
class FreeCycle {
private:

    const quantum_platform   *platformp;  // platform description
    utils::UInt              nq;          // map is (nq+nb) long; after initialization, will always be the same
    utils::UInt              nb;          // bregs are in map (behind qubits) to track dependences around conditions
    utils::UInt              ct;          // multiplication factor from cycles to nano-seconds (unit of duration)
    utils::Vec<utils::UInt>  fcv;         // fcv[real qubit index i]: qubit i is free from this cycle on
    arch::resource_manager_t rm;          // actual resources occupied by scheduled gates


    // access free cycle value of qubit q[i] or breg b[i-nq]
    utils::UInt &operator[](utils::UInt i);
    const utils::UInt &operator[](utils::UInt i) const;

public:

    // explicit FreeCycle constructor
    // needed for virgin construction
    // default constructor was deleted because it cannot construct resource_manager_t without parameters
    FreeCycle();

    void Init(const quantum_platform *p, const utils::UInt breg_count);

    // depth of the FreeCycle map
    // equals the max of all entries minus the min of all entries
    // not used yet; would be used to compute the max size of a top window on the past
    utils::UInt Depth() const;

    // min of the FreeCycle map equals the min of all entries;
    utils::UInt Min() const;

    // max of the FreeCycle map equals the max of all entries;
    utils::UInt Max() const;

    void DPRINT(const utils::Str &s) const;
    void Print(const utils::Str &s) const;

    // return whether gate with first operand qubit r0 can be scheduled earlier than with operand qubit r1
    utils::Bool IsFirstOperandEarlier(utils::UInt r0, utils::UInt r1) const;

    // will a swap(fr0,fr1) start earlier than a swap(sr0,sr1)?
    // is really a short-cut ignoring config file and perhaps several other details
    utils::Bool IsFirstSwapEarliest(utils::UInt fr0, utils::UInt fr1, utils::UInt sr0, utils::UInt sr1) const;

    // when we would schedule gate g, what would be its start cycle? return it
    // gate operands are real qubit indices and breg indices
    // is purely functional, doesn't affect state
    utils::UInt StartCycleNoRc(gate *g) const;

    // when we would schedule gate g, what would be its start cycle? return it
    // gate operands are real qubit indices and breg indices
    // is purely functional, doesn't affect state
    // FIXME JvS: except it does. can't make it (or the gate) const, because
    //   resource managers are relying on random map[] operators in debug prints
    //   to create new default-initialized keys. Fun!
    utils::UInt StartCycle(gate *g);

    // schedule gate g in the FreeCycle map
    // gate operands are real qubit indices and breg indices
    // the FreeCycle map is updated, not the resource map
    // this is done, because AddNoRc is used to represent just gate dependences, avoiding a build of a dep graph
    void AddNoRc(gate *g, utils::UInt startCycle);

    // schedule gate g in the FreeCycle and resource maps
    // gate operands are real qubit indices and breg indices
    // both the FreeCycle map and the resource map are updated
    // startcycle must be the result of an earlier StartCycle call (with rc!)
    void Add(gate *g, utils::UInt startCycle);

};

// =========================================================================================
// Past: state of the mapper while somewhere in the mapping process
//
// there is a Past attached to the output stream, that is a kind of window with a list of gates in it,
// to which gates are added after mapping; this is called the 'main' Past.
// while mapping, several alternatives are evaluated, each of which also has a Past attached,
// and each of which for most of the parts starts off as a copy of the 'main' Past;
// but it is in fact a temporary extension of this main Past
//
// Past contains gates of which the schedule might influence a future path selected for mapping binary gates
// It maintains for each qubit from which cycle on it is free, so that swap insertion
// can exploit this to hide its overall circuit latency overhead by increasing ILP.
// Also it maintains the 1 to 1 (reversible) virtual to real qubit map: all gates in past
// and beyond are mapped and have real qubits as operands.
// While experimenting with path alternatives, a clone is made of the main past,
// to insert swaps and evaluate the latency effects; note that inserting swaps changes the mapping.
//
// On arrival of a quantum gate(s):
// - [isempty(waitinglg)]
// - if 2q nonNN clone mult. pasts, in each clone Add swap/move gates, Schedule, evaluate clones, select, Add swaps to mainPast
// - Add, Add, ...: add quantum gates to waitinglg, waiting to be scheduled in [!isempty(waitinglg)]
// - Schedule: schedules all quantum gates of waitinglg into lg [isempty(waitinglg) && !isempty(lg)]
// On arrival of a classical gate:
// - FlushAll: lg flushed to outlg [isempty(waitinglg) && isempty(lg) && !isempty(outlg)]
// - ByPass: classical gate added to outlg [isempty(waitinglg) && isempty(lg) && !isempty(outlg)]
// On no gates:
// - [isempty(waitinglg)]
// - FlushAll: lg flushed to outlg [isempty(waitinglg) && isempty(lg) && !isempty(outlg)]
// On end:
// - Out: outlg flushed to outCirc [isempty(waitinglg) && isempty(lg) && isempty(outlg)]
class Past {
private:

    utils::UInt                 nq;         // width of Past, Virt2Real, UseCount maps in number of real qubits
    utils::UInt                 nb;         // extends FreeCycle next to qubits with bregs
    utils::UInt                 ct;         // cycle time, multiplier from cycles to nano-seconds
    const quantum_platform      *platformp; // platform describing resources for scheduling
    quantum_kernel              *kernelp;   // current kernel for creating gates
    Grid                        *gridp;     // pointer to grid to know which hops are inter-core

    Virt2Real                   v2r;        // state: current Virt2Real map, imported/exported to kernel
    FreeCycle                   fc;         // state: FreeCycle map (including resource_manager) of this Past
    typedef gate *      gate_p;
    utils::List<gate_p>         waitinglg;  // . . .  list of q gates in this Past, topological order, waiting to be scheduled in
    //        waitinglg only contains gates from Add and final Schedule call
    //        when evaluating alternatives, it is empty when Past is cloned; so no state
public:
    utils::List<gate_p>         lg;         // state: list of q gates in this Past, scheduled by their (start) cycle values
    //        so this is the result list of this Past, to compare with other Alters
private:
    utils::List<gate_p>         outlg;      // . . .  list of gates flushed out of this Past, not yet put in outCirc
    //        when evaluating alternatives, outlg stays constant; so no state
    utils::Map<gate_p,utils::UInt> cycle;      // state: gate to cycle map, startCycle value of each past gatecycle[gp]
    //        cycle[gp] can be different for each gp for each past
    //        gp->cycle is not used by MapGates
    //        although updated by set_cycle called from MakeAvailable/TakeAvailable
    utils::UInt                  nswapsadded;// number of swaps (including moves) added to this past
    utils::UInt                  nmovesadded;// number of moves added to this past

public:

    // explicit Past constructor
    // needed for virgin construction
    Past();

    // past initializer
    void Init(const quantum_platform *p, quantum_kernel *k, Grid *g);

    // import Past's v2r from v2r_value
    void ImportV2r(const Virt2Real &v2r_value);

    // export Past's v2r into v2r_destination
    void ExportV2r(Virt2Real &v2r_destination) const;

    void DFcPrint() const;
    void FcPrint() const;
    void Print(const utils::Str &s) const;

    // all gates in past.waitinglg are scheduled here into past.lg
    // note that these gates all are mapped and so have real operand qubit indices
    // the FreeCycle map reflects for each qubit the first free cycle
    // all new gates, now in waitinglist, get such a cycle assigned below, increased gradually, until definitive
    void Schedule();

    // compute costs in cycle extension of optionally scheduling initcirc before the inevitable circ
    utils::Int InsertionCost(const circuit &initcirc, const circuit &circ) const;

    // add the mapped gate to the current past
    // means adding it to the current past's waiting list, waiting for it to be scheduled later
    void Add(gate_p gp);

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
    utils::Bool new_gate(
        circuit &circ,
        const utils::Str &gname,
        const utils::Vec<utils::UInt> &qubits,
        const utils::Vec<utils::UInt> &cregs = {},
        utils::UInt duration = 0,
        utils::Real angle = 0.0,
        const utils::Vec<utils::UInt> &bregs = {},
        cond_type_t gcond = cond_always,
        const utils::Vec<utils::UInt> &gcondregs = {}
    ) const;

    // return number of swaps added to this past
    utils::UInt NumberOfSwapsAdded() const;

    // return number of moves added to this past
    utils::UInt NumberOfMovesAdded() const;

    static void new_gate_exception(const utils::Str &s);

    // will a swap(fr0,fr1) start earlier than a swap(sr0,sr1)?
    // is really a short-cut ignoring config file and perhaps several other details
    utils::Bool IsFirstSwapEarliest(utils::UInt fr0, utils::UInt fr1, utils::UInt sr0, utils::UInt sr1) const;

    // generate a move into circ with parameters r0 and r1 (which GenMove may reverse)
    // whether this was successfully done can be seen from whether circ was extended
    // please note that the reversal of operands may have been done also when GenMove was not successful
    void GenMove(circuit &circ, utils::UInt &r0, utils::UInt &r1);

    // generate a single swap/move with real operands and add it to the current past's waiting list;
    // note that the swap/move may be implemented by a series of gates (circuit circ below),
    // and that a swap/move essentially is a commutative operation, interchanging the states of the two qubits;
    //
    // a move is implemented by 2 CNOTs, while a swap by 3 CNOTs, provided the target qubit is in |0> (inited) state;
    // so, when one of the operands is the current location of an unused virtual qubit,
    // use a move with that location as 2nd operand,
    // after first having initialized the target qubit in |0> (inited) state when that has not been done already;
    // but this initialization must not extend the depth so can only be done when cycles for it are for free
    void AddSwap(utils::UInt r0, utils::UInt r1);

    // add the mapped gate (with real qubit indices as operands) to the past
    // by adding it to the waitinglist and scheduling it into the past
    void AddAndSchedule(gate_p gp);

    // find real qubit index implementing virtual qubit index;
    // if not yet mapped, allocate a new real qubit index and map to it
    utils::UInt MapQubit(utils::UInt v);

    static void stripname(utils::Str &name);

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
    void MakeReal(gate *gp, circuit &circ);

    // as mapper after-burner
    // make primitives of all gates that also have an entry with _prim appended to its name
    // and decomposing it according to the .json file gate decomposition
    void MakePrimitive(gate *gp, circuit &circ) const;

    utils::UInt MaxFreeCycle() const;

    // nonq and q gates follow separate flows through Past:
    // - q gates are put in waitinglg when added and then scheduled; and then ordered by cycle into lg
    //      in lg they are waiting to be inspected and scheduled, until [too many are there,] a nonq comes or end-of-circuit
    // - nonq gates first cause lg to be flushed/cleared to output before the nonq gate is output
    // all gates in outlg are out of view for scheduling/mapping optimization and can be taken out to elsewhere
    void FlushAll();

    // gp as nonq gate immediately goes to outlg
    void ByPass(gate *gp);

    // mainPast flushes outlg to parameter oc
    void Out(circuit &oc);

};

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
    const quantum_platform  *platformp;  // descriptions of resources for scheduling
    quantum_kernel          *kernelp;    // kernel pointer to allow calling kernel private methods
    Grid                    *gridp;      // grid pointer to know which hops are inter-core
    utils::UInt             nq;          // width of Past and Virt2Real map is number of real qubits
    utils::UInt             ct;          // cycle time, multiplier from cycles to nano-seconds

    gate                    *targetgp;   // gate that this variation aims to make NN
    utils::Vec<utils::UInt> total;       // full path, including source and target nodes
    utils::Vec<utils::UInt> fromSource;  // partial path after split, starting at source
    utils::Vec<utils::UInt> fromTarget;  // partial path after split, starting at target, backward

    Past                    past;        // cloned main past, extended with swaps from this path
    utils::Real           score;       // e.g. latency extension caused by the path
    utils::Bool             didscore;    // initially false, true after assignment to score

    // explicit Alter constructor
    // needed for virgin construction
    Alter();

    // Alter initializer
    // This should only be called after a virgin construction and not after cloning a path.
    void Init(const quantum_platform *p, quantum_kernel *k, Grid *g);

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
    void AddSwaps(Past &past, const utils::Str &mapselectswapsopt) const;

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
    void Split(const Grid &grid, utils::List<Alter> &resla) const;

};


// =========================================================================================
// Future: input window for mapper
//
// The future window shows the gates that still must be mapped as the availability list
// of a list scheduler that would work on a dependence graph representation of each input circuit.
// This future window is initialized once for the whole program, and gets a method call
// when it should switch to a new circuit (corresponding to a new kernel).
// In each circuit and thus each dependence graph the gates (including classical instruction) are found;
// the dependence graph models their dependences and also whether they act as barriers,
// an example of the latter being a classical branch.
// The availability list with gates (including classical instructions) is the main interface
// to the mapper, i.e. the mapper selects one or more element(s) from it to map next;
// it may even create alternatives for each combination of available gates.
// The gates in the list have attributes like criticality, which can be exploited by the mapper.
// The dependence graph and the availability list operations are provided by the Scheduler class.
//
// The future is a window because in principle it could be implemented incrementally,
// i.e. that the dependence graph would be extended when an attribute gets below a threshold,
// e.g. when successors of a gate are interrogated for a particular attribute.
// A problem might be that criticality requires having seen the end of the circuit,
// but the space overhead of this attribute is much less than that of a full dependence graph.
// The implementation below is not incremental: it creates the dep graph for a circuit completely.
//
// The implementation below just selects the most critical gate from the availability list
// as next candidate to map, the idea being that any collateral damage of mapping this gate
// will have a lower probability of increasing circuit depth
// than taking a non-critical gate as first one to map.
// Later implementations may become more sophisticated.
//
// With option maplookaheadopt=="no", the future window's dependence graph (scheduled and avlist) are not used.
// Instead a copy of the input circuit (input_gatepv) is created and iterated over (input_gatepp).

class Future {
public:
    const quantum_platform            *platformp;
    Scheduler                       *schedp;        // a pointer, since dependence graph doesn't change
    circuit                     input_gatepv;   // input circuit when not using scheduler based avlist

    utils::Map<gate*,utils::Bool>        scheduled;      // state: has gate been scheduled, here: done from future?
    utils::List<lemon::ListDigraph::Node> avlist;         // state: which nodes/gates are available for mapping now?
    circuit::iterator           input_gatepp;   // state: alternative iterator in input_gatepv

    // just program wide initialization
    void Init(const quantum_platform *p);

    // Set/switch input to the provided circuit
    // nq, nc and nb are parameters because nc/nb may not be provided by platform but by kernel
    // the latter should be updated when mapping multiple kernels
    void SetCircuit(quantum_kernel &kernel, Scheduler &sched, utils::UInt nq, utils::UInt nc, utils::UInt nb);

    // Get from avlist all gates that are non-quantum into nonqlg
    // Non-quantum gates include: classical, and dummy (SOURCE/SINK)
    // Return whether some non-quantum gate was found
    utils::Bool GetNonQuantumGates(utils::List<gate*> &nonqlg) const;

    // Get all gates from avlist into qlg
    // Return whether some gate was found
    utils::Bool GetGates(utils::List<gate*> &qlg) const;

    // Indicate that a gate currently in avlist has been mapped, can be taken out of the avlist
    // and its successors can be made available
    void DoneGate(gate *gp);

    // Return gp in lag that is most critical (provided lookahead is enabled)
    // This is used in tiebreak, when every other option has failed to make a distinction.
    gate *MostCriticalIn(utils::List<gate*> &lag) const;

};

// =========================================================================================
// Mapper: map operands of gates and insert swaps so that two-qubit gate operands are NN.
// All gates must be unary or two-qubit gates. The operands are virtual qubit indices.
// After mapping, all virtual qubit operands have been mapped to real qubit operands.

// For the mapper to work,
// the number of virtual qubits (nvq) must be less equal to the number of real qubits (nrq): nvq <= nrq;
// the mapper assumes that the virtual qubit operands (vqi) are encoded as a number 0 <= vqi < nvq
// and that the real qubit operands (rqi) are encoded as a number 0 <= rqi < nrq.
// The nrq is given by the platform, nvq is given by the program.
// The mapper ignores the latter (0 <= vqi < nvq was tested when creating the gates),
// and assumes vqi, nvq, rqi and nrq to be of the same type (utils::UInt) 0<=qi<nrq.
// Because of this, it makes no difference between nvq and nrq, and refers to both as nq,
// and initializes the latter from the platform.
// All maps mapping virtual and real qubits to something are of size nq.

// Classical registers are ignored by the mapper currently. TO BE DONE.

// The mapping is done in the context of a grid of qubits defined by the given platform.
// This grid is initialized once for the whole program and constant after that.

// Each kernel in the program is independently mapped (see the Map method),
// ignoring inter-kernel control flow and thereby the requirement to pass on the current mapping.
// However, for each kernel there are two methods: initial placement and a heuristic,
// of which initial placement may do a half-hearted job, while heuristic will always be successful in finding a map;
// but what initial placement may find, it will be used by the heuristic as an initial mapping; they are in this order.

// Anticipating on the inter-kernel mapping, the mapper maintains a kernel input mapping coming from the context,
// and produces a kernel output mapping for the context; the mapper updates the kernel's circuit from virtual to real.
//
// Without inter-kernel control flow, the flow is as follows:
// - mapping starts from a 1 to 1 mapping of virtual to real qubits (the kernel input mapping)
//      in which all virtual qubits are initialized to a fixed constant state (|0>/inited), suitable for replacing swap by move
// - optionally attempt an initial placement of the circuit, starting from the kernel input mapping
//      and thus optionally updating the virtual to real map and the state of used virtuals (from inited to inuse)
// - anyhow use heuristics to map the input (or what initial placement left to do),
//      mapping the virtual gates to (sets of) real gates, and outputing the new map and the new virtuals' state
// - optionally decompose swap and/or cnot gates in the real circuit to primitives (MakePrimitive)

// Inter-kernel control flow and consequent mapping dependence between kernels is not implemented. TO BE DONE
// The design of mapping multiple kernels is as follows (TO BE ADAPTED TO NEW REALSTATE):
// The mapping is done kernel by kernel, in the order that they appear in the list of kernels:
// - initially the program wide initial mapping is a 1 to 1 mapping of virtual to real qubits
// - when start to map a kernel, there is a set of already mapped kernels, and a set of not yet mapped kernels;
//       of each mapped kernel, there is an output mapping, i.e. the mapping of virts to reals with the rs per virtual;
//       when mapping was ready, and the current kernel has a set of kernels
//       which are direct predecessor in the program's control flow;
//       a subset of those direct predecessors thus has been mapped and another subset not mapped;
//       the output mappings of the mapped predecessor kernels are input
// - unify these multiple input mappings to a single one; this may introduce swaps on the control flow edges;
//      the result is the input mapping of the current kernel; keep it for later reference
// - attempt an initial placement of the circuit, starting from the kernel input mapping
// - anyhow use heuristics to map the input (or what initial placement left to do)
// - when done:
//       keep the output mapping as the kernel's output mapping;
//       for all mapped successor kernels, compute a transition from output to their input,
//       and add it to the edge; the edge code must be optimized for:
//       - being empty: nothing needs to be done
//       - having a source with one succ; the edge code can be appended to that succ
//       - having a target with one pred; the edge code can be prepended to that pred
//       - otherwise, a separate intermediate kernel for the transition code must be created, and added
// THE ABOVE INTER-KERNEL MAPPING IS NOT IMPLEMENTED.

// The Mapper's main entry is Map which manages the input and output streams of QASM instructions,
// and does the logic between (global) initial placement mapper and the (more local) heuristic mapper.
// It selects the quantum gates from it, and maps these in the context of what was mapped before (the Past).
// Each gate is separately mapped in MapGate in the main Past's context.
class Mapper {
private:
                                            // Initialized by Mapper::Init
                                            // OpenQL wide configuration, all constant after initialization
    const quantum_platform  *platformp;     // current platform: topology and gate definitions
    quantum_kernel          *kernelp;       // (copy of) current kernel (class) with free private circuit and methods
                                            // primarily to create gates in Past; Past is part of Mapper and of each Alter

    utils::UInt             nq;             // number of qubits in the platform, number of real qubits
    utils::UInt             nc;             // number of cregs in the platform, number of classical registers
    utils::UInt             nb;             // number of bregs in the platform, number of bit registers
    utils::UInt             cycle_time;     // length in ns of a single cycle of the platform
                                            // is divisor of duration in ns to convert it to cycles
    Grid                    grid;           // current grid

                                            // Initialized by Mapper.Map
    std::mt19937            gen;            // Standard mersenne_twister_engine, not yet seeded

public:
                                            // Passed back by Mapper::Map to caller for reporting
    utils::UInt             nswapsadded;    // number of swaps added (including moves)
    utils::UInt             nmovesadded;    // number of moves added
    utils::Vec<utils::UInt> v2r_in;         // v2r[virtual qubit index] -> real qubit index | UNDEFINED_QUBIT
    utils::Vec<utils::Int>  rs_in;          // rs[real qubit index] -> {nostate|wasinited|hasstate}
    utils::Vec<utils::UInt> v2r_ip;         // v2r[virtual qubit index] -> real qubit index | UNDEFINED_QUBIT
    utils::Vec<utils::Int>  rs_ip;          // rs[real qubit index] -> {nostate|wasinited|hasstate}
    utils::Vec<utils::UInt> v2r_out;        // v2r[virtual qubit index] -> real qubit index | UNDEFINED_QUBIT
    utils::Vec<utils::Int>  rs_out;         // rs[real qubit index] -> {nostate|wasinited|hasstate}


    // Mapper constructor is default synthesized

private:

    // initial path finder
    // generate paths with source src and target tgt as a list of path into resla;
    // this result list resla is allocated by caller and is empty on the call;
    // which indicates which paths are generated; see below the enum whichpaths;
    // on top of this, the other mapper options apply
    typedef enum {
        wp_all_shortest,            // all shortest paths
        wp_left_shortest,           // only the shortest along the left side of the rectangle of src and tgt
        wp_right_shortest,          // only the shortest along the right side of the rectangle of src and tgt
        wp_leftright_shortest       // both the left and right shortest
    } whichpaths_t;

    // Find shortest paths between src and tgt in the grid, bounded by a particular strategy (which);
    // budget is the maximum number of hops allowed in the path from src and is at least distance to tgt;
    // it can be higher when not all hops qualify for doing a two-qubit gate or to find more than just the shortest paths.
    void GenShortestPaths(gate *gp, utils::UInt src, utils::UInt tgt, utils::UInt budget, utils::List<Alter> &resla, whichpaths_t which);

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
    void GenShortestPaths(gate *gp, utils::UInt src, utils::UInt tgt, utils::List<Alter> &resla);

    // Generate all possible variations of making gp NN, starting from given past (with its mappings),
    // and return the found variations by appending them to the given list of Alters, la
    void GenAltersGate(gate *gp, utils::List<Alter> &la, Past &past);

    // Generate all possible variations of making gates in lg NN, starting from given past (with its mappings),
    // and return the found variations by appending them to the given list of Alters, la
    // Depending on maplookahead only take first (most critical) gate or take all gates.
    void GenAlters(utils::List<gate*> lg, utils::List<Alter> &la, Past &past);

    // start the random generator with a seed
    // that is unique to the microsecond
    void RandomInit();

    // if the maptiebreak option indicates so,
    // generate a random utils::Int number in range 0..count-1 and use
    // that to index in list of alternatives and to return that one,
    // otherwise return a fixed one (front, back or first most critical one
    Alter ChooseAlter(utils::List<Alter> &la, Future &future);

    // Map the gate/operands of a gate that has been routed or doesn't require routing
    void MapRoutedGate(gate *gp, Past &past);

    // commit Alter resa
    // generating swaps in past
    // and taking it out of future when done with it
    void CommitAlter(Alter &resa, Future &future, Past &past);

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
    utils::Bool MapMappableGates(Future &future, Past &past, utils::List<gate*> &lg, utils::Bool alsoNN2q);

    // select Alter determined by strategy defined by mapper options
    // - if base[rc], select from whole list of Alters, of which all 'remain'
    // - if minextend[rc], select Alter from list of Alters with minimal cycle extension of given past
    //   when several remain with equal minimum extension, recurse to reduce this set of remaining ones
    //   - level: level of recursion at which SelectAlter is called: 0 is base, 1 is 1st, etc.
    //   - option mapselectmaxlevel: max level of recursion to use, where inf indicates no maximum
    // - maptiebreak option indicates which one to take when several (still) remain
    // result is returned in resa
    void SelectAlter(utils::List<Alter> &la, Alter &resa, Future &future, Past &past, Past &basePast, utils::Int level);

    // Given the states of past and future
    // map all mappable gates and find the non-mappable ones
    // for those evaluate what to do next and do it;
    // during recursion, comparison is done with the base past (bottom of recursion stack),
    // and past is the last past (top of recursion stack) relative to which the mapping is done.
    void MapGates(Future &future, Past &past, Past &basePast);

    // Map the circuit's gates in the provided context (v2r maps), updating circuit and v2r maps
    void MapCircuit(quantum_kernel& kernel, Virt2Real& v2r);

public:

    // decompose all gates that have a definition with _prim appended to its name
    void MakePrimitives(quantum_kernel &kernel);

    // map kernel's circuit, main mapper entry once per kernel
    // JvS: moved to mapper.cc ahead of restructuring everything else for persistent INITIALPLACE switch
    void Map(quantum_kernel &kernel);

    // initialize mapper for whole program
    // lots could be split off for the whole program, once that is needed
    //
    // initialization for a particular kernel is separate (in Map entry)
    void Init(const quantum_platform *p);

};

} // namespace mapper
} // namespace ql
