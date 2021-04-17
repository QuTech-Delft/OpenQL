/** \file
 * OpenQL virtual to real qubit mapping and routing.
 */

#pragma once

#include <random>
#include <chrono>
#include <ctime>
#include <ratio>
#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/vec.h"
#include "ql/utils/list.h"
#include "ql/utils/map.h"
#include "ql/plat/platform.h"
#include "ql/ir/ir.h"
#include "ql/com/qubit_mapping.h"
#include "options.h"
#include "free_cycle.h"
#include "past.h"
#include "alter.h"
#include "future.h"

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace route {
namespace detail {

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
// Virt2Real: map of a virtual qubit index to its real qubit index.
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
//      The reverse map (get_virtual()) is implemented by a reverse look-up:
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
    plat::PlatformRef       platformp;      // current platform: topology and gate definitions
    ir::KernelRef           kernelp;        // (copy of) current kernel (class) with free private circuit and methods
                                            // primarily to create gates in Past; Past is part of Mapper and of each Alter
    OptionsRef              options;        // parsed mapper pass options

    utils::UInt             nq;             // number of qubits in the platform, number of real qubits
    utils::UInt             nc;             // number of cregs in the platform, number of classical registers
    utils::UInt             nb;             // number of bregs in the platform, number of bit registers
    utils::UInt             cycle_time;     // length in ns of a single cycle of the platform
                                            // is divisor of duration in ns to convert it to cycles

                                            // Initialized by Mapper.Map
    std::mt19937            gen;            // Standard mersenne_twister_engine, not yet seeded

public:
                                            // Passed back by Mapper::Map to caller for reporting
    utils::UInt             nswapsadded;    // number of swaps added (including moves)
    utils::UInt             nmovesadded;    // number of moves added
    com::QubitMapping       v2r_in;         // qubit mapping before mapping
    com::QubitMapping       v2r_ip;         // qubit mapping after initial placement
    com::QubitMapping       v2r_out;        // qubit mapping after mapping

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
    void GenShortestPaths(const ir::GateRef &gp, utils::UInt src, utils::UInt tgt, utils::UInt budget, utils::List<Alter> &resla, whichpaths_t which);

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
    void GenShortestPaths(const ir::GateRef &gp, utils::UInt src, utils::UInt tgt, utils::List<Alter> &resla);

    // Generate all possible variations of making gp NN, starting from given past (with its mappings),
    // and return the found variations by appending them to the given list of Alters, la
    void GenAltersGate(const ir::GateRef &gp, utils::List<Alter> &la, Past &past);

    // Generate all possible variations of making gates in lg NN, starting from given past (with its mappings),
    // and return the found variations by appending them to the given list of Alters, la
    // Depending on maplookahead only take first (most critical) gate or take all gates.
    void GenAlters(const utils::List<ir::GateRef> &lg, utils::List<Alter> &la, Past &past);

    // start the random generator with a seed
    // that is unique to the microsecond
    void RandomInit();

    // if the maptiebreak option indicates so,
    // generate a random utils::Int number in range 0..count-1 and use
    // that to index in list of alternatives and to return that one,
    // otherwise return a fixed one (front, back or first most critical one
    Alter ChooseAlter(utils::List<Alter> &la, Future &future);

    // Map the gate/operands of a gate that has been routed or doesn't require routing
    void MapRoutedGate(ir::GateRef &gp, Past &past);

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
    utils::Bool MapMappableGates(Future &future, Past &past, utils::List<ir::GateRef> &lg, utils::Bool alsoNN2q);

    // select Alter determined by strategy defined by mapper options
    // - if base[rc], select from whole list of Alters, of which all 'remain'
    // - if minextend[rc], select Alter from list of Alters with minimal cycle extension of given past
    //   when several remain with equal minimum extension, recurse to reduce this set of remaining ones
    //   - level: level of recursion at which SelectAlter is called: 0 is base, 1 is 1st, etc.
    //   - option mapselectmaxlevel: max level of recursion to use, where inf indicates no maximum
    // - maptiebreak option indicates which one to take when several (still) remain
    // result is returned in resa
    void SelectAlter(utils::List<Alter> &la, Alter &resa, Future &future, Past &past, Past &basePast, utils::UInt level);

    // Given the states of past and future
    // map all mappable gates and find the non-mappable ones
    // for those evaluate what to do next and do it;
    // during recursion, comparison is done with the base past (bottom of recursion stack),
    // and past is the last past (top of recursion stack) relative to which the mapping is done.
    void MapGates(Future &future, Past &past, Past &basePast);

    // Map the circuit's gates in the provided context (v2r maps), updating circuit and v2r maps
    void MapCircuit(const ir::KernelRef &kernel, com::QubitMapping &v2r);

public:

    // decompose all gates that have a definition with _prim appended to its name
    void MakePrimitives(const ir::KernelRef &kernel);

    // map kernel's circuit, main mapper entry once per kernel
    // JvS: moved to mapper.cc ahead of restructuring everything else for persistent INITIALPLACE switch
    void Map(const ir::KernelRef &kernel);

    // initialize mapper for whole program
    // lots could be split off for the whole program, once that is needed
    //
    // initialization for a particular kernel is separate (in Map entry)
    void Init(const plat::PlatformRef &p, const OptionsRef &opt);

};

} // namespace detail
} // namespace route
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
