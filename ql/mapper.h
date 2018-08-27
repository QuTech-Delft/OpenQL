/**
 * @file   mapper.h
 * @date   07/2018
 * @author Hans van Someren
 * @brief  openql mapping
 */

#ifndef QL_MAPPER_H
#define QL_MAPPER_H

#include "ql/utils.h"
#include "ql/platform.h"
#include "ql/arch/cc_light_resource_manager.h"
#include "ql/gate.h"

// Note on the use of constructors and Init functions for classes of the mapper
// -----------------------------------------------------------------------------
// Several local classes of the mapper have members that require initialization
// using a value that was passed on to the Mapper.Init function as a parameter (i.e. platform, cycle_time).
// Dealing with those initializations in the nested constructors was cumbersome.
// Hence, the constructors create just skeleton objects which need explicit initialization before use.
// Such initialization is provided by a class local Init function when creating a virgin object,
// or by copying an existing object to it.
// Construction of skeleton objects requires the used classes to provide such (non-parameterized) constructors;
// therefore, such a constructor was added to class resource_manager_t in cc_light_resource_manager.h


// =========================================================================================
// Virt2Real: map of a virtual qubit index to its real qubit index
//
// insertion of a swap changes this
// the qubit indices in the QASM that is input to the mapper, are assumed to be virtual
// the mapper inspects two-qubit operations for nearest-neighborship (NN) of the qubits
// and, if needed, inserts swaps to make these NN, while updating this Virt2Real mapping
//
// the main mapping algorithm evaluates multiple paths/ways to make such two qubits NN
// for each of these paths, the Virt2Real map changes and ends-up differently
// so there is a Virt2Real attached to the output (the 'main' one)
// and there is a Virt2Real for each experimental path to make one or more pairs of qubits NN;
// in the latter case, these start off as copy of the main one
class Virt2Real
{
private:

    size_t              nq;                // size of the map; after initialization, will always be the same
    std::vector<size_t> v2rMap;            // v2rMap[virtual qubit index] -> real qubit index


// map real qubit to the virtual qubit index that is mapped to it (i.e. backward map)
// a second vector next to v2rMap (i.e. an r2vMap) would speed this up
size_t GetVirt(size_t r)
{
    for (size_t v=0; v<nq; v++)
    {
        if (v2rMap[v] == r) return v;
    }
    assert(0);
    return 0;
}


public:

// expand to desired size and initialize to trivial (1-1) mapping
void Init(size_t n)
{
    // DOUT("Virt2Real::Init(n=" << n << "), initializing 1-1 mapping");
    nq = n;
    v2rMap.resize(n);
    for (size_t i=0; i<n; i++)
    {
        v2rMap[i] = i;
    }
}

// map virtual qubit index to real qubit index
size_t& operator[] (size_t v)
{
    return v2rMap[v];
}

// r0 and r1 are real qubit indices
// after a swap(r0,r1) gate application their states were exchanged,
// so when v0 was in r0 and v1 was in r1, then v0 is now in r1 and v1 is in r0
// update v2r accordingly
void Swap(size_t r0, size_t r1)
{
    size_t v0 = GetVirt(r0);
    size_t v1 = GetVirt(r1);
    // DOUT("... swap virtual indices from ("<< v0<<"->"<<r0<<","<<v1<<"->"<<r1<<") to ("<<v0<<"->"<<r1<<","<<v1<<"->"<<r0<<" )");
    v2rMap[v0] = r1;
    v2rMap[v1] = r0;
}

void Print(std::string s)
{
    std::cout << "... Virt2Real(v->r) " << s << ":";
    for (size_t v=0; v<nq; v++)
    {
        size_t r = v2rMap[v];
        std::cout << " (" << v << "->" << r << ")";
    }
    std::cout << std::endl;
#ifdef debug
    std::cout << "... real2virt(r->v) " << s << ":";
    for (size_t r=0; r<nq; r++)
    {
        size_t v = GetVirt(r);
        std::cout << " (" << r << "->" << v << ")";
    }
    std::cout << std::endl;
#endif        // debug
}

};  // end class Virt2Real





// =========================================================================================
// FreeCycle: map for each real qubit to the first cycle that it is free to use
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

    size_t                  nq;      // size of the map; after initialization, will always be the same
    size_t                  ct;      // multiplication factor from cycles to nano-seconds (unit of duration)
    ql::quantum_platform   *platformp;   // with resource descriptions for scheduling
    std::vector<size_t>     fcv;     // fcv[real qubit index i]: qubit i is free from this cycle on
    ql::arch::resource_manager_t rm; // actual resources occupied by scheduled gates


// access free cycle value of qubit i
size_t& operator[] (size_t i)
{
    return fcv[i];
}

public:

void Init(size_t n, size_t c, ql::quantum_platform *p)
{
    // DOUT("FreeCycle::Init(n=" << n << ", c=" << c << "), initializing to all 0 cycles");
    ql::arch::resource_manager_t lrm(*p);   // allocated here and copied below to rm because of platform parameter
    // DOUT("... created local resource manager");
    nq = n;
    ct = c;
    platformp = p;
    fcv.resize(n, 1);   // this 1 implies that cycle of first gate will be 1 and not 0; OpenQL convention!?!?
    // DOUT("... about to copy local resource manager to FreeCycle member rm");
    rm = lrm;
    // DOUT("... done copy local resource manager to FreeCycle member rm");
}

#ifdef used
// depth of the FreeCycle map
// equals the max of all entries minus the min of all entries
// not used yet; would be used to compute the max size of a top window on the past
size_t Depth()
{
    size_t  minFreeCycle = SIZE_MAX;
    size_t  maxFreeCycle = 0;
    for (auto& v : fcv)
    {
        if (v < minFreeCycle)
        {
            minFreeCycle = v;
        }
        if (maxFreeCycle < v)
        {
            maxFreeCycle = v;
        }
    }
    return maxFreeCycle - minFreeCycle;
}
#endif

// max of the FreeCycle map
// equals the max of all entries
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

void Print(std::string s)
{
    std::cout << "... FreeCycle " << s << ":";
    for (size_t i=0; i<nq; i++)
    {
        size_t v = fcv[i];
        std::cout << " " << v;
    }
    std::cout << std::endl;
    // rm.Print("... in FreeCycle: ");
}

static void GetGateParameters(std::string id, ql::quantum_platform *platformp, std::string& operation_name, std::string& operation_type, std::string& instruction_type)
{
    if ( !platformp->instruction_settings[id]["cc_light_instr"].is_null() )
    {
        operation_name = platformp->instruction_settings[id]["cc_light_instr"];
    }
    if ( !platformp->instruction_settings[id]["type"].is_null() )
    {
        operation_type = platformp->instruction_settings[id]["type"];
    }
    if ( !platformp->instruction_settings[id]["cc_light_instr_type"].is_null() )
    {
        instruction_type = platformp->instruction_settings[id]["cc_light_instr_type"];
    }
}

// when we would schedule gate g, what would be its start cycle? return it
// gate operands are real qubit indices
// is purely functional, doesn't affect state
size_t StartCycle(ql::gate *g)
{
    auto&       id = g->name;
    std::string operation_name(id);
    std::string operation_type;
    std::string instruction_type;

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
    
    size_t      duration = (g->duration+ct-1)/ct;   // rounded-up unsigned integer division
    auto        mapopt = ql::options::get("mapper");
    if (mapopt == "minextendrc")
    {
        size_t  baseStartCycle = startCycle;
        GetGateParameters(id, platformp, operation_name, operation_type, instruction_type);
	    while (startCycle < SIZE_MAX)
	    {
	        if (rm.available(startCycle, g, operation_name, operation_type, instruction_type, duration))
	        {
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
    assert (startCycle < SIZE_MAX);

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

// schedule gate g in the FreeCycle and resource map
// gate operands are real qubit indices
// both the FreeCycle map and the resource map are updated
void Add(ql::gate *g, size_t startCycle)
{
    AddNoRc(g, startCycle);

    auto        mapopt = ql::options::get("mapper");
    if (mapopt == "minextendrc")
    {
	    auto&       id = g->name;
	    std::string operation_name(id);
	    std::string operation_type;
	    std::string instruction_type;
	    size_t      duration = (g->duration+ct-1)/ct;   // rounded-up unsigned integer division

        GetGateParameters(id, platformp, operation_name, operation_type, instruction_type);
        rm.reserve(startCycle, g, operation_name, operation_type, instruction_type, duration);
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
// Implementation notes:
//
// 1. list of gates copying
// The implementation below copies the list of gates from the main Past upon creation of a temporary Past
// this is easiest in implementation but probably a waste of effort because FreeCycle is sufficient
// (and not adding gates to lg in the temporary Past) to compute the path's circuit latency extension;
// but when adding hardware constraint aware scheduling to this Past, it might be needed
// (since not only the starting cycle of each gate is needed but also the hardware resources it occupies);
// so keep this list of gate copying on cloning Past in until this constraint aware scheduling is added.
// For each of the gates in lg, cycle maintains its start cycle value; the same considerations
// as for the list of gates lg apply to this cycle map.
//
// 2. not windowing
// The implementation below keeps past gates in the list of gates until the end of the circuit is reached.
// Only then Past' gates are flushed to the output stream.
// The size of the Past could be limited (on number of gates or cycle difference between start and end)
// and so be made a window on the output stream, regularly flushing Past to the output stream.
// Then the overhead of the list of gates and the cycle map could be reduced to constant.
// Experimentation has shown that e.g. a max cycle difference of 100 where swaps take 10 cycles,
// is a sufficiently large window to limit the reduction in performance of the mapper;
// when scheduling a gate (or swap) in the past, for optimal performance it should never be
// at the start of the list (i.e. at a cycle number before the first cycle of the current Past)
// because then the Past window would be too small and scheduling would be negatively impacted.
// It is sufficient when the first cycle of Past is smaller/equal than the minimum value in FreeCycle.
class Past
{
private:

	size_t                  nq;         // width of Past in qubits
	size_t                  ct;         // cycle time, multiplier from cycles to nano-seconds
    ql::quantum_platform   *platformp;  // platform describing resources for scheduling
	Virt2Real               v2r;        // Virt2Real map applying to this Past
	FreeCycle               fc;         // FreeCycle map applying to this Past

	typedef ql::gate *      gate_p;
	std::list<gate_p>       waitinglg;  // list of gates in this Past, top order, waiting to be scheduled in
	std::list<gate_p>       lg;         // list of gates in this Past, scheduled by their (start) cycle values
	std::map<gate_p,size_t> cycle;      // gate to cycle map, startCycle value of each past gatecycle[gp]
    ql::circuit             *outCircp;  // output stream after past

public:

// past initializer sets nq and ct and initializes all (default-constructed) composite members
void Init(size_t n, size_t c, ql::quantum_platform *pf)
{
    // DOUT("past::Init(n=" << n << ", c=" << c << ") ");
    nq = n;
    ct = c;
    platformp = pf;
    v2r.Init(n);
    fc.Init(n,c,pf);
    // waitinglg is initialized to empty list
    // lg is initialized to empty list
    // cycle is initialized to empty map
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

void Output(ql::circuit& ct)
{
    outCircp = &ct;
}

// all gates in past.waitinglg are scheduled into past.lg
// note that these gates all are mapped and so have real operand qubit indices
// the FreeCycle map reflects for each qubit the first free cycle
// all new gates, now in waitinglist, get such a cycle assigned below, increased gradually, until definitive
void Schedule()
{
    // DOUT("Schedule ...");

    assert (!waitinglg.empty());

    do
    {
        size_t      startCycle = SIZE_MAX;
        gate_p      gp;

        // find the gate with the minimum startCycle
        //
        // IMPORTANT: this assumes that the waitinglg gates list is in topological order,
        // which is ok because the pair of swap lists use distict qubits and
        // the gates of each are added to the back of the list in the order of execution.
        // So, using AddNoRc, the tryfc (try FreeCycle map) reflects the earliest startCycle per qubit,
        // and so dependences are respected, so we can find the gate that can start first ...
        // This is really a hack to avoid the construction of a dependence graph.
        // We should use a copy of fc and not fc itself, since the latter reflects the really scheduled gates.
        FreeCycle   tryfc = fc;
        for (auto & trygp : waitinglg)
        {
            size_t tryStartCycle = tryfc.StartCycle(trygp);
            tryfc.AddNoRc(trygp, tryStartCycle);

            if (tryStartCycle < startCycle)
            {
                startCycle = tryStartCycle;
                gp = trygp;
            }
        }

        // add this gate to the maps, scheduling the gate (doing the cycle assignment)
	    // DOUT("... add " << gp->qasm() << " startcycle=" << startCycle << " cycles=" << ((gp->duration+ct-1)/ct) );
	    fc.Add(gp, startCycle);
	    cycle[gp] = startCycle;
        gp->cycle = startCycle;
	
	    // insert gate in lg, the list of gates, in cycle order, and in this order, as late as possible
	    //
	    // reverse iterate because the insertion is near the end of the list
	    // insert so that cycle values are in order afterwards and the new one is nearest to the end
	    std::list<gate_p>::reverse_iterator rigp = lg.rbegin();
	    for (; rigp != lg.rend(); rigp++)
	    {
	        if (cycle[*rigp] <= startCycle)
	        {
	            // base because insert doesn't work with reverse iteration
	            // rigp.base points after the element that rigp is pointing at
	            // which is luckly because insert only inserts before the given element
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
	
        // and remove it then from the waiting list
        waitinglg.remove(gp);
    }
    while (!waitinglg.empty());

    // Print("Schedule:");
}

// add the mapped gate to the current past's waiting list
void Add(gate_p gp)
{
    waitinglg.push_back(gp);
}

ql::custom_gate* NewCustomGate(std::string gname, std::vector<size_t> qubits, size_t duration=0, double angle=0.0)
{
    ql::custom_gate* g;

    // DOUT("NewCustomGate " << gname);
    // first check if a specialized custom gate is available
    std::string instr = gname + " ";
    if(qubits.size() > 0)
    {
        for (size_t i=0; i<(qubits.size()-1); ++i)
            instr += "q" + std::to_string(qubits[i]) + ",";
        if(qubits.size() >= 1) // to make if work with gates without operands
            instr += "q" + std::to_string(qubits[qubits.size()-1]);
    }

    ql::instruction_map_t        gate_definition = platformp->instruction_map;
    // DOUT("... NewCustomGate find " << instr);
    std::map<std::string,ql::custom_gate*>::iterator it = gate_definition.find(instr);
    if (it != gate_definition.end())
    {
        g = new ql::custom_gate(*(it->second));
        for(auto & qubit : qubits)
            g->operands.push_back(qubit);
        if(duration>0) g->duration = duration;
        g->angle = angle;
        // DOUT("... NewCustomGate find instr DONE");
    }
    else
    {
        // otherwise, check if there is a parameterized custom gate (i.e. not specialized for arguments)
        // DOUT("... NewCustomGate find parameterized " << gname);
        it = gate_definition.find(gname);
        if (it != gate_definition.end())
        {
            g = new ql::custom_gate(*(it->second));
            for(auto & qubit : qubits)
                g->operands.push_back(qubit);
            if(duration>0) g->duration = duration;
            g->angle = angle;
            // DOUT("... NewCustomGate find parameterized instr DONE");
        }
        else
        {
            EOUT("FATAL: in mapper, custom gate not added for " << gname);
            assert(0);
            g = nullptr;
        }
    }
    // DOUT("NewCustomGate " << gname << " DONE");
    return g;
}

// create a new y90
ql::custom_gate* NewY90(size_t r0)
{
    ql::custom_gate*  gp;
    // DOUT("... new ry90(q" << r0 << ")");
    std::vector<size_t> qubits; qubits.resize(1); qubits[0] = r0;
    gp = NewCustomGate("ry90", qubits);
    // DOUT("... done ry90(q" << r0 << ") with duration=" << gp->duration << ", cycles=" << ((gp->duration+ct-1)/ct) );
    return gp;
}

// create a new ym90
ql::custom_gate* NewYm90(size_t r0)
{
    ql::custom_gate*  gp;
    // DOUT("... ym90(q" << r0 << ")");
    std::vector<size_t> qubits; qubits.resize(1); qubits[0] = r0;
    gp = NewCustomGate("ym90", qubits);
    // DOUT("... done ym90(q" << r0 << ") with duration=" << gp->duration << ", cycles=" << ((gp->duration+ct-1)/ct) );
    return gp;
}

// create a new cz
ql::custom_gate* NewCz(size_t r0, size_t r1)
{
    ql::custom_gate*  gp;
    // DOUT("... cz(q" << r0 << ",q" << r1 << ")");
    std::vector<size_t> qubits; qubits.resize(2); qubits[0] = r0; qubits[1] = r1;
    gp = NewCustomGate("cz", qubits);
    // DOUT("... done cz(q" << r0 << ",q" << r1 << ") duration=" << gp->duration << ", cycles=" << ((gp->duration+ct-1)/ct) );
    return gp;
}

// generate a single swap with real operands and add it to the current past's waiting list
void AddSwap(size_t r0, size_t r1)
{
    gate_p  gp;
    // DOUT("... adding swap(q" << r0 << ",q" << r1 << ") ... " );

    gp = NewYm90(r1);  Add(gp);
    gp = NewCz(r0,r1); Add(gp);
    gp = NewY90(r1);   Add(gp);

    gp = NewYm90(r0);  Add(gp);
    gp = NewCz(r1,r0); Add(gp);
    gp = NewY90(r0);   Add(gp);

    gp = NewYm90(r1);  Add(gp);
    gp = NewCz(r0,r1); Add(gp);
    gp = NewY90(r1);   Add(gp);

    // new ql::swap(r0,r1); gp->duration = 400; Add(gp);
    // DOUT("... swap(q" << r0 << ",q" << r1 << ") with duration=" << gp->duration << ", cycles=" << ((gp->duration+ct-1)/ct) );

    v2r.Swap(r0,r1);
}

// add the mapped gate (with real qubit indices as operands) to the past
// by adding it to the waitinglist and scheduling it into the past
void AddAndSchedule(gate_p gp)
{
    Add(gp);
    Schedule();
}

size_t Map(size_t v)
{
    return v2r[v];
}

size_t MaxFreeCycle()
{
    return fc.Max();
}

void Flush()
{
    for( auto & gp : lg )
    {
        outCircp->push_back(gp);
    }
}

};  // end class Past


// =========================================================================================
// NNPath: one alternative way to make two real qbits (operands of a 2-qubit gate) nearest neighbor (NN);
// of these two qubits, the first qubit is called the source, the second is called the target qubit.
// The NNPath stores a series of real qubit indices; qubits/indices are equivalent to the nodes in the grid.
// A path represents a path through the grid from source to target qubit, with each hop between
// qubits/nodes only between neighboring nodes in the grid; the intention is that all but one hops
// translate into swaps and that one hop remains that will be the place to do the 2-qubit gate.
//
// Actually, the NNPath goes through several stages:
// - first, while finding a path from source to target, the current path is kept in total;
//   fromSource, fromTarget, past and cycleExtend are not used; past is a clone of the main past
// - paths are found starting from the source node, and aiming to reach the target node,
//   each time adding one additional hop to the path
//   fromSource, fromTarget, and cycleExtend are still empty and not used
// - each time another continuation of the path is found, the current NNPath is cloned
//   and the difference continuation represented in the total attribute; it all starts with an empty NNPath
//   fromSource, fromTarget, and cycleExtend are still empty and not used
// - once all alternative total paths from source to target have been found
//   each of these is split again in all possible ways (to ILP overlap swaps from source and target);
//   the split is the place where the two-qubit gate is put
// - the alternative splits are made separate Paths and for each
//   of these the two partial paths are stored in fromSource and fromTarget;
//   a partial path stores its starting and end nodes (so contains 1 hop less than its length);
//   the partial path of the target operand is reversed, so starts at the target qubit
// - then we add swaps to past following the recipee in fromSource and fromTarget; this extends past;
//   also we compute cycleExtend as the latency extension caused by these swaps
//
// At the end, we have a list of Paths, each with a private Past, and a private latency extension.
// The partial paths represent lists of swaps to be inserted.
// The initial two-qubit gate gets the qubits at the ends of the partial paths as operands.
// The main selection criterium from the Paths is to select the one with the minimum latency extension.
// Having done that, the other Paths can be discarded and the selected one committed to the main Past.
class NNPath
{

private:

	size_t                  nq;         // width of Past in qubits
	size_t                  ct;         // cycle time, multiplier from cycles to nano-seconds
    ql::quantum_platform   *platformp;  // descriptions of resources for scheduling

    std::vector<size_t>     total;      // full path, including source and target nodes
    std::vector<size_t>     fromSource; // partial path after split, starting at source
    std::vector<size_t>     fromTarget; // partial path after split, starting at target, backward

    Past                    past;       // cloned main past, extended with swaps from this path
    size_t                  cycleExtend;// latency extension caused by the path

// printing facilities of Paths
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
                std::cout << hd << "[" << pp.size() << "]=[";
            }
            else
            {
                std::cout << "->";
            }
            std::cout << ppe;
        }
        if (started == 1)
        {
            std::cout << "]" << std::endl;
        }
    }
}

public:

void Print(std::string s)
{
    std::cout << s;
    if (cycleExtend != SIZE_MAX)
    {
        std::cout << ": cycleExtend=" << cycleExtend << std::endl;
    }
    partialPrint("\ttotal path", total);
    partialPrint("\tpath from source", fromSource);
    partialPrint("\tpath from target", fromTarget);
    // past.Print("past in Path");
}

static
void listPrint(std::string s, std::list<NNPath> & lp)
{
    int started = 0;
    for (auto & p : lp)
    {
        if (started == 0)
        {
            started = 1;
            std::cout << s << "[" << lp.size() << "]={" << std::endl;
        }
        p.Print("");
    }
    if (started == 1)
    {
        std::cout << "}" << std::endl;
    }
}

// NNPath initializer
// This should only be called after a first constructor and not after cloning a path.
// It sets nq and ct and initializes all (default-constructed) composite members.
// Init is used because making the default constructor parameterized on n and c and
// passing these on to the constructors of the members, is not doable.
// So a default constructed NNPath should be followed by an Init on it.
void Init(size_t n, size_t c, ql::quantum_platform* p)
{
    // DOUT("path::Init(n=" << n << ", c=" << c << ") ");
    nq = n;
    ct = c;
    platformp = p;
    // total, fromSource and fromTarget start as empty vectors
    past.Init(n,c,p);           // initializes past to empty
    cycleExtend = SIZE_MAX;     // means undefined
}

// create a single node (i.e. distance 0) path consisting of just the qubit q
void Single(size_t q)
{
    // total.resize(1);
    // total[0] = q;
    total.insert(total.begin(), q); // hopelessly inefficient
}

// add a node to the path in front, extending its length with one
void Add(size_t q)
{
    total.insert(total.begin(), q); // hopelessly inefficient
}

// add swap gates for the current path to the given past
// this past can be a path-local one or the main past
void AddSwaps(Past & past)
{
    size_t  fromQ;
    size_t  toQ;

    fromQ = fromSource[0];
    for ( size_t i = 1; i < fromSource.size(); i++ )
    {
        toQ = fromSource[i];
        past.AddSwap(fromQ, toQ);
        fromQ = toQ;
    }
    fromQ = fromTarget[0];
    for ( size_t i = 1; i < fromTarget.size(); i++ )
    {
        toQ = fromTarget[i];
        past.AddSwap(fromQ, toQ);
        fromQ = toQ;
    }
}

// compute cycle extension of the path relative to the given basePast
// do this by adding the swaps described by the path to a local copy of the past and compare cycles
// store the extension relative to the base in cycleExtend and return it
size_t Extend(Past basePast)
{
    past = basePast;   // explicitly a path-local copy of this basePast!
    AddSwaps(past);
    past.Schedule();
    cycleExtend = past.MaxFreeCycle() - basePast.MaxFreeCycle();
    return cycleExtend;
}

// split the path
// starting from the representation in the total attribute,
// generate all split path variations where each path is split once at any hop in it
// the intention is that the mapped two-qubit gate can be placed at the position of that hop
// all result paths are added to the given result list
void Split(std::list<NNPath> & reslp)
{
    // DOUT("Split ...");

    size_t length = total.size();
    assert (length >= 3);   // distance > 1 so path at least: source -> intermediate -> target
    for (size_t leftopi = 0; leftopi < length-1; leftopi++)
    {
        // DOUT("... leftopi=" << leftopi);
        // leftopi is the index in total that holds the qubit that becomes the left operand of the gate
        // fromSource will contain the path with qubits at indices 0 to leftopi
        // fromTarget will contain the path with qubits at indices leftopi+1 to length-1, reversed
        //      reversal of fromTarget is done since swaps need to be generated starting at the target

        NNPath    np;
        np = *this;            // np is local copy of the current path, including total
        // np.Print("... copy of current path");

        size_t fromi, toi;

        np.fromSource.resize(leftopi+1);
        // DOUT("... fromSource size=" << np.fromSource.size());
        for (fromi = 0, toi = 0; fromi <= leftopi; fromi++, toi++)
        {
            // DOUT("... fromSource: fromi=" << fromi << " toi=" << toi);
            np.fromSource[toi] = np.total[fromi];
        }

        np.fromTarget.resize(length-leftopi-1);
        // DOUT("... fromTarget size=" << np.fromTarget.size());
        for (fromi = length-1, toi = 0; fromi > leftopi; fromi--, toi++)
        {
            // DOUT("... fromTarget: fromi=" << fromi << " toi=" << toi);
            np.fromTarget[toi] = np.total[fromi];
        }

        // np.Print("... copy of path after split");
        reslp.push_back(np);
        // DOUT("... added to result list");
        // Print("... current path after split");
    }
}

};  // end class NNPath



// =========================================================================================
// Mapper: map operands of gates and insert swaps so that two-qubit gate operands are NN
// all gates must be unary or two-qubit gates
//
// Do this mapping in the context of a grid of qubits defined by the given platform.
// Maintain several local mappings to ease navigating in the grid; these are constant after initialization.
//
// The Mapper's main entry is MapCircuit which manages the input and output streams of QASM instructions,
// selects the quantum gates from it, and maps these in the context of what was mapped before (the Past).
// Each gate is separately mapped in MapGate in the main Past's context.
class Mapper
{
private:

                                        // OpenQL wide configuration, all constant after initialization
    size_t nqbits;                      // number of qubits in the system
    size_t cycle_time;                  // length in ns of a single cycle;
                                        // is divisor of duration in ns to convert it to cycles
    ql::quantum_platform platform;      // current platform: topology and gates' duration

                                        // Grid configuration, all constant after initialization
                                        // could be factored out into a separate class
    size_t nx;                          // length of x dimension (x coordinates count 0..nx-1)
    size_t ny;                          // length of y dimension (y coordinates count 0..ny-1)
    std::map<size_t,size_t> x;          // x[i] is x coordinate of qubit i
    std::map<size_t,size_t> y;          // y[i] is y coordinate of qubit i
    typedef std::list<size_t> neighbors_t;  // neighbors is a list of qubits
    std::map<size_t,neighbors_t> nbs;   // nbs[i] is list of neighbor qubits of qubit i

                                        // Mapper dynamic state
    Past   mainPast;                    // main past window; all path alternatives start off as clones of it


// initialize mapper internal grid maps from configuration
// this remains constant over multiple kernels on the same platform
void GridInit()
{
    nx = platform.topology["x_size"];
    ny = platform.topology["y_size"];
    // DOUT("... nx=" << nx << "; ny=" << ny);

    for (auto & aqbit : platform.topology["qubits"] )
    {
        size_t qi = aqbit["id"];
        size_t qx = aqbit["x"];
        size_t qy = aqbit["y"];
        x[qi] = qx;
        y[qi] = qy;

        // sanity checks
        if ( !(0<=qi && qi<nqbits) )
        {
            EOUT(" qbit in platform topology with id=" << qi << " has id that is not in the range 0..nqbits-1 with nqbits=" << nqbits);
            throw ql::exception("Error: qbit with unsupported id.", false);
        }
        else if ( !(0<=qx && qx<nx) )
        {
            EOUT(" qbit in platform topology with id=" << qi << " has x that is not in the range 0..x_size-1 with x_size=" << nx);
            throw ql::exception("Error: qbit with unsupported x.", false);
        }
        else if ( !(0<=qy && qy<ny) )
        {
            EOUT(" qbit in platform topology with id=" << qi << " has y that is not in the range 0..y_size-1 with y_size=" << ny);
            throw ql::exception("Error: qbit with unsupported y.", false);
        }
    }
    for (auto & anedge : platform.topology["edges"] )
    {
        size_t es = anedge["src"];
        size_t ed = anedge["dst"];

        // sanity checks
        if ( !(0<=es && es<nqbits) )
        {
            EOUT(" edge in platform topology has src=" << es << " that is not in the range 0..nqbits-1 with nqbits=" << nqbits);
            throw ql::exception("Error: edge with unsupported src.", false);
        }
        if ( !(0<=ed && ed<nqbits) )
        {
            EOUT(" edge in platform topology has dst=" << ed << " that is not in the range 0..nqbits-1 with nqbits=" << nqbits);
            throw ql::exception("Error: edge with unsupported dst.", false);
        }
        nbs[es].push_back(ed);
    }

#ifdef debug
    for (size_t i=0; i<nqbits; i++)
    {
        DOUT("qubit[" << i << "]: x=" << x[i] << "; y=" << y[i]);
        std::cout << "... connects to ";
        for (auto & n : nbs[i])
        {
            std::cout << n << " ";
        }
        std::cout << std::endl;
        std::cout << "... distance(" << i << ",j)=";
        for (size_t j=0; j<nqbits; j++)
        {
            std::cout << distance(i,j) << " ";
        }
        std::cout << std::endl;
    }
#endif        // debug
}

// distance between two qubits
// implementation is for "cross" and "star" grids and assumes bidirectional edges and convex grid;
// for "plus" grids, replace "std::max" by "+"
size_t GridDistance(size_t from, size_t to)
{
    return std::max(
               std::abs( ptrdiff_t(x[to]) - ptrdiff_t(x[from]) ),
               std::abs( ptrdiff_t(y[to]) - ptrdiff_t(y[from]) ));
}

// initial path finder
// generate all paths with source src and target tgt as a list of path into reslp
// this result list reslp is allocated by caller and is empty on the call
void GenShortestPaths(size_t src, size_t tgt, std::list<NNPath> & reslp)
{
    std::list<NNPath> genlp;    // list that will get the result of a recursive Gen call

    // DOUT("GenShortestPaths: " << "src=" << src << " tgt=" << tgt);
    assert (reslp.empty());

	if (src == tgt) {
		// found target
        // create a virgin path and initialize it to become an empty path
		// add src to this path (so that it becomes a distance 0 path with one qubit, src)
        // and add the path to the result list 
        // DOUT("GenShortestPaths ... about to allocate local virgin path");
        NNPath  p;

        // DOUT("... done allocate local virgin path; now init it");
        p.Init(nqbits, cycle_time, &platform);
        // DOUT("... done init path; now add src to it, converting it to an empty path");
        p.Add(src);
        // DOUT("... done adding src; now add this empty path to result list");
        // p.Print("... path before adding to result list");
        reslp.push_back(p);
        // DOUT("... done adding empty path to result list; will print the list now");
        // p.Print("... path after adding to result list");
        // NNPath::listPrint("... result list after adding path", reslp);
        // DOUT("... will return now");
        return;
	}

	// start looking around at neighbors for serious paths
    // assume that distance is not approximate but exact and can be met
	size_t d = GridDistance(src, tgt);
	assert (d >= 1);

    // loop over all neighbors of src
    for (auto & n : nbs[src])
    {
		size_t dn = GridDistance(n, tgt);

		if (dn >= d)
        {
            // not closer, looking for shortest path, so ignore this neighbor
			continue;
		}

		// get list of all possible paths from n to tgt in genlp
		GenShortestPaths(n, tgt, genlp);
        // DOUT("... GenShortestPaths, returning from recursive call in: " << "src=" << src << " tgt=" << tgt);

		// accumulate all results
        reslp.splice(reslp.end(), genlp);   // moves all of genlp to reslp; makes genlp empty
        // DOUT("... did splice, i.e. moved any results from recursion to current level results");
        assert (genlp.empty());
	}
    // reslp contains all paths starting from a neighbor of src, to tgt

    // add src to front of all to-be-returned paths from src's neighbors to tgt
    for (auto & p : reslp)
    {
        // DOUT("... GenShortestPaths, about to add src=" << src << "in front of path");
        p.Add(src);
    }
    // DOUT("... GenShortestPaths, returning from call of: " << "src=" << src << " tgt=" << tgt);
}

// split each path in the argument old path list
// this gives all variations to put the two-qubit gate in the path
// all possible paths are returned in the result list reslp
void GenSplitPaths(std::list<NNPath> & oldlp, std::list<NNPath> & reslp)
{
    // DOUT("GenSplitPaths");
    for (auto & p : oldlp)
    {
        p.Split(reslp);
    }
    // NNPath::listPrint("... after GenSplitPaths", reslp);
}

// return path from list of paths with minimal cycle extension of mainPast
void MinimalExtendingPath(std::list<NNPath>& lp, NNPath & resp)
{
    size_t  minExtension = SIZE_MAX;
    NNPath  minp;

    // DOUT("MinimalExtendingPath");
    assert (!lp.empty());   // so there always is a result path

    for (auto & p : lp)
    {
        size_t extension = p.Extend(mainPast);
        if (extension < minExtension)
        {
            minExtension = extension;
            minp = p;
        }
    }
    resp = minp;
    // NNPath::listPrint("... after MinimalExtendingPath", lp);
}

// find the minimally extending shortest path and use it to generate swaps
void MapMinExtend(ql::gate* gp, size_t src, size_t tgt)
{
    size_t d = GridDistance(src, tgt);
    assert (d >= 1);
    DOUT("... MapMinExtend: " << gp->qasm() << " in real (q" << src << ",q" << tgt << ") at distance=" << d );

    if (d > 1)
    {
        std::list<NNPath> genlp;    // list that will hold all paths
        std::list<NNPath> splitlp;  // list that will hold all split paths
        NNPath resp;                // path in splitlp that minimally extends mainPast

        GenShortestPaths(src, tgt, genlp);       // find all shortest paths from src to tgt
        // NNPath::listPrint("... after GenShortestPaths", genlp);

        GenSplitPaths(genlp, splitlp);   // 2q gate can be put anywhere in each path
        MinimalExtendingPath(splitlp, resp);// from all these, find path that minimally extends mainPast

        resp.Print("... the minimally extending path with swaps is");
        resp.AddSwaps(mainPast);    // add swaps, as described by resp, to mainPast
        mainPast.Schedule();        // and schedule them in
    }
}

// find one (first) shortest path and use it to generate swaps
void MapBase(ql::gate* gp, size_t src, size_t tgt)
{
    size_t d = GridDistance(src, tgt);
    DOUT("... MapBase: " << gp->qasm() << " in real (q" << src << ",q" << tgt << ") at distance=" << d );
    while (d > 1)
    {
        for( auto & n : nbs[src] )
        {
            size_t dnb = GridDistance(n, tgt);
            if (dnb < d)
            {
                // DOUT(" ... distance(real " << n << ", real " << tgt << ")=" << dnb);
                mainPast.AddSwap(src, n);
                mainPast.Schedule();
                // mainPast.Print("mapping after swap");
                src = n;
                break;
            }
        }
        d = GridDistance(src, tgt);
        // DOUT(" ... new distance(real " << src << ", real " << tgt << ")=" << d);
    }
}

// map the operands of a single gate
// if necessary, insert swaps
void MapGate(ql::gate* gp)
{
    auto& q = gp->operands;
    size_t operandCount = q.size();

    DOUT("MapGate: " << gp->qasm() );
    if (operandCount == 1)
    {
        q[0] = mainPast.Map(q[0]);
        //DOUT(" ... mapped gate: " << gp->qasm() );
        mainPast.AddAndSchedule(gp);
    }
    else if (operandCount == 2)
    {
        size_t rq0 = mainPast.Map(q[0]);
        size_t rq1 = mainPast.Map(q[1]);
        
        auto mapopt = ql::options::get("mapper");
        if (mapopt == "base")
        {
            MapBase(gp, rq0, rq1);
        }
        else if (mapopt == "minextend" || mapopt == "minextendrc")
        {
            MapMinExtend(gp, rq0, rq1);
        }
        else
        {
            assert(0);  // see options.h
        }
        q[0] = mainPast.Map(q[0]);
        q[1] = mainPast.Map(q[1]);
        DOUT(" ... mapped gate: " << gp->qasm() );
        mainPast.AddAndSchedule(gp);
    }
    else
    {
        EOUT(" gate: " << gp->qasm() << " has more than 2 operand qubits; please decompose such gates first before mapping.");
        throw ql::exception("Error: gate with more than 2 operand qubits; please decompose such gates first before mapping.", false);
    }
}

public:

// alternative bundler using gate->cycle attribute instead of lemon's cycle map
// it assumes that the gate->cycle attribute reflect the cycle assignment of a particular schedule
ql::ir::bundles_t Bundler(ql::circuit& inCirc)
{
    ql::ir::bundles_t bundles;

    typedef std::vector<ql::gate*> insInOneCycle;
    std::map<size_t,insInOneCycle> insInAllCycles;

    DOUT("Bundler ...");
    size_t TotalCycles = 0;
    for ( auto & gp : inCirc)
    {
        if( gp->type() != ql::gate_type_t::__wait_gate__ )
        {
            insInAllCycles[gp->cycle].push_back( gp );
            TotalCycles = std::max(TotalCycles, gp->cycle);
        }
    }

    for(size_t currCycle=0; currCycle<=TotalCycles; ++currCycle)
    {
        auto it = insInAllCycles.find(currCycle);
        ql::ir::bundle_t abundle;
        abundle.start_cycle = currCycle;
        size_t bduration = 0;
        if( it != insInAllCycles.end() )
        {
            auto nInsThisCycle = insInAllCycles[currCycle].size();
            for(size_t i=0; i<nInsThisCycle; ++i )
            {
                ql::ir::section_t asec;
                auto & ins = insInAllCycles[currCycle][i];
                asec.push_back(ins);
                abundle.parallel_sections.push_back(asec);
                size_t iduration = ins->duration;
                bduration = std::max(bduration, iduration);
            }
            abundle.duration_in_cycles = (bduration+cycle_time-1)/cycle_time; 
            bundles.push_back(abundle);
        }
    }

    DOUT("Bundler [DONE]");
    return bundles;
}

// Mapper constructor initializes constant program-wide data, e.g. grid related
Mapper( size_t nq, ql::quantum_platform& p) :
    nqbits(nq), cycle_time(p.cycle_time), platform(p)
{
    // DOUT("==================================");
    // DOUT("Mapper creation ...");
    // DOUT("... nqbits=" << nqbits << ", cycle_time=" << cycle_time);
    // DOUT("... Grid initialization: qubits->coordinates, ->neighbors, distance ...");

    GridInit();

    // DOUT("Mapper creation [DONE]");
}

// initialize program-wide data that is passed around between kernels
// initial program-wide mapping could be computed here
void MapInit()
{
    // DOUT("Mapping initialization ...");
    // DOUT("... Initialize map(virtual->real)");
    // DOUT("... with trivial mapping (virtual==real), nqbits=" << nqbits);
    mainPast.Init(nqbits, cycle_time, &platform);
    //mainPast.Print("initial mapping");
    // DOUT("Mapping initialization [DONE]");
}

// map kernel's circuit in current mapping context as left by initialization and earlier kernels
void MapCircuit(ql::circuit& inCirc)
{
    DOUT("==================================");
    DOUT("Mapping circuit ...");
    // mainPast.Print("start mapping");

    ql::circuit outCirc;        // output gate stream, mapped; will be swapped with inCirc on return
    mainPast.Output(outCirc);       // past window will flush into outCirc

    for( auto & gp : inCirc )
    {
        // Currently a gate can only be a quantum gate,
        // but an embedded wait or classical instruction should be handled here as well.
        // When so, the past should be flushed first before these are appended to outCirc;
        // the past only contains quantum gates.
        // Note that some classical instructions might refer to a qubit; that should also be mapped!

        // if (*gp is a quantum gate) ...
        // {
            MapGate(gp);
        // }
        // else
        // {
        //  mainPast.Flush();
        //  deal with *gp
        // }
    }
    mainPast.Flush();

    // mainPast.Print("end mapping");

    // replace inCirc (the IR's main circuit) by the newly computed outCirc
    // outCirc gets the old inCirc and is destroyed when leaving scope
    // DOUT("... swapping outCirc with inCirc");
    inCirc.swap(outCirc);

    // DOUT("... Start circuit (size=" << inCirc.size() << ") after mapping:");
    // for( auto& g : inCirc )
    // {
        // DOUT("\t" << g->qasm() );
    // }
    // DOUT("... End circuit after mapping");
    IOUT("Mapped circuit depth=" << mainPast.MaxFreeCycle()-1 << ", circuit starting at cycle " << inCirc[0]->cycle);
    DOUT("Mapping circuit [DONE]");
    DOUT("==================================");
}   // end MapCircuit

};  // end class Mapper

#endif
