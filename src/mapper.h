//#define INITIALPLACE 1
// commenting out the #define INITIALPLACE above, takes out all what's needed for initial placement
/**
 * @file   mapper.h
 * @date   06/2018 - now
 * @author Hans van Someren
 * @brief  openql virtual to real qubit mapping and routing
 */

#ifndef QL_MAPPER_H
#define QL_MAPPER_H

#include <random>
#include <chrono>
#include <ctime>
#include <ratio>
#include "utils.h"
#include "platform.h"
#include "kernel.h"
#include "resource_manager.h"
#include "gate.h"
#include "scheduler.h"
#include "metrics.h"

#ifdef INITIALPLACE
#include <thread>
#include <mutex>
#include <condition_variable>
#include <lemon/lp.h>
#endif // INITIALPLACE

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
// the standard assert didn't stop the compiler on its failure (causing long debug sessions)
// so added the next to get around this and enforce assert working
// it can be replaced by the standard assert if it works
void assert_fail(const char *f, int l, const char *s)
{
    FATAL("assert " << s << " failed in file " << f << " at line " << l);
}
#define MapperAssert(condition)   { if (!(condition)) { assert_fail(__FILE__, __LINE__, #condition); } }



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
// so their indices use the same data type (size_t) and the same range type 0<=index<nq.
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

typedef
enum realstate {
    rs_nostate,     // real qubit has no relevant state needing preservation, i.e. is garbage
    rs_wasinited,   // real qubit has initialized state suitable for replacing swap by move
    rs_hasstate     // real qubit has a unique state which must be preserved
} realstate_t;

#define UNDEFINED_QUBIT    MAX_CYCLE

class Virt2Real
{
private:

    size_t              nq;                 // size of the map; after initialization, will always be the same
    std::vector<size_t> v2rMap;             // v2rMap[virtual qubit index] -> real qubit index | UNDEFINED_QUBIT
    std::vector<realstate_t>rs;             // rs[real qubit index] -> {nostate|wasinited|hasstate}

public:

// map real qubit to the virtual qubit index that is mapped to it (i.e. backward map);
// when none, return UNDEFINED_QUBIT;
// a second vector next to v2rMap (i.e. an r2vMap) would speed this up;
size_t GetVirt(size_t r)
{
    MapperAssert(r != UNDEFINED_QUBIT);
    for (size_t v=0; v<nq; v++)
    {
        if (v2rMap[v] == r) return v;
    }
    return UNDEFINED_QUBIT;
}

realstate_t GetRs(size_t q)
{
    return rs[q];
}

void SetRs(size_t q, realstate_t rsvalue)
{
    rs[q] = rsvalue;
}

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
void Init(size_t n)
{
    auto mapinitone2oneopt = ql::options::get("mapinitone2one");
    auto mapassumezeroinitstate = ql::options::get("mapassumezeroinitstate");

    nq = n;
    if ("yes" == mapinitone2oneopt)
    {
        DOUT("Virt2Real::Init(n=" << nq << "), initializing 1-1 mapping");
    }
    else
    {
        DOUT("Virt2Real::Init(n=" << nq << "), initializing on demand mapping");
    }
    if ("yes" == mapassumezeroinitstate)
    {
        DOUT("Virt2Real::Init(n=" << nq << "), assume all qubits in initialized state");
    }
    else
    {
        DOUT("Virt2Real::Init(n=" << nq << "), assume all qubits in garbage state");
    }
    v2rMap.resize(nq);
    rs.resize(nq);
    for (size_t i=0; i<nq; i++)
    {
        if ("yes" == mapinitone2oneopt)
        {
            v2rMap[i] = i;
        }
        else
        {
            v2rMap[i] = UNDEFINED_QUBIT;
        }
        if ("yes" == mapassumezeroinitstate)
        {
            rs[i] = rs_wasinited;
        }
        else
        {
            rs[i] = rs_nostate;
        }
    }
}

// map virtual qubit index to real qubit index
size_t& operator[] (size_t v)
{
    MapperAssert(v < nq);   // implies v != UNDEFINED_QUBIT
    return v2rMap[v];
}

// allocate a new real qubit for an unmapped virtual qubit v (i.e. v2rMap[v] == UNDEFINED_QUBIT);
// note that this may consult the grid or future gates to find a best real
// and thus should not be in Virt2Real but higher up
size_t AllocQubit(size_t v)
{
    // check all real indices for being in v2rMap
    // first one that isn't, is free and is returned
    for (size_t r=0; r<nq; r++)
    {
        size_t vt;
        for (vt=0; vt<nq; vt++)
        {
            if (v2rMap[vt] == r)
            {
                break;
            }
        }
        if (vt >= nq)
        {
            // real qubit r was not found in v2rMap
            // use it to map v
            v2rMap[v] = r;
            MapperAssert(rs[r]==rs_wasinited||rs[r]==rs_nostate);
            DOUT("AllocQubit(v=" << v << ") in r=" << r);
            return r;
        }
    }
    MapperAssert(0);    // number of virt qubits <= number of real qubits
    return UNDEFINED_QUBIT;
}

// r0 and r1 are real qubit indices;
// by execution of a swap(r0,r1), their states are exchanged at runtime;
// so when v0 was in r0 and v1 was in r1, then v0 is now in r1 and v1 is in r0;
// update v2r accordingly
void Swap(size_t r0, size_t r1)
{
    MapperAssert(r0 != r1);
    size_t v0 = GetVirt(r0);
    size_t v1 = GetVirt(r1);
    // DOUT("... swap between ("<< v0<<"<->"<<r0<<","<<v1<<"<->"<<r1<<") and ("<<v0<<"<->"<<r1<<","<<v1<<"<->"<<r0<<" )");
    // DPRINT("... before swap");
    MapperAssert(v0 != v1);         // also holds when vi == UNDEFINED_QUBIT

    if (v0 == UNDEFINED_QUBIT)
    {
        MapperAssert(rs[r0] != rs_hasstate);
    }
    else
    {
        MapperAssert(v0 < nq);
        v2rMap[v0] = r1;
    }

    if (v1 == UNDEFINED_QUBIT)
    {
        MapperAssert(rs[r1] != rs_hasstate);
    }
    else
    {
        MapperAssert(v1 < nq);
        v2rMap[v1] = r0;
    }

    realstate_t ts = rs[r0];
    rs[r0] = rs[r1];
    rs[r1] = ts;
    // DPRINT("... after swap");
}

void DPRINTReal(size_t r)
{
    if ( ql::utils::logger::LOG_LEVEL >= ql::utils::logger::log_level_t::LOG_DEBUG )
        PrintReal(r);
}
void PrintReal(size_t r)
{
    std::cout << " (r" << r;
    switch(rs[r])
    {
    case rs_nostate:
        std::cout << ":no";
        break;
    case rs_wasinited:
        std::cout << ":in";
        break;
    case rs_hasstate:
        std::cout << ":st";
        break;
    default:
        MapperAssert(0);
    }
    size_t v = GetVirt(r);
    if (v == UNDEFINED_QUBIT)
    {
        std::cout << "<-UN)";
    }
    else
    {
        std::cout << "<-v" << v << ")";
    }
}

void PrintVirt(size_t v)
{
    std::cout << " (v" << v;
    size_t r = v2rMap[v];
    if (r == UNDEFINED_QUBIT)
    {
        std::cout << "->UN)";
    }
    else
    {
        std::cout << "->r" << r;
        switch(rs[r])
        {
        case rs_nostate:
            std::cout << ":no)";
            break;
        case rs_wasinited:
            std::cout << ":in)";
            break;
        case rs_hasstate:
            std::cout << ":st)";
            break;
        default:
            MapperAssert(0);
        }
    }
}

void DPRINTReal(std::string s, size_t r0, size_t r1)
{
    if ( ql::utils::logger::LOG_LEVEL >= ql::utils::logger::log_level_t::LOG_DEBUG )
        PrintReal(s, r0, r1);
}
void PrintReal(std::string s, size_t r0, size_t r1)
{
    // DOUT("v2r.PrintReal ...");
    std::cout << s << ":";
//  std::cout << "... real2Virt(r<-v) " << s << ":";

    PrintReal(r0);
    PrintReal(r1);
    std::cout << std::endl;
}

void DPRINT(std::string s)
{
    if ( ql::utils::logger::LOG_LEVEL >= ql::utils::logger::log_level_t::LOG_DEBUG )
        Print(s);
}
void Print(std::string s)
{
    // DOUT("v2r.Print ...");
    std::cout << s << ":";
//  std::cout << "... virt2Real(r<-v) " << s << ":";
    for (size_t v=0; v<nq; v++)
    {
        PrintVirt(v);
    }
    std::cout << std::endl;

#ifdef needed
    std::cout << "... real2virt(r->v) " << s << ":";
    for (size_t r=0; r<nq; r++)
    {
        PrintReal(r);
    }
    std::cout << std::endl;
#endif
}

void Export(std::vector<size_t>& kv2rMap)
{
    kv2rMap = v2rMap;
}

void Export(std::vector<int>& krs)
{
    krs.resize(rs.size());
    for (unsigned int i=0; i < rs.size(); i++)
    {
        krs[i] = (int)rs[i];
    }
}

};  // end class Virt2Real





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
class FreeCycle
{
private:

    const ql::quantum_platform   *platformp;// platform description
    size_t                  nq;      // size of the map; after initialization, will always be the same
    size_t                  ct;      // multiplication factor from cycles to nano-seconds (unit of duration)
    std::vector<size_t>     fcv;     // fcv[real qubit index i]: qubit i is free from this cycle on
    ql::arch::resource_manager_t rm;  // actual resources occupied by scheduled gates


// access free cycle value of qubit i
size_t& operator[] (size_t i)
{
    return fcv[i];
}

public:

// explicit FreeCycle constructor
// needed for virgin construction
// default constructor was deleted because it cannot construct resource_manager_t without parameters
FreeCycle()
{
    DOUT("Constructing FreeCycle");
}

void Init(const ql::quantum_platform *p)
{
    DOUT("FreeCycle::Init()");
    ql::arch::resource_manager_t lrm(*p, ql::forward_scheduling);   // allocated here and copied below to rm because of platform parameter
    DOUT("... created FreeCycle Init local resource_manager");
    platformp = p;
    nq = platformp->qubit_number;
    ct = platformp->cycle_time;
    DOUT("... FreeCycle: nq=" << nq << ", ct=" << ct << "), initializing to all 0 cycles");
    fcv.clear();
    fcv.resize(nq, 1);   // this 1 implies that cycle of first gate will be 1 and not 0; OpenQL convention!?!?
    DOUT("... about to copy FreeCycle Init local resource_manager to FreeCycle member rm");
    rm = lrm;
    DOUT("... done copy FreeCycle Init local resource_manager to FreeCycle member rm");
}

// depth of the FreeCycle map
// equals the max of all entries minus the min of all entries
// not used yet; would be used to compute the max size of a top window on the past
size_t Depth()
{
    return Max() - Min();
}

// min of the FreeCycle map equals the min of all entries;
size_t Min()
{
    size_t  minFreeCycle = MAX_CYCLE;
    for (auto& v : fcv)
    {
        if (v < minFreeCycle)
        {
            minFreeCycle = v;
        }
    }
    return minFreeCycle;
}

// max of the FreeCycle map equals the max of all entries;
size_t Max()
{
    size_t  maxFreeCycle = 0;
    for (auto& v : fcv)
    {
        if (maxFreeCycle < v)
        {
            maxFreeCycle = v;
        }
    }
    return maxFreeCycle;
}

void DPRINT(std::string s)
{
    if ( ql::utils::logger::LOG_LEVEL >= ql::utils::logger::log_level_t::LOG_DEBUG )
        Print(s);
}
void Print(std::string s)
{
    size_t  minFreeCycle = Min();
    size_t  maxFreeCycle = Max();
    std::cout << "... FreeCycle" << s << ":";
    for (size_t i=0; i<nq; i++)
    {
        size_t v = fcv[i];
        std::cout << " [" << i << "]=";
        if (v == minFreeCycle)
        {
            std::cout << "_";
        }
        if (v == maxFreeCycle)
        {
            std::cout << "^";
        }
        std::cout << v;
    }
    std::cout << std::endl;
    // rm.Print("... in FreeCycle: ");
}

// return whether gate with first operand qubit r0 can be scheduled earlier than with operand qubit r1
bool IsFirstOperandEarlier(size_t r0, size_t r1)
{
    DOUT("... fcv[" << r0 << "]=" << fcv[r0]<< " fcv[" << r1 << "]=" << fcv[r1] << " IsFirstOperandEarlier=" << (fcv[r0] < fcv[r1]));
    return fcv[r0] < fcv[r1];
}

// will a swap(fr0,fr1) start earlier than a swap(sr0,sr1)?
// is really a short-cut ignoring config file and perhaps several other details
bool IsFirstSwapEarliest(size_t fr0, size_t fr1, size_t sr0, size_t sr1)
{
    std::string mapreverseswapopt = ql::options::get("mapreverseswap");
    if ("yes" == mapreverseswapopt)
    {
        if (fcv[fr0] < fcv[fr1])
        {
            size_t  tmp = fr1; fr1 = fr0; fr0 = tmp;
        }
        if (fcv[sr0] < fcv[sr1])
        {
            size_t  tmp = sr1; sr1 = sr0; sr0 = tmp;
        }
    }
    size_t  startCycleFirstSwap = std::max<size_t>(fcv[fr0]-1, fcv[fr1]);
    size_t  startCycleSecondSwap = std::max<size_t>(fcv[sr0]-1, fcv[sr1]);

    DOUT("... fcv[" << fr0 << "]=" << fcv[fr0] << " fcv[" << fr1 << "]=" << fcv[fr1] << " start=" << startCycleFirstSwap << " fcv[" << sr0 << "]=" << fcv[sr0] << " fcv[" << sr1 << "]=" << fcv[sr1] << " start=" << startCycleSecondSwap << " IsFirstSwapEarliest=" << (startCycleFirstSwap < startCycleSecondSwap));
    return startCycleFirstSwap < startCycleSecondSwap;
}

// when we would schedule gate g, what would be its start cycle? return it
// gate operands are real qubit indices
// is purely functional, doesn't affect state
size_t StartCycleNoRc(ql::gate *g)
{
    auto&       q = g->operands;
    size_t      operandCount = q.size();

    size_t      startCycle;
    if (operandCount == 1)
    {
        startCycle = fcv[q[0]];
    }
    else // if (operandCount == 2)
    {
        startCycle = std::max<size_t>(fcv[q[0]], fcv[q[1]]);
    }
    MapperAssert (startCycle < MAX_CYCLE);

    return startCycle;
}

// when we would schedule gate g, what would be its start cycle? return it
// gate operands are real qubit indices
// is purely functional, doesn't affect state
size_t StartCycle(ql::gate *g)
{
    size_t      startCycle = StartCycleNoRc(g);
    
    auto        mapopt = ql::options::get("mapper");
    if (mapopt == "baserc" || mapopt == "minextendrc")
    {
        size_t      baseStartCycle = startCycle;

        while (startCycle < MAX_CYCLE)
        {
            // DOUT("Startcycle for " << g->qasm() << ": available? at startCycle=" << startCycle);
            if (rm.available(startCycle, g, *platformp))
            {
                // DOUT(" ... [" << startCycle << "] resources available for " << g->qasm());
                break;
            }   
            else
            {
                // DOUT(" ... [" << startCycle << "] Busy resource for " << g->qasm());
                startCycle++;
            }
        }
        if (baseStartCycle != startCycle)
        {
            // DOUT(" ... from [" << baseStartCycle << "] to [" << startCycle-1 << "] busy resource(s) for " << g->qasm());
        }
    }
    MapperAssert (startCycle < MAX_CYCLE);

    return startCycle;
}

// schedule gate g in the FreeCycle map
// gate operands are real qubit indices
// the FreeCycle map is updated, not the resource map
// this is done, because AddNoRc is used to represent just gate dependences, avoiding a build of a dep graph
void AddNoRc(ql::gate *g, size_t startCycle)
{
    auto&       q = g->operands;
    size_t      operandCount = q.size();
    size_t      duration = (g->duration+ct-1)/ct;   // rounded-up unsigned integer division

    if (operandCount == 1)
    {
        fcv[q[0]] = startCycle + duration;
    }
    else // if (operandCount == 2)
    {
        fcv[q[0]] = startCycle + duration;
        fcv[q[1]] = fcv[q[0]];
    }
}

// schedule gate g in the FreeCycle and resource maps
// gate operands are real qubit indices
// both the FreeCycle map and the resource map are updated
// startcycle must be the result of an earlier StartCycle call (with rc!)
void Add(ql::gate *g, size_t startCycle)
{
    AddNoRc(g, startCycle);

    auto        mapopt = ql::options::get("mapper");
    if (mapopt == "baserc" || mapopt == "minextendrc")
    {
        rm.reserve(startCycle, g, *platformp);
    }
}

};  // end class FreeCycle


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
class Past
{
private:

    size_t                  nq;         // width of Past, Virt2Real, UseCount maps in number of real qubits
    size_t                  ct;         // cycle time, multiplier from cycles to nano-seconds
    const ql::quantum_platform    *platformp; // platform describing resources for scheduling
    ql::quantum_kernel      *kernelp;   // current kernel for creating gates

    Virt2Real               v2r;        // state: current Virt2Real map, imported/exported to kernel
    FreeCycle               fc;         // state: FreeCycle map (including resource_manager) of this Past
    typedef ql::gate *      gate_p;
    std::list<gate_p>       waitinglg;  // . . .  list of q gates in this Past, topological order, waiting to be scheduled in
                                        //        waitinglg only contains gates from Add and final Schedule call
                                        //        when evaluating alternatives, it is empty when Past is cloned; so no state
public:
    std::list<gate_p>       lg;         // state: list of q gates in this Past, scheduled by their (start) cycle values
                                        //        so this is the result list of this Past, to compare with other Alters
private:
    std::list<gate_p>       outlg;      // . . .  list of gates flushed out of this Past, not yet put in outCirc
                                        //        when evaluating alternatives, outlg stays constant; so no state
    std::map<gate_p,size_t> cycle;      // state: gate to cycle map, startCycle value of each past gatecycle[gp]
                                        //        cycle[gp] can be different for each gp for each past
                                        //        gp->cycle is not used by MapGates
                                        //        although updated by set_cycle called from MakeAvailable/TakeAvailable
    size_t                  nswapsadded;// number of swaps (including moves) added to this past
    size_t                  nmovesadded;// number of moves added to this past

public:

// explicit Past constructor
// needed for virgin construction
Past()
{
    DOUT("Constructing Past");
}

// past initializer
void Init(const ql::quantum_platform *p, ql::quantum_kernel *k)
{
    DOUT("Past::Init");
    platformp = p;
    kernelp = k;

    nq = platformp->qubit_number;
    ct = platformp->cycle_time;

    MapperAssert(kernelp->c.empty());   // kernelp->c will be used by new_gate to return newly created gates into
    v2r.Init(nq);               // v2r initializtion until v2r is imported from context
    fc.Init(platformp);         // fc starts off with all qubits free, is updated after schedule of each gate
    waitinglg.clear();          // no gates pending to be scheduled in; Add of gate to past entered here
    lg.clear();                 // no gates scheduled yet in this past; after schedule of gate, it gets here
    outlg.clear();              // no gates output yet by flushing from or bypassing this past
    nswapsadded = 0;            // no swaps or moves added yet to this past; AddSwap adds one here
    nmovesadded = 0;            // no moves added yet to this past; AddSwap may add one here
    cycle.clear();              // no gates have cycles assigned in this past; scheduling gate updates this
}

// import Past's v2r from v2r_value
void ImportV2r(Virt2Real& v2r_value)
{
    v2r = v2r_value;
}

// export Past's v2r into v2r_destination
void ExportV2r(Virt2Real& v2r_destination)
{
    v2r_destination = v2r;
}

void DFcPrint()
{
    if ( ql::utils::logger::LOG_LEVEL >= ql::utils::logger::log_level_t::LOG_DEBUG )
        fc.Print("");
}
void FcPrint()
{
    fc.Print("");
}

void Print(std::string s)
{
    std::cout << "... Past " << s << ":";
    v2r.Print("");
    fc.Print("");
    // DOUT("... list of gates in past");
    for ( auto & gp: lg)
    {
        DOUT("[" << cycle[gp] << "] " << gp->qasm());
    }
}

// all gates in past.waitinglg are scheduled here into past.lg
// note that these gates all are mapped and so have real operand qubit indices
// the FreeCycle map reflects for each qubit the first free cycle
// all new gates, now in waitinglist, get such a cycle assigned below, increased gradually, until definitive
void Schedule()
        // the copy includes the resource manager.
{
    // DOUT("Schedule ...");

    while (!waitinglg.empty())
    {
        size_t      startCycle = MAX_CYCLE;
        gate_p      gp;

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
        for (auto & trygp : waitinglg)
        {
            size_t tryStartCycle = tryfc.StartCycle(trygp);
            tryfc.Add(trygp, tryStartCycle);

            if (tryStartCycle < startCycle)
            {
                startCycle = tryStartCycle;
                gp = trygp;
            }
        }

        // add this gate to the maps, scheduling the gate (doing the cycle assignment)
        // DOUT("... add " << gp->qasm() << " startcycle=" << startCycle << " cycles=" << ((gp->duration+ct-1)/ct) );
        fc.Add(gp, startCycle);
        cycle[gp] = startCycle; // cycle[gp] is private to this past but gp->cycle is private to gp
        gp->cycle = startCycle; // so gp->cycle gets assigned for each alter' Past and finally definitively for mainPast
        // DOUT("... set " << gp->qasm() << " at cycle " << startCycle);
    
        // insert gate gp in lg, the list of gates, in cycle[gp] order, and inside this order, as late as possible
        //
        // reverse iterate because the insertion is near the end of the list
        // insert so that cycle values are in order afterwards and the new one is nearest to the end
        std::list<gate_p>::reverse_iterator rigp = lg.rbegin();
        for (; rigp != lg.rend(); rigp++)
        {
            if (cycle[*rigp] <= startCycle)
            {
                // rigp.base() because insert doesn't work with reverse iteration
                // rigp.base points after the element that rigp is pointing at
                // which is lucky because insert only inserts before the given element
                // the end effect is inserting after rigp
                lg.insert(rigp.base(), gp);
                break;
            }
        }
        // when list was empty or no element was found, just put it in front
        if (rigp == lg.rend())
        {
            lg.push_front(gp);
        }
    
        // having added it to the main list, remove it from the waiting list
        waitinglg.remove(gp);
    }

    // DPRINT("Schedule:");
}

// compute costs in cycle extension of optionally scheduling initcirc before the inevitable circ
int InsertionCost(ql::circuit& initcirc, ql::circuit& circ)
{
     // first fake-schedule initcirc followed by circ in a private freecyclemap
     size_t initmax;
     FreeCycle   tryfcinit = fc;
     for (auto & trygp : initcirc)
     {
         size_t tryStartCycle = tryfcinit.StartCycleNoRc(trygp);
         tryfcinit.AddNoRc(trygp, tryStartCycle);
     }
     for (auto & trygp : circ)
     {
         size_t tryStartCycle = tryfcinit.StartCycleNoRc(trygp);
         tryfcinit.AddNoRc(trygp, tryStartCycle);
     }
     initmax = tryfcinit.Max(); // this reflects the depth afterwards

     // then fake-schedule circ alone in a private freecyclemap
     size_t max;
     FreeCycle   tryfc = fc;
     for (auto & trygp : circ)
     {
         size_t tryStartCycle = tryfc.StartCycleNoRc(trygp);
         tryfc.AddNoRc(trygp, tryStartCycle);
     }
     max = tryfc.Max();         // this reflects the depth afterwards

     DOUT("... scheduling init+circ => depth " << initmax << ", scheduling circ => depth " << max << ", init insertion cost " << (initmax-max));
     MapperAssert(initmax >= max);
     // scheduling initcirc would be for free when initmax == max, so the cost is (initmax - max)
     return (initmax - max);
}

// add the mapped gate to the current past
// means adding it to the current past's waiting list, waiting for it to be scheduled later
void Add(gate_p gp)
{
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
bool new_gate(ql::circuit& circ, std::string gname, std::vector<size_t> qubits)
{
    bool    added;
    MapperAssert(circ.empty());
    MapperAssert(kernelp->c.empty());
    added = kernelp->gate_nonfatal(gname, qubits);   // creates gates in kernelp->c
    circ = kernelp->c;
    kernelp->c.clear();
    return added;
}

// return number of swaps added to this past
size_t NumberOfSwapsAdded()
{
    return nswapsadded;
}

// return number of moves added to this past
size_t NumberOfMovesAdded()
{
    return nmovesadded;
}

void new_gate_exception(std::string s)
{
    FATAL("gate is not supported by the target platform: '" << s << "'");
}

// will a swap(fr0,fr1) start earlier than a swap(sr0,sr1)?
// is really a short-cut ignoring config file and perhaps several other details
bool IsFirstSwapEarliest(size_t fr0, size_t fr1, size_t sr0, size_t sr1)
{
    return fc.IsFirstSwapEarliest(fr0, fr1, sr0, sr1);
}

// generate a move into circ with parameters r0 and r1 (which GenMove may reverse)
// whether this was successfully done can be seen from whether circ was extended
// please note that the reversal of operands may have been done also when GenMove was not successful
void GenMove(ql::circuit& circ, size_t& r0, size_t& r1)
{
    if (v2r.GetRs(r0)!=rs_hasstate)
    {
        MapperAssert(v2r.GetRs(r0)==rs_nostate||v2r.GetRs(r0)==rs_wasinited);
        // interchange r0 and r1, so that r1 (right-hand operand of move) will be the state-less one
        size_t  tmp = r1; r1 = r0; r0 = tmp;
        // DOUT("... reversed operands for move to become move(q" << r0 << ",q" << r1 << ") ...");
    }
    MapperAssert (v2r.GetRs(r0)==rs_hasstate);    // and r0 will be the one with state
    MapperAssert (v2r.GetRs(r1)!=rs_hasstate);    // and r1 will be the one without state (rs_nostate || rs_wasinited)

    // first (optimistically) create the move circuit and add it to circ
    bool created;
    auto mapperopt = ql::options::get("mapper");
    if ("maxfidelity" == mapperopt)
    {
        created = new_gate(circ, "move_prim", {r0,r1});    // gates implementing move returned in circ
    }
    else
    {
        created = new_gate(circ, "move_real", {r0,r1});    // gates implementing move returned in circ
    }
    if (!created)
    {
        created = new_gate(circ, "move", {r0,r1});
        if (!created) new_gate_exception("move or move_real");
    }

    if (v2r.GetRs(r1) == rs_nostate)
    {
        // r1 is not in inited state, generate in initcirc the circuit to do so
        // DOUT("... initializing non-inited " << r1 << " to |0> (inited) state preferably using move_init ...");
        ql::circuit initcirc;

        created = new_gate(initcirc, "move_init", {r1});
        if (!created)
        {
            created = new_gate(initcirc, "prepz", {r1});
            // if (created)
            // {
            //     created = new_gate(initcirc, "h", {r1});
            //     if (!created) new_gate_exception("h");
            // }
            if (!created) new_gate_exception("move_init or prepz");
        }

        // when difference in extending circuit after scheduling initcirc+circ or just circ
        // is less equal than threshold cycles (0 would mean scheduling initcirc was for free),
        // commit to it, otherwise abort
        int threshold;
        std::string mapusemovesopt = ql::options::get("mapusemoves");
        if ("yes" == mapusemovesopt)
        {
            threshold = 0;
        }
        else
        {
            threshold = atoi(mapusemovesopt.c_str());
        }
        if (InsertionCost(initcirc, circ) <= threshold)
        {
            // so we go for it!
            // circ contains move; it must get the initcirc before it ...
            // do this by appending circ's gates to initcirc, and then swapping circ and initcirc content
            DOUT("... initialization is for free, do it ...");
            for (auto& gp : circ)
            {
                initcirc.push_back(gp);
            }
            circ.swap(initcirc);
            v2r.SetRs(r1, rs_wasinited);
        }
        else
        {
            // undo damage done, will not do move but swap, i.e. nothing created thisfar
            DOUT("... initialization extends circuit, don't do it ...");
            circ.clear();       // circ being cleared also indicates creation wasn't successful
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
void AddSwap(size_t r0, size_t r1)
{
    bool created = false;

    DOUT("... extending with swap(q" << r0 << ",q" << r1 << ") ...");
    v2r.DPRINTReal("... adding swap/move", r0, r1);

    MapperAssert(v2r.GetRs(r0)==rs_wasinited||v2r.GetRs(r0)==rs_nostate||v2r.GetRs(r0)==rs_hasstate);
    MapperAssert(v2r.GetRs(r1)==rs_wasinited||v2r.GetRs(r1)==rs_nostate||v2r.GetRs(r1)==rs_hasstate);

    if (v2r.GetRs(r0)!=rs_hasstate && v2r.GetRs(r1)!=rs_hasstate)
    {
        DOUT("... no state in both operand of intended swap/move; don't add swap/move gates");
        v2r.Swap(r0,r1);
        return;
    }

    ql::circuit circ;   // current kernel copy, clear circuit
    std::string mapusemovesopt = ql::options::get("mapusemoves");
    if ("no" != mapusemovesopt && (v2r.GetRs(r0)!=rs_hasstate || v2r.GetRs(r1)!=rs_hasstate))
    {
        GenMove(circ, r0, r1);
        created = circ.size()!=0;
        if (created)
        {
            // generated move
            // move is in circ, optionally with initialization in front of it
            // also rs of its 2nd operand is 'rs_wasinited'
            // note that after swap/move, r0 will be in this state then
            nmovesadded++;                       // for reporting at the end
            DOUT("... move(q" << r0 << ",q" << r1 << ") ...");
        }
        else
        {
            DOUT("... move(q" << r0 << ",q" << r1 << ") cancelled, go for swap");
        }
    }
    if (!created)
    {
        // no move generated so do swap
        std::string mapreverseswapopt = ql::options::get("mapreverseswap");
        if ("yes" == mapreverseswapopt)
        {
            // swap(r0,r1) is about to be generated
            // it is functionally symmetrical,
            // but in the implementation r1 starts 1 cycle earlier than r0 (we should derive this from json file ...)
            // so swap(r0,r1) with interchanged operands might get scheduled 1 cycle earlier;
            // when fcv[r0] < fcv[r1], r0 is free for use 1 cycle earlier than r1, so a reversal will help
            if (fc.IsFirstOperandEarlier(r0, r1))
            {
                size_t  tmp = r1; r1 = r0; r0 = tmp;
                DOUT("... reversed swap to become swap(q" << r0 << ",q" << r1 << ") ...");
            }
        }
        auto mapperopt = ql::options::get("mapper");
        if ("maxfidelity" == mapperopt)
        {
            created = new_gate(circ, "swap_prim", {r0,r1});    // gates implementing swap returned in circ
        }
        else
        {
            created = new_gate(circ, "swap_real", {r0,r1});    // gates implementing swap returned in circ
        }
        if (!created)
        {
            created = new_gate(circ, "swap", {r0,r1});
            if (!created) new_gate_exception("swap or swap_real");
        }
        DOUT("... swap(q" << r0 << ",q" << r1 << ") ...");
    }
    nswapsadded++;                       // for reporting at the end
    for (auto &gp : circ)
    {
        Add(gp);
    }
    v2r.Swap(r0,r1);        // reflect in v2r that r0 and r1 interchanged state, i.e. update the map to reflect the swap
}

// add the mapped gate (with real qubit indices as operands) to the past
// by adding it to the waitinglist and scheduling it into the past
void AddAndSchedule(gate_p gp)
{
    Add(gp);
    Schedule();
}

// find real qubit index implementing virtual qubit index;
// if not yet mapped, allocate a new real qubit index and map to it
size_t MapQubit(size_t v)
{
    size_t  r = v2r[v];
    if (r == UNDEFINED_QUBIT)
    {
        r = v2r.AllocQubit(v);
    }
    return r;
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

void stripname(std::string& name)
{
    // DOUT("stripname(name=" << name << ")");
    size_t p = name.find(" ");
    if (p != std::string::npos)
    {
        name = name.substr(0,p);
    }
    // DOUT("... after stripname name=" << name);
}

void MakeReal(ql::gate* gp, ql::circuit& circ)
{
    std::string gname = gp->name;
    stripname(gname);

    std::vector<size_t> real_qubits  = gp->operands;// starts off as copy of virtual qubits!
    for (auto& qi : real_qubits)
    {
        qi = MapQubit(qi);          // and now they are real
        auto mapprepinitsstateopt = ql::options::get("mapprepinitsstate");
        if (mapprepinitsstateopt == "yes" && (gname == "prepz" || gname == "Prepz"))
        {
            v2r.SetRs(qi, rs_wasinited);
        }
        else
        {
            v2r.SetRs(qi, rs_hasstate);
        }
    }

    auto mapperopt = ql::options::get("mapper");
    std::string real_gname = gname;
    if ("maxfidelity" == mapperopt)
    {
        DOUT("MakeReal: with mapper==maxfidelity generate _prim");
        real_gname.append("_prim");
    }
    else
    {
        real_gname.append("_real");
    }
    bool created = new_gate(circ, real_gname, real_qubits);
    if (!created)
    {
        created = new_gate(circ, gname, real_qubits);
        if (!created)
        {
            FATAL("MakeReal: failed creating gate " << real_gname << " or " << gname);
        }
    }
    DOUT("... MakeReal: new gate created for: " << real_gname << " or " << gname);
}

// as mapper after-burner
// make primitives of all gates that also have an entry with _prim appended to its name
// and decomposing it according to the .json file gate decomposition
void MakePrimitive(ql::gate* gp, ql::circuit& circ)
{
    std::string gname = gp->name;
    stripname(gname);
    std::string prim_gname = gname;
    prim_gname.append("_prim");
    bool created = new_gate(circ, prim_gname, gp->operands);
    if (!created)
    {
        created = new_gate(circ, gname, gp->operands);
        if (!created)
        {
            FATAL("MakePrimtive: failed creating gate " << prim_gname << " or " << gname);
        }
    }
    DOUT("... MakePrimtive: new gate created for: " << prim_gname << " or " << gname);
}

size_t MaxFreeCycle()
{
    return fc.Max();
}

// nonq and q gates follow separate flows through Past:
// - q gates are put in waitinglg when added and then scheduled; and then ordered by cycle into lg
//      in lg they are waiting to be inspected and scheduled, until [too many are there,] a nonq comes or end-of-circuit
// - nonq gates first cause lg to be flushed/cleared to output before the nonq gate is output
// all gates in outlg are out of view for scheduling/mapping optimization and can be taken out to elsewhere
void FlushAll()
{
    for( auto & gp : lg )
    {
        outlg.push_back(gp);
    }
    lg.clear();         // so effectively, lg's content was moved to outlg

    // fc.Init(platformp); // needed?
    // cycle.clear();      // needed?
                        // cycle is initialized to empty map
                        // is ok without windowing, but with window, just delete the ones outside the window
}

// gp as nonq gate immediately goes to outlg
void ByPass(ql::gate* gp)
{
    if (!lg.empty())
    {
        FlushAll();
    }
    outlg.push_back(gp);
}

// mainPast flushes outlg to parameter oc
void Out(ql::circuit& oc)
{
    for( auto & gp : outlg )
    {
        oc.push_back(gp);
    }
    outlg.clear();
}

};  // end class Past


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
class Alter
{
public:
    const ql::quantum_platform   *platformp;  // descriptions of resources for scheduling
    ql::quantum_kernel     *kernelp;    // kernel class pointer to allow calling kernel private methods
    size_t                  nq;         // width of Past and Virt2Real map is number of real qubits
    size_t                  ct;         // cycle time, multiplier from cycles to nano-seconds

    ql::gate               *targetgp;   // gate that this variation aims to make NN
    std::vector<size_t>     total;      // full path, including source and target nodes
    std::vector<size_t>     fromSource; // partial path after split, starting at source
    std::vector<size_t>     fromTarget; // partial path after split, starting at target, backward

    Past                    past;       // cloned main past, extended with swaps from this path
    double                  score;      // e.g. latency extension caused by the path
    bool                    didscore;   // initially false, true after assignment to score

// explicit Alter constructor
// needed for virgin construction
Alter()
{
    DOUT("Constructing Alter");
}

// Alter initializer
// This should only be called after a virgin construction and not after cloning a path.
void Init(const ql::quantum_platform* p, ql::quantum_kernel* k)
{
    DOUT("Alter::Init(number of qubits=" << nq);
    platformp = p;
    kernelp = k;

    nq = platformp->qubit_number;
    ct = platformp->cycle_time;
    // total, fromSource and fromTarget start as empty vectors
    past.Init(platformp, kernelp);      // initializes past to empty
    didscore = false;                   // will not print score for now
}

// printing facilities of Paths
// print path as hd followed by [0->1->2]
// and then followed by "implying" swap(q0,q1) swap(q1,q2)
void partialPrint(std::string hd, std::vector<size_t> & pp)
{
    if (!pp.empty())
    {
        int started = 0;
        for (auto & ppe : pp)
        {
            if (started == 0)
            {
                started = 1;
                std::cout << hd << "[";
            }
            else
            {
                std::cout << "->";
            }
            std::cout << ppe;
        }
        if (started == 1)
        {
            std::cout << "]";
//          if (pp.size() >= 2)
//          {
//              std::cout << " implying:";
//              for (size_t i = 0; i < pp.size()-1; i++)
//              {
//                  std::cout << " swap(q" << pp[i] << ",q" << pp[i+1] << ")";
//              }
//          }
        }
    }
}

void DPRINT(std::string s)
{
    if ( ql::utils::logger::LOG_LEVEL >= ql::utils::logger::log_level_t::LOG_DEBUG )
        Print(s);
}
void Print(std::string s)
{
    // std::cout << s << "- " << targetgp->qasm();
    std::cout << s << "- " << targetgp->qasm();
    if (fromSource.empty() && fromTarget.empty())
    {
        partialPrint(", total path:", total);
    }
    else
    {
        partialPrint(", path from source:", fromSource);
        partialPrint(", from target:", fromTarget);
    }
    if (didscore)
    {
        std::cout << ", score=" << score;
    }
    // past.Print("past in Alter");
    std::cout << std::endl;
}

static
void DPRINT(std::string s, std::vector<Alter> & va)
{
    if ( ql::utils::logger::LOG_LEVEL >= ql::utils::logger::log_level_t::LOG_DEBUG )
        Print(s, va);
}
static
void Print(std::string s, std::vector<Alter> & va)
{
    int started = 0;
    for (auto & a : va)
    {
        if (started == 0)
        {
            started = 1;
            std::cout << s << "[" << va.size() << "]={" << std::endl;
        }
        a.Print("");
    }
    if (started == 1)
    {
        std::cout << "}" << std::endl;
    }
}

static
void DPRINT(std::string s, std::list<Alter> & la)
{
    if ( ql::utils::logger::LOG_LEVEL >= ql::utils::logger::log_level_t::LOG_DEBUG )
        Print(s, la);
}
static
void Print(std::string s, std::list<Alter> & la)
{
    int started = 0;
    for (auto & a : la)
    {
        if (started == 0)
        {
            started = 1;
            std::cout << s << "[" << la.size() << "]={" << std::endl;
        }
        a.Print("");
    }
    if (started == 1)
    {
        std::cout << "}" << std::endl;
    }
}

// add a node to the path in front, extending its length with one
void Add2Front(size_t q)
{
    total.insert(total.begin(), q); // hopelessly inefficient
}

// add to a max of maxnumbertoadd swap gates for the current path to the given past
// this past can be a path-local one or the main past
// after having added them, schedule the result into that past
void AddSwaps(Past & past, std::string mapselectswapsopt)
{
    // DOUT("Addswaps " << mapselectswapsopt);
    if ("one"==mapselectswapsopt || "all"==mapselectswapsopt)
    {
        size_t  numberadded = 0;
        size_t  maxnumbertoadd = ("one"==mapselectswapsopt ? 1 : MAX_CYCLE);

        size_t  fromSourceQ;
        size_t  toSourceQ;
        fromSourceQ = fromSource[0];
        for ( size_t i = 1; i < fromSource.size() && numberadded < maxnumbertoadd; i++ )
        {
            toSourceQ = fromSource[i];
            past.AddSwap(fromSourceQ, toSourceQ);
            fromSourceQ = toSourceQ;
            numberadded++;
        }
    
        size_t  fromTargetQ;
        size_t  toTargetQ;
        fromTargetQ = fromTarget[0];
        for ( size_t i = 1; i < fromTarget.size() && numberadded < maxnumbertoadd; i++ )
        {
            toTargetQ = fromTarget[i];
            past.AddSwap(fromTargetQ, toTargetQ);
            fromTargetQ = toTargetQ;
            numberadded++;
        }
    }
    else
    {
        MapperAssert("earliest"==mapselectswapsopt);
        if (fromSource.size() >= 2 && fromTarget.size() >= 2)
        {
            if (past.IsFirstSwapEarliest(fromSource[0], fromSource[1], fromTarget[0], fromTarget[1]))
            {
                past.AddSwap(fromSource[0], fromSource[1]);
            }
            else
            {
                past.AddSwap(fromTarget[0], fromTarget[1]);
            }
        }
        else if (fromSource.size() >= 2)
        {
            past.AddSwap(fromSource[0], fromSource[1]);
        }
        else if (fromTarget.size() >= 2)
        {
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
void Extend(Past currPast, Past basePast)
{
    // DOUT("... clone past, add swaps, compute overall score and keep it all in current alternative");
    past = currPast;   // explicitly clone currPast to an alternative-local copy of it, Alter.past
    // DOUT("... adding swaps to alternative-local past ...");
    AddSwaps(past, "all");
    // DOUT("... done adding/scheduling swaps to alternative-local past");

    auto mapperopt = ql::options::get("mapper");
    if ("maxfidelity" == mapperopt)
    {
        score = ql::quick_fidelity(past.lg);
    }
    else
    {
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
// distance=5   means length=6  means 4 swaps + 1 CZ gate, e.g.
// index in total:      0           1           2           length-3        length-2        length-1
// qubit:               2   ->      5   ->      7   ->      3       ->      1       CZ      4
void Split(std::list<Alter> & resla)
{
    // DOUT("Split ...");

    size_t length = total.size();
    MapperAssert (length >= 2);   // distance >= 1 so path at least: source -> target
    for (size_t rightopi = length-1; rightopi >= 1; rightopi--)
    {
        size_t leftopi = rightopi - 1;
        MapperAssert (leftopi >= 0);
        // DOUT("... leftopi=" << leftopi);
        // leftopi is the index in total that holds the qubit that becomes the left operand of the gate
        // rightopi is the index in total that holds the qubit that becomes the right operand of the gate
        // rightopi == leftopi + 1
        // fromSource will contain the path with qubits at indices 0 to leftopi
        // fromTarget will contain the path with qubits at indices rightopi to length-1, reversed
        //      reversal of fromTarget is done since swaps need to be generated starting at the target

        Alter    na = *this;      // na is local copy of the current path, including total
        // na = *this;            // na is local copy of the current path, including total
        // na.DPRINT("... copy of current alter");

        size_t fromi, toi;

        na.fromSource.resize(leftopi+1);
        // DOUT("... fromSource size=" << na.fromSource.size());
        for (fromi = 0, toi = 0; fromi <= leftopi; fromi++, toi++)
        {
            // DOUT("... fromSource: fromi=" << fromi << " toi=" << toi);
            na.fromSource[toi] = na.total[fromi];
        }

        na.fromTarget.resize(length-leftopi-1);
        // DOUT("... fromTarget size=" << na.fromTarget.size());
        for (fromi = length-1, toi = 0; fromi > leftopi; fromi--, toi++)
        {
            // DOUT("... fromTarget: fromi=" << fromi << " toi=" << toi);
            na.fromTarget[toi] = na.total[fromi];
        }

        // na.DPRINT("... copy of alter after split");
        resla.push_back(na);
        // DOUT("... added to result list");
        // DPRINT("... current alter after split");
    }
}

};  // end class Alter



// =========================================================================================
// Grid: definition and access functions to the grid of qubits that supports the real qubits.
// Maintain several maps to ease navigating in the grid; these are constant after initialization.

// Grid
//
// Config file definitions:
//  nq:                 hardware_settings.qubit_number
//  topology.form;      cross/plus/irregular: relation between neighbors in terms of x/y coordinates x[i]/y[i] qubits
//  topology.x_size/y_size: x/y space, defines underlying grid
//  topology.qubits:    mapping of qubit to x/y coordinates (defines x[i]/y[i] for each qubit i)
//  topology.edges:     mapping of edge (physical connection between 2 qubits) to its src and dst qubits (defines nbs)
//
// Grid public members (apart from nq):
//  form:               how presence of neighbors relates to x/y coordinates of qubits
//  Distance(qi,qj):    distance in physical connection hops from real qubit qi to real qubit qj;
//                      - computing it relies on nbs (and Floyd-Warshall)
//                      - in a fully assigned regular topology it could be defined by a formula (not supported)
//  nbs[qi]:            list of neighbor real qubits of real qubit qi
//                      - nbs can be derived from topology.edges or
//                      - nbs can be computed for a fully assigned regular topology (not supported)
//  Normalize(qi, neighborlist):    rotate neighborlist such that largest angle diff around qi is behind last element
//                      relies on nbs, and x[i]/y[i]
//
// For an irregular grid form, only nq and edges (so nbs) need to be specified; distance is computed from nbs:
// - there is no underlying rectangular grid, so there are no defined x and y coordinates of qubits;
//   this means that Normalize as needed by mappathselect==borders cannot work
// For a fully assigned and regular grid, we can have two forms: cross and plus, named after where neighbors are:
// - these forms use formulae for x- and y-coordinates and for distance; this is not supported (yet)
// Below, we support regular (xy) grids which need not be fully assigned; this requires edges (so nbs) to be defined,
//   from which distance is computed; also we have x/y coordinates per qubit specified in the configuration file
//   An underlying grid with x/y coordinates comes in use for:
//   - crossbars
//   - cclight qwg assignment (done in another manner now)
//   - when mappathselectopt==borders
typedef
enum GridForms
{
    gf_xy,          // nodes have explicit neighbor definitions, qubits have explicit x/y coordinates
//  gf_cross,       // nodes have implicit neighbor definitions along two diagonals, qubits have implicit x/y coordinates
//  gf_plus,        // nodes have implicit neighbor definitions horizontally and vertically, qubits have implicit x/y coords
    gf_irregular    // nodes have explicit neighbor definitions, qubits don't have x/y coordinates
} gridform_t;

class Grid
{
public:
    const ql::quantum_platform* platformp;    // current platform: topology
    size_t nq;                          // number of qubits in the platform
                                        // Grid configuration, all constant after initialization
    gridform_t form;                    // form of grid
    int nx;                             // length of x dimension (x coordinates count 0..nx-1)
    int ny;                             // length of y dimension (y coordinates count 0..ny-1)

    typedef std::list<size_t> neighbors_t;  // neighbors is a list of qubits
    std::map<size_t,neighbors_t> nbs;   // nbs[i] is list of neighbor qubits of qubit i
    std::map<size_t,int> x;             // x[i] is x coordinate of qubit i
    std::map<size_t,int> y;             // y[i] is y coordinate of qubit i
    std::vector<std::vector<size_t>>  dist; // dist[i][j] is computed distance between qubits i and j;

// Grid initializer
// initialize mapper internal grid maps from configuration
// this remains constant over multiple kernels on the same platform
void Init(const ql::quantum_platform* p)
{
    DOUT("Grid::Init");
    platformp = p;
    nq = platformp->qubit_number;
    DOUT("... number of real qbits=" << nq);

    std::string formstr;
    if (platformp->topology.count("form") <= 0)
    {
        formstr = "xy";
    }
    else
    {
        formstr = platformp->topology["form"].get<std::string>();
    }
    if (formstr == "xy") { form = gf_xy; }
    if (formstr == "irregular") { form = gf_irregular; }

    if (form == gf_irregular)
    {
        // irregular can do without topology.x_size, topology.y_size, and topology.qubits
        nx = 0;
        ny = 0;
    }
    else
    {
        // xy/cross/plus have an x/y space; coordinates are explicit for xy but implicit for cross/plus
        nx = platformp->topology["x_size"];
        ny = platformp->topology["y_size"];
    }
    DOUT("... formstr=" << formstr << "; form=" << form << "; nx=" << nx << "; ny=" << ny);

    InitXY();
    InitNbs();
    AngleSortNbs();
    ComputeDist();
    DPRINTGrid();
}

// distance between two qubits
// formulae for convex (hole free) topologies with underlying grid and with bidirectional edges:
//      gf_cross:   std::max( std::abs( x[to_realqi] - x[from_realqi] ), std::abs( y[to_realqi] - y[from_realqi] ))
//      gf_plus:    std::abs( x[to_realqi] - x[from_realqi] ) + std::abs( y[to_realqi] - y[from_realqi] )
// when the neighbor relation is defined (topology.edges in config file), Floyd-Warshall is used, which currently is always
size_t Distance(size_t from_realqi, size_t to_realqi)
{
    return dist[from_realqi][to_realqi];
}

// return clockwise angle around (cx,cy) of (x,y) wrt vertical y axis with angle 0 at 12:00, 0<=angle<2*pi
double Angle(int cx, int cy, int x, int y)
{
    const double pi = 4*std::atan(1);
    double a = std::atan2((x-cx),(y-cy));
    if (a < 0) a += 2*pi;
    return a;
}

// rotate neighbors list such that largest angle difference between adjacent elements is behind back;
// this is needed when a given subset of variations from a node is wanted (mappathselect==borders);
// and this can only be computed when there is an underlying x/y grid (so not for form==gf_irregular)
void Normalize( size_t src, neighbors_t& nbl )
{
    if (form == gf_irregular)
    {
        // there no implicit/explicit x/y coordinates defined per qubit, so no sense of nearness
        std::string mappathselectopt = ql::options::get("mappathselect");
        MapperAssert ("borders" != mappathselectopt);
        return;
    }

    // std::cout << "Normalizing list from src=" << src << ": ";
    // for (auto dn : nbl) { std::cout << dn << " "; } std::cout << std::endl;

    const double pi = 4*std::atan(1);
    if (nbl.size() == 1)
    {
        // DOUT("... size was 1; unchanged");
        return;
    }

    // find maxinx index in neighbor list before which largest angle difference occurs
    int maxdiff = 0;                            // current maximum angle difference in loop search below
    neighbors_t::iterator maxinx = nbl.begin(); // before which max diff occurs

    // for all indices in and its next one inx compute angle difference and find largest of these
    for (neighbors_t::iterator in = nbl.begin(); in != nbl.end(); in++)
    {
        double a_in = Angle(x[src], y[src], x[*in], y[*in]);

        neighbors_t::iterator inx = std::next(in); if (inx == nbl.end()) inx = nbl.begin();
        double a_inx = Angle(x[src], y[src], x[*inx], y[*inx]);

        int diff = a_inx - a_in; if (diff < 0) diff += 2*pi;
        if (diff > maxdiff)
        {
            maxdiff = diff;
            maxinx = inx;
        }
    }

    // and now rotate neighbor list so that largest angle difference is behind last one
    neighbors_t   newnbl;
    for (neighbors_t::iterator in = maxinx; in != nbl.end(); in++)
    {
        newnbl.push_back(*in);
    }
    for (neighbors_t::iterator in = nbl.begin(); in != maxinx; in++)
    {
        newnbl.push_back(*in);
    }
    nbl = newnbl;

    // std::cout << "... rotated; result: ";
    // for (auto dn : nbl) { std::cout << dn << " "; } std::cout << std::endl;
}

// Floyd-Warshall dist[i][j] = shortest distances between all nq qubits i and j
void ComputeDist()
{
    // initialize all distances to maximum value, to neighbors to 1, to itself to 0
    dist.resize(nq); for (size_t i=0; i<nq; i++) dist[i].resize(nq, MAX_CYCLE);
    for (size_t i=0; i<nq; i++)
    {
        dist[i][i] = 0;
        for (size_t j: nbs[i])
        {
            dist[i][j] = 1;
        }
    }

    // find shorter distances by gradually including more qubits (k) in path
    for (size_t k=0; k<nq; k++)
    {
        for (size_t i=0; i<nq; i++)
        {
            for (size_t j=0; j<nq; j++)
            {
               if (dist[i][j] > dist[i][k] + dist[k][j])
               {
                   dist[i][j] = dist[i][k] + dist[k][j];
               }
            }
        }
    }
#ifdef debug
    for (size_t i=0; i<nq; i++)
    {
        for (size_t j=0; j<nq; j++)
        {
            if (form == gf_cross)
            {
                MapperAssert (dist[i][j] == (std::max( std::abs( x[i] - x[j] ), std::abs( y[i] - y[j] ))) );
            }
            else if (form == gf_plus)
            {
                MapperAssert (dist[i][j] == (std::abs( x[i] - x[j] ) + std::abs( y[i] - y[j] )) );
            }

        }
    }
#endif
}

void DPRINTGrid()
{
    if ( ql::utils::logger::LOG_LEVEL >= ql::utils::logger::log_level_t::LOG_DEBUG )
        PrintGrid();
}
void PrintGrid()
{
    for (size_t i=0; i<nq; i++)
    {
        std::cout << "qubit[" << i << "]=(" << x[i] << "," << y[i] << ")";
        std::cout << " has neighbors ";
        for (auto & n : nbs[i])
        {
            std::cout << "qubit[" << n << "]=(" << x[n] << "," << y[n] << ") ";
        }
        std::cout << std::endl;
    }
    for (size_t i=0; i<nq; i++)
    {
        std::cout << "qubit[" << i << "] distance(" << i << ",j)=";
        for (size_t j=0; j<nq; j++)
        {
            std::cout << Distance(i,j) << " ";
        }
        std::cout << std::endl;
    }
}

// init x, and y maps
void InitXY()
{
    if (platformp->topology.count("qubits") == 0)
    {
        MapperAssert (form == gf_irregular);
    }
    else
    {
        for (auto & aqbit : platformp->topology["qubits"] )
        {
            size_t qi = aqbit["id"];
            int qx = aqbit["x"];
            int qy = aqbit["y"];
    
            // sanity checks
            if ( !(0<=qi && qi<nq) )
            {
                FATAL(" qbit in platform topology with id=" << qi << " is configured with id that is not in the range 0..nq-1 with nq=" << nq);
            }
            if (x.count(qi) > 0)
            {
                FATAL(" qbit in platform topology with id=" << qi << ": duplicate definition of x coordinate");
            }
            if (y.count(qi) > 0)
            {
                FATAL(" qbit in platform topology with id=" << qi << ": duplicate definition of y coordinate");
            }
            if ( !(0<=qx && qx<nx) )
            {
                FATAL(" qbit in platform topology with id=" << qi << " is configured with x that is not in the range 0..x_size-1 with x_size=" << nx);
            }
            if ( !(0<=qy && qy<ny) )
            {
                FATAL(" qbit in platform topology with id=" << qi << " is configured with y that is not in the range 0..y_size-1 with y_size=" << ny);
            }

            x[qi] = qx;
            y[qi] = qy;
        }
    }
}

// init nbs map
void InitNbs()
{
    if (platformp->topology.count("edges") == 0)
    {
        FATAL(" There aren't edges configured in the platform's topology");
    }
    for (auto & anedge : platformp->topology["edges"] )
    {
        size_t qs = anedge["src"];
        size_t qd = anedge["dst"];

        // sanity checks
        if ( !(0<=qs && qs<nq) )
        {
            FATAL(" edge in platform topology has src=" << qs << " that is not in the range 0..nq-1 with nq=" << nq);
        }
        if ( !(0<=qd && qd<nq) )
        {
            FATAL(" edge in platform topology has dst=" << qd << " that is not in the range 0..nq-1 with nq=" << nq);
        }
        for (auto & n : nbs[qs])
        {
            if (n == qd)
            {
                FATAL(" redefinition of edge with src=" << qs << " and dst=" << qd);
            }
        }

        nbs[qs].push_back(qd);
    }
}

// sort neighbor list by angles
void AngleSortNbs()
{
    for (size_t qi=0; qi<nq; qi++)
    {
        // sort nbs[qi] to have increasing clockwise angles around qi, starting with angle 0 at 12:00
        nbs[qi].sort(
            [this,qi](const size_t& i, const size_t& j)
            {
                return Angle(x[qi], y[qi], x[i], y[i]) < Angle(x[qi], y[qi], x[j], y[j]);
            }
        );
    }
}

};  // end class Grid


#ifdef INITIALPLACE
using namespace lemon;
// =========================================================================================
// InitialPlace: initial placement solved as an MIP, mixed integer linear program
// the initial placement is modelled as a Quadratic Assignment Problem
// by Lingling Lao in her mapping paper:
//  
// variables:
//     forall i: forall k: x[i][k], x[i][k] is integral and 0 or 1, meaning qubit i is in location k
// objective:
//     min z = sum i: sum j: sum k: sum l: refcount[i][j] * distance(k,l) * x[i][k] * x[j][l]
// subject to:
//     forall k: ( sum i: x[i][k] <= 1 )        allow more locations than qubits
//     forall i: ( sum k: x[i][k] == 1 )        but each qubit must have one locations
//  
// the article "An algorithm for the quadratic assignment problem using Benders' decomposition"
// by L. Kaufman and F. Broeckx, transforms this problem by introducing w[i][k] as follows:
//  
// forall i: forall k: w[i][k] =  x[i][k] * ( sum j: sum l: refcount[i][j] * distance(k,l) * x[j][l] )
//  
// to the following mixed integer linear problem:
//  
//  precompute:
//      forall i: forall k: costmax[i][k] = sum j: sum l: refcount[i][j] * distance(k,l)
//      (note: each of these costmax[][] is >= 0, so the "max(this,0)" around this is not needed)
//  variables:
//      forall i: forall k: x[i][k], x[i][k] is integral and 0 or 1
//      forall i: forall k: w[i][k], w[i][k] is real and >= 0
//  objective:
//      min z = sum i: sum k: w[i][k]
//  subject to:
//      forall k: ( sum i: x[i][k] <= 1 )
//      forall i: ( sum k: x[i][k] == 1 )
//      forall i: forall k: costmax[i][k] * x[i][k]
//          + ( sum j: sum l: refcount[i][j]*distance(k,l)*x[j][l] ) - w[i][k] <= costmax[i][k]
//
// This model is coded in lemon/mip below.
// The latter is mapped onto glpk.
//
// Since solving takes a while, two ways are offered to deal with this; these can be combined:
// 1. option initialplace2qhorizon: one of: 0,10,20,30,40,50,60,70,80,90,100
// The initialplace algorithm considers only this number of initial two-qubit gates to determine a mapping.
// When 0 is specified as option value, there is no limit.
// 2. option initialplace: an option steerable timeout mechanism around it is implemented, using threads:
// The solver runs in a subthread which can succeed or be timed out by the main thread waiting for it.
// When timed out, it can stop the compiler by raising an exception or continue mapping as if it were not called.
// When INITIALPLACE is not defined, the compiler doesn't contain initial placement support and ignores calls to it;
// then all this: lemon/mip, glpk, thread support is avoided making OpenQL much easier build and run.
// Otherwise, depending on the initialplace option value, initial placement is attempted before the heuristic.
// Options values of initialplace:
//  no      don't run initial placement ('ip')
//  yes     run ip until the solver is ready
//  1hx     run ip max for 1 hour; when timed out, stop the compiler
//  1h      run ip max for 1 hour; when timed out, just use heuristics
//  10mx    run ip max for 10 minutes; when timed out, stop the compiler
//  10m     run ip max for 10 minutes; when timed out, just use heuristics
//  1mx     run ip max for 1 minute; when timed out, stop the compiler
//  1m      run ip max for 1 minute; when timed out, just use heuristics
//  10sx    run ip max for 10 seconds; when timed out, stop the compiler
//  10s     run ip max for 10 seconds; when timed out, just use heuristics
//  1sx     run ip max for 1 second; when timed out, stop the compiler
//  1s      run ip max for 1 second; when timed out, just use heuristics

typedef
enum InitialPlaceResults
{
    ipr_any,            // any mapping will do because there are no two-qubit gates in the circuit
    ipr_current,        // current mapping will do because all two-qubit gates are NN
    ipr_newmap,         // initial placement solution found a mapping
    ipr_failed,         // initial placement solution failed
    ipr_timedout        // initial placement solution timed out and thus failed
} ipr_t;

class InitialPlace
{
private:
                                        // parameters, constant for a kernel
    const ql::quantum_platform   *platformp;  // platform
    size_t                  nlocs;      // number of locations, real qubits; index variables k and l
    size_t                  nvq;        // same range as nlocs; when not, take set from config and create v2i earlier
    Grid                   *gridp;      // current grid with Distance function

                                        // remaining attributes are computed per circuit
    size_t                  nfac;       // number of facilities, actually used virtual qubits; index variables i and j
                                        // nfac <= nlocs: e.g. nlocs == 7, but only v2 and v5 are used; nfac then is 2

public:

std::string ipr2string(ipr_t ipr)
{
    switch(ipr)
    {
    case ipr_any:       return "any";
    case ipr_current:   return "current";
    case ipr_newmap:    return "newmap";
    case ipr_failed:    return "failed";
    case ipr_timedout:  return "timedout";
    }
    return "unknown";
}

// kernel-once initialization
void Init(Grid* g, const ql::quantum_platform *p)
{
    // DOUT("InitialPlace Init ...");
    platformp = p;
    nlocs = p->qubit_number;
    nvq = p->qubit_number;  // same range; when not, take set from config and create v2i earlier
    // DOUT("... number of real qubits (locations): " << nlocs);
    gridp = g;
    DOUT("Init: platformp=" << platformp << " nlocs=" << nlocs << " nvq=" << nvq << " gridp=" << gridp);
}

// find an initial placement of the virtual qubits for the given circuit
// the resulting placement is put in the provided virt2real map
// result indicates one of the result indicators (ipr_t, see above)
void PlaceBody( ql::circuit& circ, Virt2Real& v2r, ipr_t &result, double& iptimetaken)
{
    DOUT("InitialPlace.PlaceBody ...");

    // check validity of circuit
    for ( auto& gp : circ )
    {
        auto&   q = gp->operands;
        if (q.size() > 2)
        {
            FATAL(" gate: " << gp->qasm() << " has more than 2 operand qubits; please decompose such gates first before mapping.");
        }
    }

    // only consider first number of two-qubit gates as specified by option initialplace2qhorizon
    // this influences refcount (so constraints) and nfac (number of facilities, so size of MIP problem)
    std::string initialplace2qhorizonopt = ql::options::get("initialplace2qhorizon");
    int  prefix = stoi(initialplace2qhorizonopt);

    // compute ipusecount[] to know which virtual qubits are actually used
    // use it to compute v2i, mapping (non-contiguous) virtual qubit indices to contiguous facility indices
    // (the MIP model is shorter when the indices are contiguous)
    // finally, nfac is set to the number of these facilities;
    // only consider virtual qubit uses until the specified max number of two qubit gates has been seen
    DOUT("... compute ipusecount by scanning circuit");
    std::vector<size_t>  ipusecount;// ipusecount[v] = count of use of virtual qubit v in current circuit
    ipusecount.resize(nvq,0);       // initially all 0
    std::vector<size_t> v2i;        // v2i[virtual qubit index v] -> index of facility i
    v2i.resize(nvq,UNDEFINED_QUBIT);// virtual qubit v not used by circuit as gate operand

    int  twoqubitcount = 0;
    for ( auto& gp : circ )
    {
        if (prefix == 0 || twoqubitcount < prefix)
        {
            for ( auto v : gp->operands)
            {
                ipusecount[v] += 1;
            }
        }
        if (gp->operands.size() == 2)
        {
            twoqubitcount++;
        }
    }
    nfac = 0;
    for (size_t v=0; v < nvq; v++)
    {
        if (ipusecount[v] != 0)
        {
            v2i[v] = nfac;
            nfac += 1;
        }
    }
    DOUT("... number of facilities: " << nfac << " while number of used virtual qubits is: " << nvq);

    // precompute refcount (used by the model as constants) by scanning circuit;
    // refcount[i][j] = count of two-qubit gates between facilities i and j in current circuit
    // at the same time, set anymap and currmap
    // anymap = there are no two-qubit gates so any map will do
    // currmap = in the current map, all two-qubit gates are NN so current map will do
    DOUT("... compute refcount by scanning circuit");
    std::vector<std::vector<size_t>>  refcount;
    refcount.resize(nfac); for (size_t i=0; i<nfac; i++) refcount[i].resize(nfac,0);
    bool anymap = true;    // true when all refcounts are 0
    bool currmap = true;   // true when in current map all two-qubit gates are NN
    
    twoqubitcount = 0;
    for ( auto& gp : circ )
    {
        auto&   q = gp->operands;
        if (q.size() == 2)
        {
            if (prefix == 0 || twoqubitcount < prefix)
            {
                anymap = false;
                refcount[v2i[q[0]]][v2i[q[1]]] += 1;
                
                if (v2r[q[0]] == UNDEFINED_QUBIT
                    || v2r[q[1]] == UNDEFINED_QUBIT
                    || gridp->Distance(v2r[q[0]], v2r[q[1]]) > 1
                    )
                {
                    currmap = false;
                }
            }
            twoqubitcount++;
        }
    }
    if (prefix != 0 && twoqubitcount >= prefix)
    {
        DOUT("InitialPlace: only considered " << prefix << " of " << twoqubitcount << " two-qubit gates, so resulting mapping is not exact");
    }
    if (anymap)
    {
        DOUT("InitialPlace: no two-qubit gates found, so no constraints, and any mapping is ok");
        DOUT("InitialPlace.PlaceBody [ANY MAPPING IS OK]");
        result = ipr_any;
        iptimetaken = 0.0;
        return;
    }
    if (currmap)
    {
        DOUT("InitialPlace: in current map, all two-qubit gates are nearest neighbor, so current map is ok");
        DOUT("InitialPlace.PlaceBody [CURRENT MAPPING IS OK]");
        result = ipr_current;
        iptimetaken = 0.0;
        return;
    }

    // compute iptimetaken, start interval timer here
    using namespace std::chrono;
    high_resolution_clock::time_point t1 = high_resolution_clock::now();

    // precompute costmax by applying formula
    // costmax[i][k] = sum j: sum l: refcount[i][j] * distance(k,l) for facility i in location k
    DOUT("... precompute costmax by combining refcount and distances");
    std::vector<std::vector<size_t>>  costmax;   
    costmax.resize(nfac); for (size_t i=0; i<nfac; i++) costmax[i].resize(nlocs,0);
    for ( size_t i=0; i<nfac; i++ )
    {
        for ( size_t k=0; k<nlocs; k++ )
        {
            for ( size_t j=0; j<nfac; j++ )
            {
                for ( size_t l=0; l<nlocs; l++ )
                {
                    costmax[i][k] += refcount[i][j] * (gridp->Distance(k,l) - 1);
                }
            }
        }
    }
    
    // the problem
    // mixed integer programming
    Mip  mip;
    
    // variables (columns)
    //  x[i][k] are integral, values 0 or 1
    //      x[i][k] represents whether facility i is in location k
    //  w[i][k] are real, values >= 0
    //      w[i][k] represents x[i][k] * sum j: sum l: refcount[i][j] * distance(k,l) * x[j][l]
    //       i.e. if facility i not in location k then 0
    //       else for all facilities j in its location l sum refcount[i][j] * distance(k,l)
    // DOUT("... allocate x column variable");
    std::vector<std::vector<Mip::Col>> x;
        x.resize(nfac); for (size_t i=0; i<nfac; i++) x[i].resize(nlocs);
    // DOUT("... allocate w column variable");
    std::vector<std::vector<Mip::Col>> w;
        w.resize(nfac); for (size_t i=0; i<nfac; i++) w[i].resize(nlocs);
    // DOUT("... add/initialize x and w column variables with trivial constraints and type");
    for ( size_t i=0; i<nfac; i++ )
    {
        for ( size_t k=0; k<nlocs; k++ )
        {
            x[i][k] = mip.addCol();
            mip.colLowerBound(x[i][k], 0);          // 0 <= x[i][k]
            mip.colUpperBound(x[i][k], 1);          //      x[i][k] <= 1
            mip.colType(x[i][k], Mip::INTEGER);     // int
            // DOUT("x[" << i << "][" << k << "] INTEGER >= 0 and <= 1");
    
            w[i][k] = mip.addCol();
            mip.colLowerBound(w[i][k], 0);          // 0 <= w[i][k]
            mip.colType(w[i][k], Mip::REAL);        // real
            // DOUT("w[" << i << "][" << k << "] REAL >= 0");
        }
    }
    
    // constraints (rows)
    //  forall i: ( sum k: x[i][k] == 1 )
    // DOUT("... add/initialize sum to 1 constraint rows");
    for ( size_t i=0; i<nfac; i++ )
    {
        Mip::Expr   sum;
        std::string s = "";
        bool started = false;
        for ( size_t k=0; k<nlocs; k++ )
        {
            sum += x[i][k];
            if (started) s += "+ "; else started = true;
            s += "x[";
            s += std::to_string(i);
            s += "][";
            s += std::to_string(k);
            s += "]";
        }
        mip.addRow(sum == 1);
        s += " == 1";
        // DOUT(s);
    }
    
    // constraints (rows)
    //  forall k: ( sum i: x[i][k] <= 1 )
    //  < 1 (i.e. == 0) may apply for a k when location k doesn't contain a qubit in this solution
    for ( size_t k=0; k<nlocs; k++ )
    {
        Mip::Expr   sum;
        std::string s = "";
        bool started = false;
        for ( size_t i=0; i<nfac; i++ )
        {
            sum += x[i][k];
            if (started) s += "+ "; else started = true;
            s += "x[";
            s += std::to_string(i);
            s += "]["; 
            s += std::to_string(k); 
            s += "]";
        }
        mip.addRow(sum <= 1);
        s += " <= 1";
        // DOUT(s);
    }
    
    // constraints (rows)
    //  forall i, k: costmax[i][k] * x[i][k]
    //          + sum j sum l refcount[i][j]*distance[k][l]*x[j][l] - w[i][k] <= costmax[i][k]
    // DOUT("... add/initialize nfac x nlocs constraint rows based on nfac x nlocs column combinations");
    for ( size_t i=0; i<nfac; i++ )
    {
        for ( size_t k=0; k<nlocs; k++ )
        {
            Mip::Expr   left = costmax[i][k] * x[i][k];
            std::string lefts = "";
            bool started = false;
            for ( size_t j=0; j<nfac; j++ )
            {
                for ( size_t l=0; l<nlocs; l++ )
                {
                    left += refcount[i][j] * gridp->Distance(k,l) * x[j][l];
                    if (refcount[i][j] * gridp->Distance(k,l) != 0)
                    {
                        if (started) lefts += " + "; else started = true;
                        lefts += std::to_string(refcount[i][j] * gridp->Distance(k,l));
                        lefts += " * x[";
                        lefts += std::to_string(j);
                        lefts += "][";
                        lefts += std::to_string(l);
                        lefts += "]";
                    }
                }
            }
            left -= w[i][k];
            lefts += "- w[";
            lefts += std::to_string(i);
            lefts += "][";
            lefts += std::to_string(k);
            lefts += "]";
            Mip::Expr   right = costmax[i][k];
            mip.addRow(left <= right);
            // DOUT(lefts << " <= " << costmax[i][k]);
        }
    }
    
    // objective
    Mip::Expr   objective;
    // DOUT("... add/initialize objective");
    std::string objs = "";
    bool started = false;
    mip.min();
    for ( size_t i=0; i<nfac; i++ )
    {
        for ( size_t k=0; k<nlocs; k++ )
        {
            objective += w[i][k];
            if (started) objs += "+ "; else started = true;
            objs += "w[";
            objs += std::to_string(i);
            objs += "][";
            objs += std::to_string(k);
            objs += "]";
        }
    }
    mip.obj(objective);
    // DOUT("MINIMIZE " << objs);

    DOUT("... v2r before solving, nvq=" << nvq);
    for (size_t v=0; v<nvq; v++)
    {
        DOUT("... about to print v2r[" << v << "]= ...");
        DOUT("....." << v2r[v]);
    }
    DOUT("..1 nvq=" << nvq);
    
    // solve the problem
    WOUT("... computing initial placement using MIP, this may take a while ...");
    DOUT("InitialPlace: solving the problem, this may take a while ...");
    DOUT("..2 nvq=" << nvq);
    Mip::SolveExitStatus s;
    DOUT("Just before solve: platformp=" << platformp << " nlocs=" << nlocs << " nvq=" << nvq << " gridp=" << gridp);
    DOUT("Just before solve: objs=" << objs << " x.size()=" << x.size() << " w.size()=" << w.size() << " refcount.size()=" << refcount.size() << " v2i.size()=" << v2i.size() << " ipusecount.size()=" << ipusecount.size());
    DOUT("..2b nvq=" << nvq);
    {
        s = mip.solve();
    }
    DOUT("..3 nvq=" << nvq);
    DOUT("Just after solve: platformp=" << platformp << " nlocs=" << nlocs << " nvq=" << nvq << " gridp=" << gridp);
    DOUT("Just after solve: objs=" << objs << " x.size()=" << x.size() << " w.size()=" << w.size() << " refcount.size()=" << refcount.size() << " v2i.size()=" << v2i.size() << " ipusecount.size()=" << ipusecount.size());
    MapperAssert(nvq == nlocs);         // consistency check, mainly to let it crash

    // computing iptimetaken, stop interval timer
    DOUT("..4 nvq=" << nvq);
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    duration<double> time_span = t2 - t1;
    iptimetaken = time_span.count();
    DOUT("..5 nvq=" << nvq);

    // DOUT("... determine result of solving");
    Mip::ProblemType pt = mip.type();
    DOUT("..6 nvq=" << nvq);
    if (s != Mip::SOLVED || pt != Mip::OPTIMAL)
    {
        DOUT("... InitialPlace: no (optimal) solution found; solve returned:"<< s << " type returned:" << pt);
        result = ipr_failed;
        DOUT("InitialPlace.PlaceBody [FAILED, DID NOT FIND MAPPING]");
        return;
    }
    DOUT("..7 nvq=" << nvq);

    // return new mapping as result in v2r

    // get the results: x[i][k] == 1 iff facility i is in location k (i.e. real qubit index k)
    // use v2i to translate facilities back to original virtual qubit indices
    // and fill v2r with the found locations for the used virtual qubits;
    // the unused mapped virtual qubits are mapped to an arbitrary permutation of the remaining locations;
    // the latter must be updated to generate swaps when mapping multiple kernels
    DOUT("..8 nvq=" << nvq);
    DOUT("... interpret result and copy to Virt2Real, nvq=" << nvq);
    for (size_t v=0; v<nvq; v++)
    {
        DOUT("... about to set v2r to undefined for v " << v);
        v2r[v] = UNDEFINED_QUBIT;      // i.e. undefined, i.e. v is not an index of a used virtual qubit
    }
    for ( size_t i=0; i<nfac; i++ )
    {
        size_t v;   // found virtual qubit index v represented by facility i
        // use v2i backward to find virtual qubit v represented by facility i
        DOUT("... about to inspect v2i to get solution and set it in v2r for facility " << i);
        for (v=0; v<nvq; v++)
        {
            if (v2i[v] == i)
            {
                break;
            }
        }
        MapperAssert(v < nvq);  // for each facility there must be a virtual qubit
        size_t k;   // location to which facility i being virtual qubit index v was allocated
        for (k=0; k<nlocs; k++ )
        {
            if (mip.sol(x[i][k]) == 1)
            {
                v2r[v] = k;
                // v2r.rs[] is not updated because no gates were really mapped yet
                break;
            }
        }
        MapperAssert(k < nlocs);  // each facility i by definition represents a used qubit so must have got a location
        DOUT("... end loop body over nfac");
    }

    auto mapinitone2oneopt = ql::options::get("mapinitone2one");
    if ("yes" == mapinitone2oneopt)
    {
        DOUT("... correct location of unused mapped virtual qubits to be an unused location");
        v2r.DPRINT("... result Virt2Real map of InitialPlace before mapping unused mapped virtual qubits ");
        // virtual qubits used by this kernel v have got their location k filled in in v2r[v] == k
        // unused mapped virtual qubits still have location UNDEFINED_QUBIT, fill with the remaining locs
        // this should be replaced by actually swapping them to there, when mapping multiple kernels
        for (size_t v=0; v<nvq; v++)
        {
            if (v2r[v] == UNDEFINED_QUBIT)
            {
                // v is unused by this kernel; find an unused location k
                size_t k;   // location k that is checked for having been allocated to some virtual qubit w
                for (k=0; k<nlocs; k++ )
                {
                    size_t w;
                    for (w=0; w<nvq; w++)
                    {
                        if (v2r[w] == k)
                        {
                            break;
                        }
                    }
                    if (w >= nvq)
                    {
                        // no w found for which v2r[w] == k
                        break;     // k is an unused location
                    }
                    // k is a used location, so continue with next k to check whether it is hopefully unused
                }
                MapperAssert(k < nlocs);  // when a virtual qubit is not used, there must be a location that is not used
                v2r[v] = k;
            }
            DOUT("... end loop body over nvq when mapinitone2oneopt");
        }
    }
    v2r.DPRINT("... final result Virt2Real map of InitialPlace");
    result = ipr_newmap;
    DOUT("InitialPlace.PlaceBody [SUCCESS, FOUND MAPPING]");
}

// the above PlaceBody is a regular function using circ, and updating v2r and result before it returns;
// it implements Initial Placement as if the call to Place in the mapper called PlaceBody directly;
// because it may take a while to return, a new Place and a PlaceWrapper are put in between;
// the idea is to run PlaceBody in a detached thread, that, when ready, signals the main thread;
// the main thread waits for this signal with a timeout value;
// all this is done in a try block where the catch is called on this timeout;
// why exceptions are used, is not clear, so it was replaced by PlaceWrapper returning "timedout" or not
// and this works as well ...
bool PlaceWrapper( ql::circuit& circ, Virt2Real& v2r, ipr_t& result, double& iptimetaken, std::string& initialplaceopt)
{
    DOUT("InitialPlace.PlaceWrapper called");
    std::mutex  m;
    std::condition_variable cv;

    // prepare timeout
    int      waitseconds;
    bool     andthrowexception = false;
    if ("1s" == initialplaceopt)        { waitseconds = 1; }
    else if ("1sx" == initialplaceopt)  { waitseconds = 1; andthrowexception = true; }
    else if ("10s" == initialplaceopt)  { waitseconds = 10; }
    else if ("10sx" == initialplaceopt) { waitseconds = 10; andthrowexception = true; }
    else if ("1m" == initialplaceopt)   { waitseconds = 60; }
    else if ("1mx" == initialplaceopt)  { waitseconds = 60; andthrowexception = true; }
    else if ("10m" == initialplaceopt)  { waitseconds = 600; }
    else if ("10mx" == initialplaceopt) { waitseconds = 600; andthrowexception = true; }
    else if ("1h" == initialplaceopt)   { waitseconds = 3600; }
    else if ("1hx" == initialplaceopt)  { waitseconds = 3600; andthrowexception = true; }
    else
    {
        FATAL("Unknown value of option 'initialplace'='" << initialplaceopt << "'.");
    }
    iptimetaken = waitseconds;    // pessimistic, in case of timeout, otherwise it is corrected

    // v2r and result are allocated on stack of main thread by some ancestor so be careful with threading
    std::thread t([&cv, this, &circ, &v2r, &result, &iptimetaken]()
        {
            DOUT("InitialPlace.PlaceWrapper subthread about to call PlaceBody");
            PlaceBody(circ, v2r, result, iptimetaken);
            DOUT("InitialPlace.PlaceBody returned in subthread; about to signal the main thread");
            cv.notify_one();        // by this, the main thread awakes from cv.wait_for without timeout
            DOUT("InitialPlace.PlaceWrapper subthread after signaling the main thread, and is about to die");
        }
    );
    DOUT("InitialPlace.PlaceWrapper main code created thread; about to call detach on it");
    t.detach();
    DOUT("InitialPlace.PlaceWrapper main code detached thread");
    {
        std::chrono::seconds maxwaittime(waitseconds);
        std::unique_lock<std::mutex> l(m);
        DOUT("InitialPlace.PlaceWrapper main code starts waiting with timeout of " << waitseconds << " seconds");
        if (cv.wait_for(l, maxwaittime) == std::cv_status::timeout)
        {
            DOUT("InitialPlace.PlaceWrapper main code awoke from waiting with timeout");
            if (andthrowexception)
            {
                DOUT("InitialPlace: timed out and stops compilation [TIMED OUT, STOP COMPILATION]");
                FATAL("Initial placement timed out and stops compilation [TIMED OUT, STOP COMPILATION]");
            }
            DOUT("InitialPlace.PlaceWrapper about to return timedout==true");
            return true;
        }
        DOUT("InitialPlace.PlaceWrapper main code awoke from waiting without timeout, from signal sent by InitialPlace.PlaceWrapper subthread just before its death");
    }

    DOUT("InitialPlace.PlaceWrapper about to return timedout==false");
    return false;
}

// find an initial placement of the virtual qubits for the given circuit as in Place
// put a timelimit on its execution specified by the initialplace option
// when it expires, result is set to ipr_timedout;
// details of how this is accomplished, can be found above;
// v2r is updated by PlaceBody/PlaceWrapper when it has found a mapping
void Place( ql::circuit& circ, Virt2Real& v2r, ipr_t& result, double& iptimetaken, std::string& initialplaceopt)
{
    Virt2Real   v2r_orig = v2r;

    DOUT("InitialPlace.Place ...");
    if ("yes" == initialplaceopt)
    {
        // do initial placement without time limit
        DOUT("InitialPlace.Place calling PlaceBody without time limit");
        PlaceBody(circ, v2r, result, iptimetaken);
        // v2r reflects new mapping, if any found, otherwise unchanged
        DOUT("InitialPlace.Place [done, no time limit], result=" << result << " iptimetaken=" << iptimetaken << " seconds");
    }
    else
    {
        bool timedout;
        timedout = PlaceWrapper(circ, v2r, result, iptimetaken, initialplaceopt);
    
        if (timedout)
        {
            result = ipr_timedout;
            DOUT("InitialPlace.Place [done, TIMED OUT, NO MAPPING FOUND], result=" << result << " iptimetaken=" << iptimetaken << " seconds");

            v2r = v2r_orig; // v2r may have got corrupted when timed out during v2r updating
        }
        else
        {
            // v2r reflects new mapping, if any found, otherwise unchanged
            DOUT("InitialPlace.Place [done, not timed out], result=" << result << " iptimetaken=" << iptimetaken << " seconds");
        }
    }
}
    
};  // end class InitialPlace
#endif // INITIALPLACE

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

class Future
{
public:
    const ql::quantum_platform            *platformp;
    Scheduler                       *schedp;        // a pointer, since dependence graph doesn't change
    ql::circuit                     input_gatepv;   // input circuit when not using scheduler based avlist

    std::map<ql::gate*,bool>        scheduled;      // state: has gate been scheduled, here: done from future?
    std::list<ListDigraph::Node>    avlist;         // state: which nodes/gates are available for mapping now?
    ql::circuit::iterator           input_gatepp;   // state: alternative iterator in input_gatepv

// just program wide initialization
void Init( const ql::quantum_platform *p)
{
    // DOUT("Future::Init ...");
    platformp = p;
    // DOUT("Future::Init [DONE]");
}

// Set/switch input to the provided circuit
// nq and nc are parameters because nc may not be provided by platform but by kernel
// the latter should be updated when mapping multiple kernels
void SetCircuit(ql::quantum_kernel& kernel, Scheduler& sched, size_t nq, size_t nc)
{
    DOUT("Future::SetCircuit ...");
    schedp = &sched;
    std::string maplookaheadopt = ql::options::get("maplookahead");
    if ("no" == maplookaheadopt)
    {
        input_gatepv = kernel.c;                                // copy to free original circuit to allow outputing to
        input_gatepp = input_gatepv.begin();                    // iterator set to start of input circuit copy
    }
    else
    {
        schedp->init(kernel.c, *platformp, nq, nc);             // fills schedp->graph (dependence graph) from all of circuit
                                                                // and so also the original circuit can be output to after this
        for( auto & gp : kernel.c )
        {
            scheduled[gp] = false;   // none were scheduled
        }
        scheduled[schedp->instruction[schedp->s]] = false;      // also the dummy nodes not
        scheduled[schedp->instruction[schedp->t]] = false;
        avlist.clear();
        avlist.push_back(schedp->s);
        schedp->set_remaining(ql::forward_scheduling);          // to know criticality

        if (ql::options::get("print_dot_graphs") == "yes")
        {
            std::string     map_dot;
            std::stringstream fname;

            schedp->get_dot(map_dot);

            fname << ql::options::get("output_dir") << "/" << kernel.name << "_" << "mapper" << ".dot";
            IOUT("writing " << "mapper" << " dependence graph dot file to '" << fname.str() << "' ...");
            ql::utils::write_file(fname.str(), map_dot);
        }
    }
    DOUT("Future::SetCircuit [DONE]");
}

// Get from avlist all gates that are non-quantum into nonqlg
// Non-quantum gates include: classical, and dummy (SOURCE/SINK)
// Return whether some non-quantum gate was found
bool GetNonQuantumGates(std::list<ql::gate*>& nonqlg)
{
    nonqlg.clear();
    std::string maplookaheadopt = ql::options::get("maplookahead");
    if ("no" == maplookaheadopt)
    {
        ql::gate*   gp = *input_gatepp;
        if (input_gatepp != input_gatepv.end())
        {
            if (gp->type() == ql::__classical_gate__
                || gp->type() == ql::__dummy_gate__
                )
            {
                nonqlg.push_back(gp);
            }
        }
    }
    else
    {
        for ( auto n : avlist)
        {
            ql::gate*  gp = schedp->instruction[n];
            if (gp->type() == ql::__classical_gate__
                || gp->type() == ql::__dummy_gate__
                )
            {
                nonqlg.push_back(gp);
            }
        }
    }
    return nonqlg.size() != 0;
}

// Get all gates from avlist into qlg
// Return whether some gate was found
bool GetGates(std::list<ql::gate*>& qlg)
{
    qlg.clear();
    std::string maplookaheadopt = ql::options::get("maplookahead");
    if ("no" == maplookaheadopt)
    {
        if (input_gatepp != input_gatepv.end())
        {
            ql::gate*  gp = *input_gatepp;
            if (gp->operands.size() > 2)
            {
                FATAL(" gate: " << gp->qasm() << " has more than 2 operand qubits; please decompose such gates first before mapping.");
            }
            qlg.push_back(gp);
        }
    }
    else
    {
        for ( auto n : avlist)
        {
            ql::gate*  gp = schedp->instruction[n];
            if (gp->operands.size() > 2)
            {
                FATAL(" gate: " << gp->qasm() << " has more than 2 operand qubits; please decompose such gates first before mapping.");
            }
            qlg.push_back(gp);
        }
    }
    return qlg.size() != 0;
}

// Indicate that a gate currently in avlist has been mapped, can be taken out of the avlist
// and its successors can be made available
void DoneGate(ql::gate* gp)
{
    std::string maplookaheadopt = ql::options::get("maplookahead");
    if ("no" == maplookaheadopt)
    {
        input_gatepp = std::next(input_gatepp);
    }
    else
    {
        schedp->TakeAvailable(schedp->node[gp], avlist, scheduled, ql::forward_scheduling);
    }
}

// Return gp in lag that is most critical (provided lookahead is enabled)
// This is used in tiebreak, when every other option has failed to make a distinction.
ql::gate* MostCriticalIn(std::list<ql::gate*>& lag)
{
    std::string maplookaheadopt = ql::options::get("maplookahead");
    if ("no" == maplookaheadopt)
    {
        return lag.front();
    }
    else
    {
        return schedp->find_mostcritical(lag);
    }
}

};  // end class Future



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
// and assumes vqi, nvq, rqi and nrq to be of the same type (size_t) 0<=qi<nrq.
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
// The design of mapping multiple kernels is as follows (HERE, TO BE ADAPTED TO NEW REALSTATE):
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
class Mapper
{
private:
                                    // Initialized by Mapper::Init
                                    // OpenQL wide configuration, all constant after initialization
    const ql::quantum_platform *platformp;// current platform: topology and gate definitions
    ql::quantum_kernel   *kernelp;  // (copy of) current kernel (class) with free private circuit and methods
                                    // primarily to create gates in Past; Past is part of Mapper and of each Alter
                                    
    size_t          nq;             // number of qubits in the platform, number of real qubits
    size_t          nc;             // number of cregs in the platform, number of classical registers
    size_t          cycle_time;     // length in ns of a single cycle of the platform
                                    // is divisor of duration in ns to convert it to cycles
    Grid            grid;           // current grid

                                    // Initialized by Mapper.Map
    std::mt19937    gen;            // Standard mersenne_twister_engine, not yet seeded

public:
                                    // Passed back by Mapper::Map to caller for reporting
    size_t          nswapsadded;    // number of swaps added (including moves)
    size_t          nmovesadded;    // number of moves added
    std::vector<size_t> v2r_in;     // v2r[virtual qubit index] -> real qubit index | UNDEFINED_QUBIT
    std::vector<int>    rs_in;      // rs[real qubit index] -> {nostate|wasinited|hasstate}
    std::vector<size_t> v2r_ip;     // v2r[virtual qubit index] -> real qubit index | UNDEFINED_QUBIT
    std::vector<int>    rs_ip;      // rs[real qubit index] -> {nostate|wasinited|hasstate}
    std::vector<size_t> v2r_out;    // v2r[virtual qubit index] -> real qubit index | UNDEFINED_QUBIT
    std::vector<int>    rs_out;     // rs[real qubit index] -> {nostate|wasinited|hasstate}


// Mapper constructor is default synthesized

private:

// initial path finder
// generate paths with source src and target tgt as a list of path into resla;
// this result list resla is allocated by caller and is empty on the call;
// which indicates which paths are generated; see below the enum whichpaths;
// on top of this, the other mapper options apply
typedef
enum {
    wp_all_shortest,            // all shortest paths
    wp_left_shortest,           // only the shortest along the left side of the rectangle of src and tgt
    wp_right_shortest,          // only the shortest along the right side of the rectangle of src and tgt
    wp_leftright_shortest       // both the left and right shortest
} whichpaths_t;

// Find shortest paths between src and tgt in the grid, bounded by a particular strategy (which)
void GenShortestPaths(ql::gate* gp, size_t src, size_t tgt, std::list<Alter> & resla, whichpaths_t which)
{
    std::list<Alter> genla;    // list that will get the result of a recursive Gen call

    // DOUT("GenShortestPaths: " << "src=" << src << " tgt=" << tgt << " which=" << which);
    MapperAssert (resla.empty());

    if (src == tgt) {
        // found target
        // create a virgin Alter and initialize it to become an empty path
        // add src to this path (so that it becomes a distance 0 path with one qubit, src)
        // and add the Alter to the result list 
        Alter  a;
        a.Init(platformp, kernelp);
        a.targetgp = gp;
        a.Add2Front(src);
        resla.push_back(a);
        // a.DPRINT("... empty path after adding to result list");
        // Alter::DPRINT("... result list after adding empty path", resla);
        // DOUT("... will return now");
        return;
    }

    // start looking around at neighbors for serious paths
    // assume that distance is not approximate but exact and can be met
    size_t d = grid.Distance(src, tgt);
    MapperAssert (d >= 1);

    // reduce neighbors nbs to those continuing a shortest path
    auto nbl = grid.nbs[src];
    nbl.remove_if( [this,d,tgt](const size_t& n) { return grid.Distance(n,tgt) >= d; } );

    // rotate neighbor list nbl such that largest difference between angles of adjacent elements is beyond back()
    grid.Normalize(src, nbl);
    // subset to those neighbors that continue in direction(s) we want
    if (which == wp_left_shortest)
    {
        nbl.remove_if( [nbl](const size_t& n) { return n != nbl.front(); } );
    }
    else if (which == wp_right_shortest)
    {
        nbl.remove_if( [nbl](const size_t& n) { return n != nbl.back(); } );
    }
    else if (which == wp_leftright_shortest)
    {
        nbl.remove_if( [nbl](const size_t& n) { return n != nbl.front() && n != nbl.back(); } );
    }

    // std::cout << "... before iterating, nbl: ";
    // for (auto dn : nbl) { std::cout << dn << " "; } std::cout << std::endl;

    // for all resulting neighbors, find all continuations of a shortest path
    for (auto & n : nbl)
    {
        whichpaths_t newwhich = which;
        // but for each neighbor only look in desired direction, if any
        if (which == wp_leftright_shortest && nbl.size() != 1)
        {
            // when looking both left and right still, and there is a choice now, split into left and right
            if (n == nbl.front())
            {
                newwhich = wp_left_shortest;
            }
            else
            {
                newwhich = wp_right_shortest;
            }
        }
        GenShortestPaths(gp, n, tgt, genla, newwhich);  // get list of possible paths from n to tgt in genla
        resla.splice(resla.end(), genla);           // moves all of genla to resla; makes genla empty
    }
    // resla contains all paths starting from a neighbor of src, to tgt

    // add src to front of all to-be-returned paths from src's neighbors to tgt
    for (auto & a : resla)
    {
        // DOUT("... GenShortestPaths, about to add src=" << src << "in front of path");
        a.Add2Front(src);
    }
    // DOUT("... GenShortestPaths, returning from call of: " << "src=" << src << " tgt=" << tgt << " which=" << which);
}

// Generate shortest paths in the grid
void GenShortestPaths(ql::gate* gp, size_t src, size_t tgt, std::list<Alter> & resla)
{
    std::string mappathselectopt = ql::options::get("mappathselect");
    if ("all" == mappathselectopt)
    {
        GenShortestPaths(gp, src, tgt, resla, wp_all_shortest);
    }
    else if ("borders" == mappathselectopt)
    {
        GenShortestPaths(gp, src, tgt, resla, wp_leftright_shortest);
    }
    else
    {
        FATAL("Unknown value of mapppathselect option " << mappathselectopt);
    }
}

// split each path in the argument old Alter list
// this gives all variations to put the two-qubit gate in the path
// all possible Alters are returned in the result list resla
void GenSplitPaths(std::list<Alter> & oldla, std::list<Alter> & resla)
{
    // DOUT("GenSplitPaths");
    for (auto & a : oldla)
    {
        a.Split(resla);
    }
    // Alter::DPRINT("... after GenSplitPaths", resla);
}

// Generate all possible variations of making gp NN, starting from given past (with its mappings),
// and return the found variations by appending them to the given list of Alters, la
void GenAltersGate(ql::gate* gp, std::list<Alter>& la, Past& past)
{
    auto&   q = gp->operands;
    MapperAssert (q.size() == 2);
    size_t  src = past.MapQubit(q[0]);  // interpret virtual operands in past's current map
    size_t  tgt = past.MapQubit(q[1]);
    size_t d = grid.Distance(src, tgt);     // and find distance between real counterparts
    DOUT("GenAltersGate: " << gp->qasm() << " in real (q" << src << ",q" << tgt << ") at distance=" << d );
    past.DFcPrint();

    std::list<Alter> directla;  // list that will hold all Alters directly from src to tgt
    GenShortestPaths(gp, src, tgt, directla);// find shortest paths directly from src to tgt, without split
    // Alter::DPRINT("... after GenShortestPaths", directla);
    GenSplitPaths(directla, la);  // split each: 2q gate can be put at each hop in each direct path
    // Alter::DPRINT("... after GenSplitPaths", la);
}

// Generate all possible variations of making gates in lg NN, starting from given past (with its mappings),
// and return the found variations by appending them to the given list of Alters, la
// Depending on maplookahead only take first (most critical) gate or take all gates.
void GenAlters(std::list<ql::gate*> lg, std::list<Alter>& la, Past& past)
{
    std::string maplookaheadopt = ql::options::get("maplookahead");
    if ("all" == maplookaheadopt)
    {
        // create alternatives for each gate in lg
        // DOUT("GenAlters, " << lg.size() << " 2q gates; create an alternative for each");
        for (auto gp : lg)
        {
            // gen alternatives for gp and add these to la
            // DOUT("GenAlters: create alternatives for: " << gp->qasm());
            GenAltersGate(gp, la, past);  // gen all possible variations to make gp NN, in current v2r mapping ("past")
        }
    }
    else
    {
        // only take the first gate in avlist, the most critical one, and generate alternatives for it
        ql::gate*  gp = lg.front();
        // DOUT("GenAlters, " << lg.size() << " 2q gates; take first: " << gp->qasm());
        GenAltersGate(gp, la, past);  // gen all possible variations to make gp NN, in current v2r mapping ("past")
    }
}

// start the random generator with a seed
// that is unique to the microsecond
void RandomInit()
{
    auto ts = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    // DOUT("Seeding random generator with " << ts );
    gen.seed(ts);
}

// if the maptiebreak option indicates so,
// generate a random int number in range 0..count-1 and use
// that to index in list of alternatives and to return that one,
// otherwise return a fixed one (front, back or first most critical one
Alter ChooseAlter(std::list<Alter>& la, Future& future)
{
    if (la.size() == 1)
    {
        return la.front();
    }

    std::string maptiebreakopt = ql::options::get("maptiebreak");
    if ("critical" == maptiebreakopt)
    {
        std::list<ql::gate*> lag;
        for (auto& a : la)
        {
            lag.push_back(a.targetgp);
        }
        ql::gate*   gp = future.MostCriticalIn(lag);
        MapperAssert(gp != NULL);
        for (auto& a : la)
        {
            if (a.targetgp == gp)
            {
                // DOUT(" ... took first alternative with most critical target gate");
                return a;
            }
        }
        return la.front();
    }
    if ("random" == maptiebreakopt)
    {
        Alter res;
        std::uniform_int_distribution<> dis(0, (la.size()-1));
        size_t choice = dis(gen);
        size_t i = 0;
        for (auto& a : la)
        {
            if (i == choice)
            {
                res = a;
                break;
            }
            i++;
        }
        // DOUT(" ... took random draw " << choice << " from 0.." << (la.size()-1));
        return res;
    }
    if ("last" == maptiebreakopt)
    {
        // DOUT(" ... took last " << " from 0.." << (la.size()-1));
        return la.back();
    }
    if ("first" == maptiebreakopt)
    {
        // DOUT(" ... took first " << " from 0.." << (la.size()-1));
        return la.front();
    }
    return la.front();  // to shut up gcc
}

// Map the gate/operands of a gate that has been routed or doesn't require routing
void MapRoutedGate(ql::gate* gp, Past& past)
{
    DOUT("MapRoutedGate on virtual: " << gp->qasm() );

    // MakeReal of this gate maps its qubit operands and optionally updates its gate name
    // when the gate name was updated, a new gate with that name is created;
    // when that new gate is a composite gate, it is immediately decomposed (by gate creation)
    // the resulting gate/expansion (anyhow a sequence of gates) is collected in circ
    ql::circuit circ;   // result of MakeReal
    past.MakeReal(gp, circ);        
    for (auto newgp : circ)
    {
        DOUT(" ... new mapped real gate, about to be added to past: " << newgp->qasm() );
        past.AddAndSchedule(newgp);
    }
}

// commit Alter resa
// generating swaps in past
// and taking it out of future when done with it
void CommitAlter(Alter& resa, Future& future, Past& past)
{
    ql::gate*  resgp = resa.targetgp;   // and the 2q target gate then in resgp
    resa.DPRINT("... CommitAlter, alternative to commit, will add swaps and then map target 2q gate");

    std::string mapselectswapsopt = ql::options::get("mapselectswaps");
    resa.AddSwaps(past, mapselectswapsopt);

    // when only some swaps were added, the resgp might not yet be NN, so recheck
    auto&   q = resgp->operands;
    if (grid.Distance(past.MapQubit(q[0]), past.MapQubit(q[1])) == 1)
    {
        // resgp is NN: so done with this 2q gate
        // DOUT("... CommitAlter, target 2q is NN, map it and done: " << resgp->qasm());
        MapRoutedGate(resgp, past);     // the 2q target gate is NN now and thus can be mapped
        future.DoneGate(resgp);         // and then taken out of future
    } else {
        // DOUT("... CommitAlter, target 2q is not NN yet, keep it: " << resgp->qasm());
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
bool MapMappableGates(Future& future, Past& past, std::list<ql::gate*>& lg, bool alsoNN2q)
{
    std::list<ql::gate*>   nonqlg; // list of non-quantum gates in avlist
    std::list<ql::gate*>   qlg;    // list of (remaining) gates in avlist

    DOUT("MapMappableGates entry");
    while (1)
    {
        if (future.GetNonQuantumGates(nonqlg))
        {
            // avlist contains non-quantum gates
            // and GetNonQuantumGates indicates these (in nonqlg) must be done first
            DOUT("MapMappableGates, there is a set of non-quantum gates");
            for (auto gp : nonqlg)
            {
                // here add code to map qubit use of any non-quantum instruction????
                // dummy gates are nonq gates internal to OpenQL such as SOURCE/SINK; don't output them
                if ( gp->type() != ql::__dummy_gate__)
                {
                    // past only can contain quantum gates, so non-quantum gates must by-pass Past
                    past.ByPass(gp);    // this flushes past.lg first to outlg
                }
                future.DoneGate(gp); // so on avlist= nonNN2q -> NN2q -> 1q -> nonq: the nonq is done first
                DOUT("MapMappableGates, done with " << gp->qasm());
            }
            DOUT("MapMappableGates, done with set of non-quantum gates, continuing ...");
            continue;
        }
        if (!future.GetGates(qlg))
        {
            DOUT("MapMappableGates, no gates anymore, return");
            // avlist doesn't contain any gate
            lg.clear();
            return false;
        }

        // avlist contains quantum gates
        // and GetNonQuantumGates/GetGates indicate these (in qlg) must be done now
        bool foundone = false;  // whether a quantum gate was found that never requires routing
        for (auto gp : qlg)
        {
            if ( gp->type() == ql::gate_type_t::__wait_gate__
                || gp->operands.size() == 1
                )
            {
                // a quantum gate not requiring routing ever is found
                MapRoutedGate(gp, past);
                future.DoneGate(gp);
                foundone = true;    // a quantum gate was found that never requires routing
                                    // so on avlist= nonNN2q -> NN2q -> 1q: the 1q is done first
                break;
            }
        }
        if (foundone)
        {
            continue;
        }
        // qlg only contains 2q gates (that could require routing)
        if ( alsoNN2q)
        {
            // when there is a 2q in qlg that is mappable already, map it
            // when more, take most critical one first (because qlg is ordered, most critical first)
            for (auto gp : qlg)
            {
                auto&   q = gp->operands;
                size_t  src = past.MapQubit(q[0]);      // interpret virtual operands in current map
                size_t  tgt = past.MapQubit(q[1]);
                size_t  d = grid.Distance(src, tgt);    // and find distance between real counterparts
                if (d == 1)
                {
                    DOUT("MapMappableGates, NN no routing: " << gp->qasm() << " in real (q" << src << ",q" << tgt << ")");
                    MapRoutedGate(gp, past);
                    future.DoneGate(gp);
                    foundone = true;    // a 2q quantum gate was found that was mappable
                                        // so on avlist= nonNN2q -> NN2q: the NN2q is done first
                    break;
                }
            }
            if (foundone)
            {
                // found a mappable 2q one, and mapped it
                // don't map more mappable 2q ones
                // (they might not be critical, the now available 1q gates may hide a more critical 2q gate),
                // but deal with all available non-quantum, and 1q gates first,
                // and only when non of those remain, map a next mappable 2q (now most critical) one
                DOUT("MapMappableGates, found and mapped an easy quantum gate, continuing ...");
                continue;
            }
            DOUT("MapMappableGates, only nonNN 2q gates remain: ...");
        }
        else
        {
            DOUT("MapMappableGates, only 2q gates remain (nonNN and NN): ...");
        }
        // avlist (qlg) only contains 2q gates (when alsoNN2q: only non-NN ones; otherwise also perhaps NN ones)
        lg = qlg;
        if ( ql::utils::logger::LOG_LEVEL >= ql::utils::logger::log_level_t::LOG_DEBUG )
        {
            for (auto gp: lg)
            {
                DOUT("... 2q gate returned: " << gp->qasm());
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
void SelectAlter(std::list<Alter>& la, Alter & resa, Future& future, Past& past, Past& basePast, int level)
{
                                // la are all alternatives we enter with
    MapperAssert(!la.empty());  // so there is always a result Alter

    std::list<Alter> gla;       // good alternative subset of la, suitable to go in recursion with
    std::list<Alter> bla;       // best alternative subset of gla, suitable to choose result from

    DOUT("SelectAlter ENTRY level=" << level << " from " << la.size() << " alternatives");
    auto mapperopt = ql::options::get("mapper");
    if (mapperopt == "base"|| mapperopt == "baserc")
    {
        Alter::DPRINT("... SelectAlter base (equally good/best) alternatives:", la);
        resa = ChooseAlter(la, future);
        resa.DPRINT("... the selected Alter is");
        // DOUT("SelectAlter DONE level=" << level << " from " << la.size() << " alternatives");
        return;
    }
    MapperAssert(mapperopt == "minextend" || mapperopt == "minextendrc" || mapperopt == "maxfidelity");

    // Compute a.score of each alternative relative to basePast, and sort la on it, minimum first
    for (auto & a : la)
    {
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
    gla.remove_if( [this,la](const Alter& a) { return a.score != la.front().score; } );
    size_t  las = la.size();
    size_t  glas = gla.size();
    auto mapselectmaxwidthopt = ql::options::get("mapselectmaxwidth");
    if ("min" != mapselectmaxwidthopt)
    {
        size_t  keep = 1;
        if ( "minplusone" == mapselectmaxwidthopt)
        {
            keep = glas+1;
        }
        else if ("minplushalfmin" == mapselectmaxwidthopt)
        {
            keep = glas+glas/2;
        }
        else if ( "minplusmin" == mapselectmaxwidthopt)
        {
            keep = glas*2;
        }
        else if ( "all" == mapselectmaxwidthopt)
        {
            keep = las;
        }
        if (keep < las)
        {
            gla = la;
            std::list<Alter>::iterator  ia;
            ia = gla.begin();
            advance(ia, keep);
            gla.erase(ia, gla.end());
        }
        else
        {
            gla = la;
        }
    }
    // DOUT("SelectAlter mapselectmaxwidth=" << mapselectmaxwidthopt << " level=" << level << " reduced la to gla");
    Alter::DPRINT("... SelectAlter good alternatives before recursion:", gla);

    // Prepare for recursion;
    // option mapselectmaxlevel indicates the maximum level of recursion (0 is no recursion)
    auto mapselectmaxlevelstring = ql::options::get("mapselectmaxlevel");
    int  mapselectmaxlevel = ("inf" == mapselectmaxlevelstring) ?  MAX_CYCLE : atoi(mapselectmaxlevelstring.c_str());

    // When maxlevel has been reached, stop the recursion, and choose from the best minextend/maxfidelity alternatives
    if (level >= mapselectmaxlevel)
    {
        // Reduce list of good alternatives (gla) to list of minextend/maxfidelity best alternatives (bla)
        // and make a choice from that list to return as result
        bla = gla;
        bla.remove_if( [this,gla](const Alter& a) { return a.score != gla.front().score; } );
        Alter::DPRINT("... SelectAlter reduced to best alternatives to choose result from:", bla);
        resa = ChooseAlter(bla, future);
        resa.DPRINT("... the selected Alter (STOPPING RECURSION) is");
        // DOUT("SelectAlter DONE level=" << level << " from " << bla.size() << " best alternatives");
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
    // DOUT("... SelectAlter level=" << level << " entering recursion with " << gla.size() << " good alternatives");
    for (auto& a : gla)
    {
        a.DPRINT("... ... considering alternative:");
        Future future_copy = future;            // copy!
        Past   past_copy = past;                // copy!
        CommitAlter(a, future_copy, past_copy);
        a.DPRINT("... ... committed this alternative first before recursion:");

        bool    havegates;                  // are there still non-NN 2q gates to map?
        std::list<ql::gate*> lg;            // list of non-NN 2q gates taken from avlist, as returned from MapMappableGates
        std::string maplookaheadopt = ql::options::get("maplookahead");
        std::string maprecNN2qopt = ql::options::get("maprecNN2q");
        // In recursion, look at option maprecNN2q:
        // - MapMappableGates with alsoNN2q==true is greedy and immediately maps each 1q and NN 2q gate
        // - MapMappableGates with alsoNN2q==false is not greedy, maps all 1q gates but not the (NN) 2q gates
        //
        // when yes and when maplookaheadopt is noroutingfirst or all, let MapMappableGates stop mapping only on nonNN2q
        // when no, let MapMappableGates stop mapping on any 2q
        // This creates more clear recursion: one 2q at a time instead of a possible empty set of NN2qs followed by a nonNN2q;
        // also when a NN2q is found, this is perfect; this is not seen when immediately mapping all NN2qs.
        // So goal is to prove that maprecNN2q should be no at this place, in the recursion step, but not at level 0!
        bool alsoNN2q = ("yes" == maprecNN2qopt) && ( "noroutingfirst" == maplookaheadopt || "all" == maplookaheadopt );
        havegates = MapMappableGates(future_copy, past_copy, lg, alsoNN2q); // map all easy gates; remainder returned in lg

        if (havegates)
        {
            // DOUT("... ... SelectAlter level=" << level << ", committed + mapped easy gates, now facing " << lg.size() << " 2q gates to evaluate next");
            std::list<Alter> la;                // list that will hold all variations, as returned by GenAlters
            GenAlters(lg, la, past_copy);       // gen all possible variations to make gates in lg NN, in current past.v2r mapping
            // DOUT("... ... SelectAlter level=" << level << ", generated for these 2q gates " << la.size() << " alternatives; RECURSE ... ");
            Alter resa;                         // result alternative selected and returned by next SelectAlter call
            SelectAlter(la, resa, future_copy, past_copy, basePast, level+1); // recurse, best in resa ...
            resa.DPRINT("... ... SelectAlter, generated for these 2q gates ... ; RECURSE DONE; resulting alternative ");
            a.score = resa.score;               // extension of deep recursion is treated as extension at current level,
                                                // by this an alternative started bad may be compensated by deeper alts
        }
        else
        {
            // DOUT("... ... SelectAlter level=" << level << ", no gates to evaluate next; RECURSION BOTTOM");
            auto mapperopt = ql::options::get("mapper");
            if ("maxfidelity" == mapperopt)
            {
                a.score = ql::quick_fidelity(past_copy.lg);
            }
            else
            {
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
    bla.remove_if( [this,gla](const Alter& a) { return a.score != gla.front().score; } );
    Alter::DPRINT("... SelectAlter equally best alternatives on return of RECURSION:", bla);
    resa = ChooseAlter(bla, future);
    resa.DPRINT("... the selected Alter is");
    // DOUT("... SelectAlter level=" << level << " selecting from " << bla.size() << " equally good alternatives above DONE");
    DOUT("SelectAlter DONE level=" << level << " from " << la.size() << " alternatives");
}

// Given the states of past and future
// map all mappable gates and find the non-mappable ones
// for those evaluate what to do next and do it;
// during recursion, comparison is done with the base past (bottom of recursion stack),
// and past is the last past (top of recursion stack) relative to which the mapping is done.
void MapGates(Future& future, Past& past, Past& basePast)
{
    std::list<ql::gate*>   lg;              // list of non-mappable gates taken from avlist, as returned from MapMappableGates
    std::string maplookaheadopt = ql::options::get("maplookahead");
    bool alsoNN2q = ( "noroutingfirst" == maplookaheadopt || "all" == maplookaheadopt );
    while (MapMappableGates(future, past, lg, alsoNN2q))  // returns false when no gates remain
    {
        // all gates in lg are two-qubit quantum gates that cannot be mapped
        // select which one(s) to (partially) route, according to one of the known strategies
        // the only requirement on the code below is that at least something is done that decreases the problem

        // generate all variations
        std::list<Alter> la;                // list that will hold all variations, as returned by GenAlters
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
void MapCircuit(ql::quantum_kernel& kernel, Virt2Real& v2r)
{
    Future  future;         // future window, presents input in avlist
    Past    mainPast;       // past window, contains output schedule, storing all gates until taken out
    Scheduler sched;        // new scheduler instance (from src/scheduler.h) used for its dependence graph

    future.Init(platformp);
    future.SetCircuit(kernel, sched, nq, nc); // constructs depgraph, initializes avlist, ready for producing gates
    kernel.c.clear();       // future has copied kernel.c to private data; kernel.c ready for use by new_gate
    kernelp = &kernel;      // keep kernel to call kernelp->gate() inside Past.new_gate(), to create new gates

    mainPast.Init(platformp, kernelp);  // mainPast and Past clones inside Alters ready for generating output schedules into
    mainPast.ImportV2r(v2r);    // give it the current mapping/state
    // mainPast.DPRINT("start mapping");

    MapGates(future, mainPast, mainPast);
    mainPast.FlushAll();

    // mainPast.DPRINT("end mapping");

    DOUT("... retrieving outCirc from mainPast.outlg; swapping outCirc with kernel.c, kernel.c contains output circuit");
    ql::circuit outCirc;
    mainPast.Out(outCirc);
    kernel.c.swap(outCirc);
    mainPast.ExportV2r(v2r);
    nswapsadded = mainPast.NumberOfSwapsAdded();
    nmovesadded = mainPast.NumberOfMovesAdded();
}

public:

// decompose all gates that have a definition with _prim appended to its name
void MakePrimitives(ql::quantum_kernel& kernel)
{
    DOUT("MakePrimitives circuit ...");

    ql::circuit     input_gatepv = kernel.c;    // copy to allow kernel.c use by Past.new_gate
    kernel.c.clear();                           // kernel.c ready for use by new_gate

    Past            mainPast;                   // output window in which gates are scheduled
    mainPast.Init(platformp, kernelp);

    for( auto & gp : input_gatepv )
    {
        ql::circuit tmpCirc;
        mainPast.MakePrimitive(gp, tmpCirc);
        for (auto newgp : tmpCirc)
        {
            mainPast.AddAndSchedule(newgp);
        }
    }
    mainPast.FlushAll();

    ql::circuit     outCirc;                    // ultimate output gate stream
    mainPast.Out(outCirc);
    kernel.c.swap(outCirc);

    DOUT("MakePrimitives circuit [DONE]");
}   // end MakePrimitives

// map kernel's circuit, main mapper entry once per kernel
void Map(ql::quantum_kernel& kernel)
{
    COUT("Mapping kernel " << kernel.name << " [START]");
    DOUT("... kernel original virtual number of qubits=" << kernel.qubit_count);
    nc = kernel.creg_count;     // in absence of platform creg_count, take it from kernel, i.e. from OpenQL program
    kernelp = NULL;             // no new_gates until kernel.c has been copied

    Virt2Real   v2r;            // current mapping while mapping this kernel

    // unify all incoming v2rs into v2r to compute kernel input mapping;
    // but until inter-kernel mapping is implemented, take program initial mapping for it
    v2r.Init(nq);               // v2r now contains program initial mapping
    v2r.DPRINT("After initialization");

    v2r.Export(v2r_in);  // from v2r to caller for reporting
    v2r.Export(rs_in);   // from v2r to caller for reporting

    std::string initialplaceopt = ql::options::get("initialplace");
    if("no" != initialplaceopt)
    {
#ifdef INITIALPLACE
        std::string initialplace2qhorizonopt = ql::options::get("initialplace2qhorizon");
        DOUT("InitialPlace: kernel=" << kernel.name << " initialplace=" << initialplaceopt << " initialplace2qhorizon=" << initialplace2qhorizonopt << " [START]");
        InitialPlace    ip;             // initial placer facility
        ipr_t           ipok;           // one of several ip result possibilities
        double          iptimetaken;      // time solving the initial placement took, in seconds
        
        ip.Init(&grid, platformp);
        ip.Place(kernel.c, v2r, ipok, iptimetaken, initialplaceopt); // compute mapping (in v2r) using ip model, may fail
        DOUT("InitialPlace: kernel=" << kernel.name << " initialplace=" << initialplaceopt << " initialplace2qhorizon=" << initialplace2qhorizonopt << " result=" << ip.ipr2string(ipok) << " iptimetaken=" << iptimetaken << " seconds [DONE]");
#else // ifdef INITIALPLACE
        DOUT("InitialPlace code commented out; please uncomment #define INITIALPLACE in src/mapper.h when wanted [DONE]");
        WOUT("InitialPlace code commented out; please uncomment #define INITIALPLACE in src/mapper.h when wanted [DONE]");
#endif // ifdef INITIALPLACE
    }
    v2r.DPRINT("After InitialPlace");

    v2r.Export(v2r_ip);  // from v2r to caller for reporting
    v2r.Export(rs_ip);   // from v2r to caller for reporting

    MapCircuit(kernel, v2r);        // updates kernel.c with swaps, maps all gates, updates v2r map
    v2r.DPRINT("After heuristics");

    MakePrimitives(kernel);         // decompose to primitives as specified in the config file

    kernel.qubit_count = nq;        // bluntly copy nq (==#real qubits), so that all kernels get the same qubit_count
    v2r.Export(v2r_out);     // from v2r to caller for reporting
    v2r.Export(rs_out);      // from v2r to caller for reporting

    COUT("Mapping kernel " << kernel.name << " [DONE]");
}   // end Map

// initialize mapper for whole program
// lots could be split off for the whole program, once that is needed
//
// initialization for a particular kernel is separate (in Map entry)
void Init(const ql::quantum_platform *p)
{
    // DOUT("Mapping initialization ...");
    // DOUT("... Grid initialization: platform qubits->coordinates, ->neighbors, distance ...");
    platformp = p;
    nq = p->qubit_number;
    // nc = p->creg_number;  // nc should come from platform, but doesn't; is taken from kernel in Map
    RandomInit();
    // DOUT("... platform/real number of qubits=" << nq << ");
    cycle_time = p->cycle_time;

    grid.Init(platformp);

    // DOUT("Mapping initialization [DONE]");
}   // end Init


};  // end class Mapper

#endif // ifndef QL_MAPPER_H
