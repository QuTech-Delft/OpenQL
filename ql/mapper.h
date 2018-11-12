#define INITIALPLACE 1
/**
 * @file   mapper.h
 * @date   09/2018
 * @author Hans van Someren
 * @brief  openql virtual to real qubit mapping and routing
 */

#ifndef QL_MAPPER_H
#define QL_MAPPER_H

#include "ql/utils.h"
#include "ql/platform.h"
#include "ql/arch/cc_light_resource_manager.h"
#include "ql/gate.h"

void assert_fail(const char *f, int l, const char *s)
{
    EOUT("assert " << s << " failed in file " << f << " at line " << l);
    throw ql::exception("assert failed",false);
}
#define MapperAssert(condition)   { if (!(condition)) { assert_fail(__FILE__, __LINE__, #condition); } }

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
//
// it is wrong when nv is just the number of virtual qubits in the program
// because when swapping through the grid, more real qubits might get involved
// and then we're having more real than virtual qubits in use; and then the mapping is not 1-1 anymore;
// so nv as size of Virt2Real maps should be the number of real qubits in the platform;
// at the same time, each virtual qubit index should be < nv, or we need a v2i map as in initial placement
class Virt2Real
{
private:

    size_t              nv;                // size of the map; after initialization, will always be the same
    std::vector<size_t> v2rMap;            // v2rMap[virtual qubit index] -> real qubit index


// map real qubit to the virtual qubit index that is mapped to it (i.e. backward map)
// a second vector next to v2rMap (i.e. an r2vMap) would speed this up
size_t GetVirt(size_t r)
{
    for (size_t v=0; v<nv; v++)
    {
        if (v2rMap[v] == r) return v;
    }
    MapperAssert(0);
    return MAX_CYCLE;
}


public:

// expand to desired size and initialize to trivial (1-1) mapping
void Init(size_t n)
{
    nv = n;
    // DOUT("Virt2Real::Init(n=" << nv << "), initializing 1-1 mapping");
    v2rMap.resize(nv);
    for (size_t i=0; i<nv; i++)
    {
        v2rMap[i] = i;
    }
}

// map virtual qubit index to real qubit index
size_t& operator[] (size_t v)
{
    MapperAssert(v < nv);
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
    MapperAssert(v0 != v1);
    MapperAssert(v0 < nv);
    MapperAssert(v1 < nv);

    v2rMap[v0] = r1;
    v2rMap[v1] = r0;
}

void Print(std::string s)
{
    std::cout << "... Virt2Real(v->r) " << s << ":";
    for (size_t v=0; v<nv; v++)
    {
        size_t r = v2rMap[v];
        std::cout << " (" << v << "->" << r << ")";
    }
    std::cout << std::endl;
#ifdef debug
    std::cout << "... real2virt(r->v) " << s << ":";
    for (size_t r=0; r<nv; r++)
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

    ql::quantum_platform   *platformp;// platform description
    size_t                  nq;      // size of the map; after initialization, will always be the same
    size_t                  ct;      // multiplication factor from cycles to nano-seconds (unit of duration)
    std::vector<size_t>     fcv;     // fcv[real qubit index i]: qubit i is free from this cycle on
    ql::arch::resource_manager_t rm; // actual resources occupied by scheduled gates


// access free cycle value of qubit i
size_t& operator[] (size_t i)
{
    return fcv[i];
}

public:

void Init(ql::quantum_platform *p)
{
    // DOUT("FreeCycle::Init()");
    ql::arch::resource_manager_t lrm(*p);   // allocated here and copied below to rm because of platform parameter
    // DOUT("... created local resource manager");
    // DOUT("... FreeCycle: nq=" << n << ", c=" << c << "), initializing to all 0 cycles");
    platformp = p;
    nq = platformp->qubit_number;
    ct = platformp->cycle_time;
    fcv.clear();
    fcv.resize(nq, 1);   // this 1 implies that cycle of first gate will be 1 and not 0; OpenQL convention!?!?
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
    size_t  minFreeCycle = MAX_CYCLE;
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
    if (mapopt == "baserc" || mapopt == "minextendrc")
    {
        size_t  baseStartCycle = startCycle;
        GetGateParameters(id, platformp, operation_name, operation_type, instruction_type);
        while (startCycle < MAX_CYCLE)
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

// schedule gate g in the FreeCycle and resource map
// gate operands are real qubit indices
// both the FreeCycle map and the resource map are updated
void Add(ql::gate *g, size_t startCycle)
{
    AddNoRc(g, startCycle);

    auto        mapopt = ql::options::get("mapper");
    if (mapopt == "baserc" || mapopt == "minextendrc")
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
// not windowing
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

    size_t                  nq;         // width of Past and Virt2Real map in number of real qubits
    size_t                  ct;         // cycle time, multiplier from cycles to nano-seconds
    ql::quantum_platform   *platformp;  // platform describing resources for scheduling
    std::map<std::string,ql::custom_gate*> *gate_definitionp; // gate definitions from platform's .json file
                                        // to be able to create new gates
    Virt2Real               v2r;        // Virt2Real map applying to this Past
    FreeCycle               fc;         // FreeCycle map applying to this Past

    typedef ql::gate *      gate_p;
    std::list<gate_p>       waitinglg;  // list of gates in this Past, top order, waiting to be scheduled in
    std::list<gate_p>       lg;         // list of gates in this Past, scheduled by their (start) cycle values
    size_t                  nswapsadded;// number of swaps (incl. moves) added to this past
    size_t                  nmovesadded;// number of moves added to this past
    std::map<gate_p,size_t> cycle;      // gate to cycle map, startCycle value of each past gatecycle[gp]
    ql::circuit             *outCircp;  // output stream after past

public:

// past initializer
void Init(ql::quantum_platform *p)
{
    // DOUT("Past::Init");
    platformp = p;

    nq = platformp->qubit_number;
    ct = platformp->cycle_time;
    gate_definitionp = &platformp->instruction_map;
    v2r.Init(nq);
    fc.Init(platformp);
    waitinglg.clear();  // waitinglg is initialized to empty list
    lg.clear();         // lg is initialized to empty list
    nswapsadded = 0;
    nmovesadded = 0;
    cycle.clear();      // cycle is initialized to empty map
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

void Output(ql::circuit& circ)
{
    outCircp = &circ;
}

void GetV2r(Virt2Real& argv2r)
{
    argv2r = v2r;
}

void SetV2r(Virt2Real& newv2r)
{
    v2r = newv2r;
}

// all gates in past.waitinglg are scheduled into past.lg
// note that these gates all are mapped and so have real operand qubit indices
// the FreeCycle map reflects for each qubit the first free cycle
// all new gates, now in waitinglist, get such a cycle assigned below, increased gradually, until definitive
void Schedule()
{
    // DOUT("Schedule ...");

    MapperAssert (!waitinglg.empty());

    do
    {
        size_t      startCycle = MAX_CYCLE;
        gate_p      gp;

        // find the gate with the minimum startCycle
        //
        // IMPORTANT: this assumes that the waitinglg gates list is in topological order,
        // which is ok because the pair of swap lists use distict qubits and
        // the gates of each are added to the back of the list in the order of execution.
        // So, using AddNoRc, the tryfc (try FreeCycle map) reflects the earliest startCycle per qubit,
        // and so dependences are respected, so we can find the gate that can start first ...
        // This is really a hack to avoid the construction of a dependence graph.
        // We use a copy of fc and not fc itself, since the latter reflects the really scheduled gates.
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
        // DOUT("... set " << gp->qasm() << " at cycle " << startCycle);
    
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

// ===========================================
// essentially copies follow of the gate interface of kernel.h, adding the instructions instead to the circ parameter


// if a specialized custom gate ("cz q0 q4") is available, add it to circuit and return true
// if a parameterized custom gate ("cz") is available, add it to circuit and return true
//
// note that there is no check for the found gate being a composite gate; this is in HvS's opinion, a flaw
bool new_custom_gate_if_available(std::string & gname, std::vector<size_t> qubits, ql::circuit& circ,
                                  size_t duration=0, double angle=0.0)
{
    bool added = false;
    // first check if a specialized custom gate is available
    std::string instr = gname + " ";
    if(qubits.size() > 0)
    {
        for (size_t i=0; i<(qubits.size()-1); ++i)
            instr += "q" + std::to_string(qubits[i]) + ",";
        if(qubits.size() >= 1) // to make if work with gates without operands
            instr += "q" + std::to_string(qubits[qubits.size()-1]);
    }

    std::map<std::string,ql::custom_gate*>::iterator it = gate_definitionp->find(instr);
    if (it != gate_definitionp->end())
    {
        // a specialized custom gate is of the form: "cz q0 q3"
        ql::custom_gate* g = new ql::custom_gate(*(it->second));
        for(auto & qubit : qubits)
            g->operands.push_back(qubit);
        if(duration>0) g->duration = duration;
        g->angle = angle;
        added = true;
        circ.push_back(g);
    }
    else
    {
        // otherwise, check if there is a parameterized custom gate (i.e. not specialized for arguments)
        // this one is of the form: "cz", i.e. just the gate's name
        std::map<std::string,ql::custom_gate*>::iterator it = gate_definitionp->find(gname);
        if (it != gate_definitionp->end())
        {
            ql::custom_gate* g = new ql::custom_gate(*(it->second));
            for(auto & qubit : qubits)
                g->operands.push_back(qubit);
            if(duration>0) g->duration = duration;
            g->angle = angle;
            added = true;
            circ.push_back(g);
        }
    }

    if(added)
    {
        // DOUT("new: custom gate added for " << gname);
    }
    else
    {
        // DOUT("new: custom gate not added for " << gname);
    }

    return added;
}

// return the subinstructions of a composite gate
// while doing, test whether the subinstructions have a definition (so they cannot be specialized or default ones!)
void new_get_decomposed_ins( ql::composite_gate * gptr, std::vector<std::string> & sub_instructons)
{
    auto & sub_gates = gptr->gs;
    // DOUT("new: composite ins: " << gptr->name);
    for(auto & agate : sub_gates)
    {
        std::string & sub_ins = agate->name;
        // DOUT("new:   sub ins: " << sub_ins);
        auto it = gate_definitionp->find(sub_ins);
        if( it != gate_definitionp->end() )
        {
            sub_instructons.push_back(sub_ins);
        }
        else
        {
            throw ql::exception("[x] error : ql::kernel::gate() : gate decomposition not available for '"+sub_ins+"'' in the target platform !",false);
        }
    }
}

// if specialized composed gate: "cz q0,q3" available, with composition of subinstructions, return true
//      also check each subinstruction for presence of a custom_gate (or a default gate)
// otherwise, return false
bool new_spec_decomposed_gate_if_available(std::string gate_name, std::vector<size_t> all_qubits, ql::circuit& circ)
{
    bool added = false;
    // DOUT("new: Checking if specialized decomposition is available for " << gate_name);
    std::string instr_parameterized = gate_name + " ";
    size_t i;
    if(all_qubits.size() > 0)
    {
        for(i=0; i<all_qubits.size()-1; i++)
        {
            instr_parameterized += "q" + std::to_string(all_qubits[i]) + " ";
        }
        if(all_qubits.size() >= 1)
        {
            instr_parameterized += "q" + std::to_string(all_qubits[i]);
        }
    }
    // DOUT("new: decomposed specialized instruction name: " << instr_parameterized);

    auto it = gate_definitionp->find(instr_parameterized);
    if( it != gate_definitionp->end() )
    {
        // DOUT("new: specialized composite gate found for " << instr_parameterized);
        ql::composite_gate * gptr = (ql::composite_gate *)(it->second);
        if( ql::__composite_gate__ == gptr->type() )
        {
            // DOUT("new: composite gate type");
        }
        else
        {
            // DOUT("new: Not a composite gate type");
            return false;
        }


        std::vector<std::string> sub_instructons;
        new_get_decomposed_ins( gptr, sub_instructons);
        for(auto & sub_ins : sub_instructons)
        {
            // DOUT("new: Adding sub ins: " << sub_ins);
            std::replace( sub_ins.begin(), sub_ins.end(), ',', ' ');
            // DOUT("new:  after comma removal, sub ins: " << sub_ins);
            std::istringstream iss(sub_ins);

            std::vector<std::string> tokens{ std::istream_iterator<std::string>{iss},
                                             std::istream_iterator<std::string>{} };

            std::vector<size_t> this_gate_qubits;
            std::string & sub_ins_name = tokens[0];

            for(size_t i=1; i<tokens.size(); i++)
            {
                // DOUT("new: tokens[i] : " << tokens[i]);
                auto sub_str_token = tokens[i].substr(1);
                // DOUT("new: sub_str_token[i] : " << sub_str_token);
                this_gate_qubits.push_back( stoi( tokens[i].substr(1) ) );
            }

            // DOUT( ql::utils::to_string<size_t>(this_gate_qubits, "new: actual qubits of this gate:") );

            // custom gate check
            // when found, custom_added is true, and the expanded subinstruction was added to the circuit
            bool custom_added = new_custom_gate_if_available(sub_ins_name, this_gate_qubits, circ);
            if(!custom_added)
            {
                // DOUT("unknown gate '" << sub_ins_name << "' with " << ql::utils::to_string(this_gate_qubits,"qubits") );
                throw ql::exception("[x] error : ql::kernel::gate() : the gate '"+sub_ins_name+"' with " +ql::utils::to_string(this_gate_qubits,"qubits")+" is not supported by the target platform !",false);
            }
        }
        added = true;
    }
    else
    {
        // DOUT("new: composite gate not found for " << instr_parameterized);
    }

    return added;
}


// if composite gate: "cz %0 %1" available, return true;
//      also check each subinstruction for availability as a custom gate (or default gate)
// if not, return false
bool new_param_decomposed_gate_if_available(std::string gate_name, std::vector<size_t> all_qubits, ql::circuit& circ)
{
    bool added = false;
    // DOUT("new: Checking if parameterized decomposition is available for " << gate_name);
    std::string instr_parameterized = gate_name + " ";
    size_t i;
    if(all_qubits.size() > 0)
    {
        for(i=0; i<all_qubits.size()-1; i++)
        {
            instr_parameterized += "%" + std::to_string(i) + " ";
        }
        if(all_qubits.size() >= 1)
        {
            instr_parameterized += "%" + std::to_string(i);
        }
    }
    // DOUT("new: decomposed parameterized instruction name: " << instr_parameterized);

    // check for composite ins
    auto it = gate_definitionp->find(instr_parameterized);
    if( it != gate_definitionp->end() )
    {
        // DOUT("new: parameterized composite gate found for " << instr_parameterized);
        ql::composite_gate * gptr = (ql::composite_gate *)(it->second);
        if( ql::__composite_gate__ == gptr->type() )
        {
            // DOUT("new: composite gate type");
        }
        else
        {
            // DOUT("new: Not a composite gate type");
            return false;
        }

        std::vector<std::string> sub_instructons;
        new_get_decomposed_ins( gptr, sub_instructons);
        for(auto & sub_ins : sub_instructons)
        {
            // DOUT("new: Adding sub ins: " << sub_ins);
            std::replace( sub_ins.begin(), sub_ins.end(), ',', ' ');
            // DOUT("new:  after comma removal, sub ins: " << sub_ins);
            std::istringstream iss(sub_ins);

            std::vector<std::string> tokens{ std::istream_iterator<std::string>{iss},
                                             std::istream_iterator<std::string>{} };

            std::vector<size_t> this_gate_qubits;
            std::string & sub_ins_name = tokens[0];

            for(size_t i=1; i<tokens.size(); i++)
            {
                this_gate_qubits.push_back( all_qubits[ stoi( tokens[i].substr(1) ) ] );
            }

            // DOUT( ql::utils::to_string<size_t>(this_gate_qubits, "new: actual qubits of this gate:") );

            // custom gate check
            // when found, custom_added is true, and the expanded subinstruction was added to the circuit
            bool custom_added = new_custom_gate_if_available(sub_ins_name, this_gate_qubits, circ);
            if(!custom_added)
            {
                // DOUT("unknown gate '" << sub_ins_name << "' with " << ql::utils::to_string(this_gate_qubits,"qubits") );
                throw ql::exception("[x] error : ql::kernel::gate() : the gate '"+sub_ins_name+"' with " +ql::utils::to_string(this_gate_qubits,"qubits")+" is not supported by the target platform !",false);
            }
        }
        added = true;
    }
    else
    {
        // DOUT("new: composite gate not found for " << instr_parameterized);
    }
    return added;
}

/**
 * custom gate with arbitrary number of operands
 * return the gate (or its decomposition) by appending it to the circuit parameter
 */
// terminology:
// - composite/custom/default (in decreasing order of priority during lookup in the gate definition):
//      - composite gate: a gate definition with subinstructions; when matched, decompose and add the subinstructions
//      - custom gate: a fully configurable gate definition, with all kinds of attributes; there is no decomposition
//      - default gate: a gate definition build-in in this compiler; see above for the definition
//          deprecated; setting option "use_default_gates" from "yes" to "no" turns it off
// - specialized/parameterized (in decreasing order of priority during lookup in the gate definition)
//      - specialized: a gate definition that is special for its operands, i.e. the operand qubits must match
//      - parameterized: a gate definition that can be used for all possible qubit operands
//
// the following order of checks is used below:
// check if specialized composite gate is available
//      "cz q0,q3" available as composite gate, where subinstructions are available as custom gates
// if not, check if parameterized composite gate is available
//      "cz %0 %1" in gate_definition, where subinstructions are available as custom gates
// if not, check if a specialized custom gate is available
//      "cz q0,q3" available as non-composite gate
// if not, check if a parameterized custom gate is available
//      "cz" in gate_definition as non-composite gate
// (default gate is not supported)
// if not, then return false else true

bool new_gate(std::string gname, std::vector<size_t> qubits, ql::circuit& circ, size_t duration=0, double angle = 0.0)
{
    bool added = false;
    for(auto & qno : qubits)
    {
        // DOUT("new: qno : " << qno);
        if( qno >= nq )
        {
            EOUT("Number of qubits in platform: " << std::to_string(nq) << ", specified qubit numbers out of range for gate: '" << gname << "' with " << ql::utils::to_string(qubits,"qubits") );
            throw ql::exception("[x] error : ql::kernel::gate() : Number of qubits in platform: "+std::to_string(nq)+", specified qubit numbers out of range for gate '"+gname+"' with " +ql::utils::to_string(qubits,"qubits")+" !",false);
        }
    }

    str::lower_case(gname);
    // DOUT("new: Adding gate : " << gname << " with " << ql::utils::to_string(qubits,"qubits"));

    // specialized composite gate check
    // DOUT("new: trying to add specialized decomposed gate(s) for: " << gname);
    bool spec_decom_added = new_spec_decomposed_gate_if_available(gname, qubits, circ);
    if(spec_decom_added)
    {
        added = true;
        // DOUT("new: specialized decomposed gates added for " << gname);
    }
    else
    {
        // parameterized composite gate check
        // DOUT("new: trying to add parameterized decomposed gate for: " << gname);
        bool param_decom_added = new_param_decomposed_gate_if_available(gname, qubits, circ);
        if(param_decom_added)
        {
            added = true;
            // DOUT("new: decomposed gates added for " << gname);
        }
        else
        {
            // specialized/parameterized custom gate check
            // DOUT("new: adding custom gate for " << gname);
            bool custom_added = new_custom_gate_if_available(gname, qubits, circ, duration, angle);
            if(!custom_added)
            {
                // DOUT("unknown gate '" << gname << "' with " << ql::utils::to_string(qubits,"qubits") );
                // throw ql::exception("[x] error : ql::kernel::gate() : the gate '"+gname+"' with " +ql::utils::to_string(qubits,"qubits")+" is not supported by the target platform !",false);
            }
            else
            {
                added = true;
                // DOUT("new: custom gate added for " << gname);
            }
        }
    }
    // DOUT("new: ");
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

// generate a single swap with real operands and add it to the current past's waiting list
// note that the swap may be implemented by a series of gates
void AddSwap(size_t r0, size_t r1)
{
    bool created;
    ql::circuit circ;

    // DOUT("... adding/trying swap(q" << r0 << ",q" << r1 << ") ... " );

    created = new_gate("swap_real", {r0,r1}, circ);    // gates implementing swap returned in circ
    if (!created)
    {
        created = new_gate("swap", {r0,r1}, circ);
        if (!created)
        {
            EOUT("unknown gates 'swap(q" << r0 << ",q" << r1 << ")' and 'swap_real(...)'");
            throw ql::exception("[x] error : ql::mapper::new_gate() : the gates 'swap' and 'swap_real' are not supported by the target platform !",false);
        }
    }
    for (auto &gp : circ)
    {
        Add(gp);
    }
    nswapsadded++;                       // for reporting at the end
    // DOUT("... swap(q" << r0 << ",q" << r1 << ")");

    v2r.Swap(r0,r1);
}

// add the mapped gate (with real qubit indices as operands) to the past
// by adding it to the waitinglist and scheduling it into the past
void AddAndSchedule(gate_p gp)
{
    Add(gp);
    Schedule();
}

// find real qubit index implementing virtual qubit index
size_t MapQubit(size_t v)
{
    return v2r[v];
}

// devirtualize gp
// assume gp points to a virtual gate with virtual qubit indices as operands;
// when a gate can be created with the same name but with "_real" appended, with the real qubits as operands, then create that gate
// otherwise keep the old gate; replace the virtual qubit operands by the real qubit indices
// since creating a new gate may result in a decomposition to several gates, the result is returned as a circuit vector
void DeVirtualize(ql::gate* gp, ql::circuit& circ)
{
    std::vector<size_t> real_qubits  = gp->operands; // a copy!
    for (auto& qi : real_qubits)
    {
        qi = MapQubit(qi);
    }

    std::string real_gname = gp->name;   // a copy!
    real_gname.append("_real");
    bool created = new_gate(real_gname, real_qubits, circ);
    if (created)
    {
        DOUT("... DeVirtualize: new gates created for: " << real_gname);
    }
    else
    {
        gp->operands = real_qubits;
        // DOUT("... DeVirtualize: keep gate after mapping qubit indices: " << gp->qasm());
        circ.push_back(gp);
    }
}

// as mapper after-burner
// decompose all gates with names ending in _prim
// by replacing it by a new copy of this gate with as name _prim replaced by _dprim
// and decomposing it according to the .json file gate decomposition
void Decompose(ql::gate* gp, ql::circuit& circ)
{
    std::string gname = gp->name;   // a copy!
    std::string postfix ("_prim");
    std::size_t found = gname.find(postfix, (gname.length()-postfix.length())); // i.e. postfix ends gname

    if (found != std::string::npos)
    {
        // decompose gates with _prim postfix to equivalent with _dprim
        gname.replace(found, postfix.length(), "_dprim"); 
        bool created = new_gate(gname, gp->operands, circ);
        if (!created)
        {
            EOUT("unknown gate '" << gname << "' with " << ql::utils::to_string(gp->operands,"qubits") );
            throw ql::exception("[x] error : ql::kernel::gate() : the gate '"+gname+"' with " +ql::utils::to_string(gp->operands,"qubits")+" is not supported by the target platform !",false);
        }
        else
        {
            DOUT("... Decomposed: " << gp->qasm() << " to decomposition of " << gname << "(...)");
        }
    }
    else
    {
        DOUT("... Decompose: keep gate: " << gp->qasm());
        circ.push_back(gp);
    }
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

    fc.Init(platformp);
    lg.clear();         // lg is initialized to empty list
    cycle.clear();      // cycle is initialized to empty map
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

    ql::quantum_platform   *platformp;  // descriptions of resources for scheduling
    size_t                  nq;         // width of Past and Virt2Real map is number of real qubits
    size_t                  ct;         // cycle time, multiplier from cycles to nano-seconds

    std::vector<size_t>     total;      // full path, including source and target nodes
    std::vector<size_t>     fromSource; // partial path after split, starting at source
    std::vector<size_t>     fromTarget; // partial path after split, starting at target, backward

    Past                    past;       // cloned main past, extended with swaps from this path
    size_t                  cycleExtend;// latency extension caused by the path


public:
// NNPath initializer
// This should only be called after a virgin construction and not after cloning a path.
void Init(ql::quantum_platform* p)
{
    // DOUT("path::Init(number of virtual qubits=" << n);
    platformp = p;

    nq = platformp->qubit_number;
    ct = platformp->cycle_time;
    // total, fromSource and fromTarget start as empty vectors
    past.Init(platformp);                // initializes past to empty
    cycleExtend = MAX_CYCLE;             // means undefined
}

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
            std::cout << "]";
            if (pp.size() >= 2)
            {
                std::cout << " implying:";
                for (size_t i = 0; i < pp.size()-1; i++)
                {
                    std::cout << " swap(q" << pp[i] << ",q" << pp[i+1] << ")";
                }
            }
            std::cout << std::endl;
        }
    }
}

void Print(std::string s)
{
    std::cout << s;
    if (cycleExtend != MAX_CYCLE)
    {
        std::cout << ": cycleExtend=" << cycleExtend << std::endl;
    }
    if (fromSource.empty() && fromTarget.empty())
    {
        partialPrint("\ttotal path", total);
    }
    else
    {
        partialPrint("\tpath from source", fromSource);
        partialPrint("\tpath from target", fromTarget);
    }
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

// create a single node (i.e. distance 0) path consisting of just the qubit q
void Single(size_t q)
{
    // total.resize(1);
    // total[0] = q;
    total.insert(total.begin(), q); // hopelessly inefficient
}

// add a node to the path in front, extending its length with one
void Add2Front(size_t q)
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
    // DOUT("... adding swaps for local past ...");
    AddSwaps(past);
    // DOUT("... done adding swaps for local past");
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
    MapperAssert (length >= 3);   // distance > 1 so path at least: source -> intermediate -> target
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
// Grid: definition and access functions to the grid of qubits that supports the real qubits
class Grid
{
private:
    ql::quantum_platform* platformp;    // current platform: topology
    size_t nqbits;                      // number of qubits in the platform
                                        // Grid configuration, all constant after initialization
    size_t nx;                          // length of x dimension (x coordinates count 0..nx-1)
    size_t ny;                          // length of y dimension (y coordinates count 0..ny-1)
    std::map<size_t,size_t> x;          // x[i] is x coordinate of qubit i
    std::map<size_t,size_t> y;          // y[i] is y coordinate of qubit i

public:
    typedef std::list<size_t> neighbors_t;  // neighbors is a list of qubits
    std::map<size_t,neighbors_t> nbs;   // nbs[i] is list of neighbor qubits of qubit i


// distance between two qubits
// implementation is for "cross" and "star" grids and assumes bidirectional edges and convex grid;
// for "plus" grids, replace "std::max" by "+"
size_t Distance(size_t from_realqbit, size_t to_realqbit)
{
    return std::max(
               std::abs( ptrdiff_t(x[to_realqbit]) - ptrdiff_t(x[from_realqbit]) ),
               std::abs( ptrdiff_t(y[to_realqbit]) - ptrdiff_t(y[from_realqbit]) ));
}

// Grid initializer
// initialize mapper internal grid maps from configuration
// this remains constant over multiple kernels on the same platform
void Init(ql::quantum_platform* p)
{
    DOUT("Grid::Init");
    platformp = p;
    nqbits = platformp->qubit_number;
    DOUT("... number of real qbits=" << nqbits);

    nx = platformp->topology["x_size"];
    ny = platformp->topology["y_size"];
    DOUT("... nx=" << nx << "; ny=" << ny);

    // init x, y and nbs maps
    for (auto & aqbit : platformp->topology["qubits"] )
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
    for (auto & anedge : platformp->topology["edges"] )
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
    for (int i=0; i<nqbits; i++)
    {
        DOUT("qubit[" << i << "]: x=" << x[i] << "; y=" << y[i]);
        std::cout << "... connects to ";
        for (auto & n : nbs[i])
        {
            std::cout << n << " ";
        }
        std::cout << std::endl;
        std::cout << "... distance(" << i << ",j)=";
        for (int j=0; j<nqbits; j++)
        {
            std::cout << Distance(i,j) << " ";
        }
        std::cout << std::endl;
    }
#endif        // debug
}


};  // end class Grid

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

#ifdef INITIALPLACE
#include <lemon/lp.h>
using namespace lemon;

typedef
enum InitialPlaceResults
{
    ipr_any,            // any mapping will do because there are no two-qubit gates in the circuit
    ipr_current,        // current mapping will do because all two-qubit gates are NN
    ipr_newmap,         // initial placement solution found a mapping
    ipr_failed          // initial placement solution failed
} ipr_t;

class InitialPlace
{
private:
                                        // parameters, constant for a kernel
    ql::quantum_platform   *platformp;  // platform
    size_t                  nlocs;      // number of locations, real qubits; index variables k and l
    size_t                  nvq;        // same range as nlocs; when not, take set from config and create v2i earlier
    Grid                   *gridp;      // current grid with Distance function

                                        // remaining attributes are computed per circuit
    size_t                  nfac;       // number of facilities, actually used virtual qubits; index variables i and j
                                        // nfac <= nlocs: e.g. nlocs == 7, but only v2 and v5 are used; nfac then is 2

public:

// kernel-once initialization
void Init(Grid* g, ql::quantum_platform *p)
{
    DOUT("InitialPlace Init ...");
    platformp = p;
    nlocs = p->qubit_number;
    nvq = p->qubit_number;  // same range; when not, take set from config and create v2i earlier
    DOUT("... number of real qubits (locations): " << nlocs);
    gridp = g;
}

// find an initial placement of the virtual qubits for the given circuit
// the resulting placement is put in the provided virt2real map
// result indicates one of the result indicators (ipr_t, see above)
void Place( ql::circuit& circ, Virt2Real& v2r, ipr_t &result)
{
    DOUT("InitialPlace circuit ...");

    // compute usecount to know which virtual qubits are actually used
    // use it to compute v2i, mapping virtual qubit indices to contiguous facility indices
    // finally, nfac is set to the number of these facilities
    DOUT("... compute usecount by scanning circuit");
    std::vector<size_t>  usecount;  // usecount[v] = count of use of virtual qubit v in current circuit
    usecount.resize(nvq,0);         // initially all 0
    std::vector<size_t> v2i;        // v2i[virtual qubit index v] -> index of facility i
    v2i.resize(nvq,MAX_CYCLE);      // MAX_CYCLE means undefined, virtual qubit v not used by circuit as gate operand
    for ( auto& gp : circ )
    {
        for ( auto v : gp->operands)
        {
            usecount[v] += 1;
        }
    }
    nfac = 0;
    for (size_t v=0; v < nvq; v++)
    {
        if (usecount[v] != 0)
        {
            v2i[v] = nfac;
            nfac += 1;
        }
    }
    DOUT("... number of facilities: " << nfac << " while number of virtual qubits is: " << nvq);

    // precompute refcount by scanning circuit
    // refcount[i][j] = count of two-qubit gates between facilities i and j in current circuit
    // at the same time, set anymap and currmap
    // anymap = there are no two-qubit gates so any map will do
    // currmap = in the current map, all two-qubit gates are NN so current map will do
    DOUT("... compute refcount by scanning circuit");
    std::vector<std::vector<size_t>>  refcount;
    refcount.resize(nfac); for (size_t i=0; i<nfac; i++) refcount[i].resize(nfac,0);
    bool anymap = true;    // true when all refcounts are 0
    bool currmap = true;   // true when in current map all two-qubit gates are NN
    for ( auto& gp : circ )
    {
        auto&   q = gp->operands;
        if (q.size() > 2)
        {
            EOUT(" gate: " << gp->qasm() << " has more than 2 operand qubits; please decompose such gates first before mapping.");
            throw ql::exception("Error: gate with more than 2 operand qubits; please decompose such gates first before mapping.", false);
        }
        if (q.size() == 2)
        {
            anymap = false;
            refcount[v2i[q[0]]][v2i[q[1]]] += 1;
            
            if (gridp->Distance(v2r[q[0]], v2r[q[1]]) > 1)
            {
                currmap = false;
            }
        }
    }
    if (anymap)
    {
        DOUT("Initial placement: no two-qubit gates found, so no constraints, and any mapping is ok");
        DOUT("InitialPlace circuit [ANY]");
        result = ipr_any;
        return;
    }
    if (currmap)
    {
        DOUT("Initial placement: in current map, all two-qubit gates are nearest neighbor, so current map is ok");
        DOUT("InitialPlace circuit [CURRENT]");
        result = ipr_current;
        return;
    }

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
                    costmax[i][k] += refcount[i][j] * gridp->Distance(k,l);
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
    
    // solve the problem
    WOUT("... solve the initial placement model, this may take a while ...");
    DOUT("... solve the problem");
    Mip::SolveExitStatus s = mip.solve();
    DOUT("... determine result of solving");
    Mip::ProblemType pt = mip.type();
    if (s != Mip::SOLVED || pt != Mip::OPTIMAL)
    {
        DOUT("Initial placement: no (optimal) solution found; solve returned:"<< s << " type returned:" << pt);
        EOUT("Initial placement: no (optimal) solution found; solve returned:"<< s << " type returned:" << pt);
        // throw ql::exception("Initial placement: no (optimal) solution found", false);
        result = ipr_failed;
        DOUT("InitialPlace circuit [FAILED]");
        return;
    }

    // get the results: x[i][k] == 1 iff facility i is in location k (i.e. real qubit index k)
    // use v2i to translate facilities back to virtual qubit indices
    // and fill v2r with the found locations for the used virtual qubits;
    // the unused virtual qubits are mapped to an arbitrary permutation of the remaining locations
    // since these might get used by swap paths and thus must have a (fake) virtual qubit assigned for the mapper's logic
    DOUT("... interpret result and copy to Virt2Real");
    for (size_t v=0; v<nvq; v++)
    {
        v2r[v] = MAX_CYCLE;      // i.e. undefined, i.e. v is not an index of a used virtual qubit
    }
    for ( size_t i=0; i<nfac; i++ )
    {
        size_t v;   // found virtual qubit index v represented by facility i
        // use v2i backward to find virtual qubit v represented by facility i
        for (v=0; v<nvq; v++)
        {
            if (v2i[v] == i)
            {
                break;
            }
        }
        MapperAssert(v < nvq);  // for each facility there must be a virtual qubit
        size_t k;   // location to which facility i and thus virtual qubit index v was allocated
        for (k=0; k<nlocs; k++ )
        {
            if (mip.sol(x[i][k]) == 1)
            {
                v2r[v] = k;
                break;
            }
        }
        MapperAssert(k < nlocs);  // each facility i by definition represents a used qubit so must have got a location
    }
    v2r.Print("... result Virt2Real map of InitialPlace before adding unused virtual qubits and unused locations ");
    // used virtual qubits v have got their location k filled in in v2r[v] == k
    // unused virtual qubits still have location MAX_CYCLE, fill those with the remaining locations
    for (size_t v=0; v<nvq; v++)
    {
        if (v2r[v] == MAX_CYCLE)
        {
            // v is unused; find an unused location k
            size_t k;   // location k that is checked for having been allocated to some virtual qubit w
            for (k=0; k<nlocs; k++ )
            {
                int found_this_k_use = 0;
                for (size_t w=0; w<nvq; w++)
                {
                    if (v2r[w] == k)
                    {
                        found_this_k_use = 1;
                        break;
                    }
                }
                if (found_this_k_use == 0)
                {
                    break;     // k is an unused location
                }
                // k is a used location, so continue with next k to check whether it is hopefully unused
            }
            MapperAssert(k < nlocs);  // when a virtual qubit is not used, there must be a location that is not used
            v2r[v] = k;
        }
    }
    v2r.Print("... final result Virt2Real map of InitialPlace");
    result = ipr_newmap;
    DOUT("InitialPlace circuit [DONE]");
}
    
};  // end class InitialPlace
#endif


// =========================================================================================
// Mapper: map operands of gates and insert swaps so that two-qubit gate operands are NN
// all gates must be unary or two-qubit gates
// The operands are virtual qubit indices, in the same range as the real qubit indices of the platform.
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
                                    // Initialized by Mapper.Init
                                    // OpenQL wide configuration, all constant after initialization
    ql::quantum_platform platform;  // current platform: topology and gate definitions
    size_t          nqbits;         // number of qubits in the platform, number of real qubits
    size_t          cycle_time;     // length in ns of a single cycle of the platform
                                    // is divisor of duration in ns to convert it to cycles
    Grid            grid;           // current grid

                                    // Initialized by Mapper.MapCircuit
#ifdef INITIALPLACE
    InitialPlace    ip;             // initial placer facility
#endif
    Past            mainPast;       // main past window; all path alternatives start off as clones of it


// Mapper constructor is default synthesized

private:
// initial path finder
// generate all paths with source src and target tgt as a list of path into reslp
// this result list reslp is allocated by caller and is empty on the call
void GenShortestPaths(size_t src, size_t tgt, std::list<NNPath> & reslp)
{
    std::list<NNPath> genlp;    // list that will get the result of a recursive Gen call

    // DOUT("GenShortestPaths: " << "src=" << src << " tgt=" << tgt);
    MapperAssert (reslp.empty());

    if (src == tgt) {
        // found target
        // create a virgin path and initialize it to become an empty path
        // add src to this path (so that it becomes a distance 0 path with one qubit, src)
        // and add the path to the result list 
        // DOUT("GenShortestPaths ... about to allocate local virgin path");
        NNPath  p;

        // DOUT("... done allocate local virgin path; now init it");
        p.Init(&platform);
        // DOUT("... done init path; now add src to it, converting it to an empty path");
        p.Add2Front(src);
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
    size_t d = grid.Distance(src, tgt);
    MapperAssert (d >= 1);

    // loop over all neighbors of src
    for (auto & n : grid.nbs[src])
    {
        size_t dn = grid.Distance(n, tgt);

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
        MapperAssert (genlp.empty());
    }
    // reslp contains all paths starting from a neighbor of src, to tgt

    // add src to front of all to-be-returned paths from src's neighbors to tgt
    for (auto & p : reslp)
    {
        // DOUT("... GenShortestPaths, about to add src=" << src << "in front of path");
        p.Add2Front(src);
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
    size_t  minExtension = MAX_CYCLE;
    NNPath  minp;

    // DOUT("MinimalExtendingPath");
    MapperAssert (!lp.empty());   // so there always is a result path

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
void MapMinExtend(ql::gate* gp)
{
    auto&   q = gp->operands;
    size_t  src = mainPast.MapQubit(q[0]);
    size_t  tgt = mainPast.MapQubit(q[1]);
    size_t  d = grid.Distance(src, tgt);
    MapperAssert (d >= 1);
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
void MapBase(ql::gate* gp)
{
    auto& q = gp->operands;
    size_t src = mainPast.MapQubit(q[0]);
    size_t tgt = mainPast.MapQubit(q[1]);
        
    size_t d = grid.Distance(src, tgt);
    DOUT("... MapBase: " << gp->qasm() << " in real (q" << src << ",q" << tgt << ") at distance=" << d );
    while (d > 1)
    {
        for( auto & n : grid.nbs[src] )
        {
            size_t dnb = grid.Distance(n, tgt);
            if (dnb < d)
            {
                // DOUT(" ... distance(real " << n << ", real " << tgt << ")=" << dnb);
                mainPast.AddSwap(src, n);
                DOUT(" ... adding swap(q" << src << ",q" << n << ")");
                mainPast.Schedule();
                // mainPast.Print("mapping after swap");
                src = n;
                break;
            }
        }
        d = grid.Distance(src, tgt);
        // DOUT(" ... new distance(real " << src << ", real " << tgt << ")=" << d);
    }
}

// map the gate/operands of a single gate
// - if necessary, insert swaps
// - if necessary, create new gates
void MapGate(ql::gate* gp)
{
    auto& q = gp->operands;
    size_t operandCount = q.size();

    DOUT("MapGate: " << gp->qasm() );
    if (operandCount > 2)
    {
        EOUT(" gate: " << gp->qasm() << " has more than 2 operand qubits; please decompose such gates first before mapping.");
        throw ql::exception("Error: gate with more than 2 operand qubits; please decompose such gates first before mapping.", false);
    }

    // for each two-qubit gate, make the mapping right, if necessary by inserting swaps
    if (operandCount == 2)
    {
        auto mapopt = ql::options::get("mapper");
        if (mapopt == "base"|| mapopt == "baserc")
        {
            MapBase(gp);
        }
        else if (mapopt == "minextend" || mapopt == "minextendrc")
        {
            MapMinExtend(gp);
        }
        else
        {
            EOUT("Unknown value of option 'mapper'='" << mapopt << "'.");
            throw ql::exception("Error: unknown value of mapper option.", false);
        }
    }

    // with mapping done, insert the gate itself
    // devirtualization of this gate maps its qubit operands and optionally updates its gate name
    // when the gate name was updated, a new gate with that name is created;
    // when that new gate is a composite gate, it is decomposed
    // the resulting gate/expansion (anyhow a sequence of gates) is collected in circ
    ql::circuit circ;   // result of devirtualization
    mainPast.DeVirtualize(gp, circ);
    for (auto newgp : circ)
    {
        // DOUT(" ... mapped gate: " << newgp->qasm() );
        mainPast.AddAndSchedule(newgp);
    }
}

// map the circuit's gates in the provided context (v2r and fc maps)
void MapGates(ql::circuit& circ, std::string& kernel_name)
{
    // mainPast.Print("start mapping");

    ql::circuit outCirc;        // output gate stream, mapped; will be swapped with circ on return
    mainPast.Output(outCirc);       // past window will flush into outCirc

    for( auto & gp : circ )
    {
        switch(gp->type())
        {
        case ql::__classical_gate__:
            // flush Past first because Past should only contain quantum gates
            mainPast.Flush();
            // map qubit use of any classical instruction????
            outCirc.push_back(gp);
            break;
        case ql::__wait_gate__:
            break;
        default:    // quantum gate
            MapGate(gp);
            break;
        }
    }
    mainPast.Flush();

    // mainPast.Print("end mapping");

    // replace circ (the IR's main circuit) by the newly computed outCirc
    // outCirc gets the old circ and is destroyed when leaving scope
    // DOUT("... swapping outCirc with circ");
    circ.swap(outCirc);
}

public:
// retrieve number of swaps added
void GetNumberOfSwapsAdded(size_t & sa)
{
    sa = mainPast.NumberOfSwapsAdded();
}

// retrieve number of moves added
void GetNumberOfMovesAdded(size_t & sa)
{
    sa = mainPast.NumberOfMovesAdded();
}

// decompose all gates with names ending in _prim
// by replacing it by a new copy of this gate with as name _prim replaced by _dprim
// and decomposing it according to the .json file gate decomposition
//
// so:  this decomposes swap_prim to whatever is specified in .json gate decomposition behind swap_dprim
// and: this decomposes cnot_prim to whatever is specified in .json gate decomposition behind cnot_dprim
void Decomposer(ql::circuit& circ)
{
    DOUT("Decompose circuit ...");

    ql::circuit outCirc;        // output gate stream
    mainPast.Output(outCirc);   // past window will flush into outCirc
    for( auto & gp : circ )
    {
        ql::circuit tmpCirc;
        DOUT("mainPast.Init in Decomposer not doing it");
        mainPast.Decompose(gp, tmpCirc);
        for (auto newgp : tmpCirc)
        {
            mainPast.AddAndSchedule(newgp);
        }
    }
    mainPast.Flush();

    circ.swap(outCirc);

    DOUT("Decompose circuit [DONE]");
}   // end Decomposer

// alternative bundler using gate->cycle attribute instead of lemon's cycle map
// it assumes that the gate->cycle attribute reflect the cycle assignment of a particular schedule
ql::ir::bundles_t Bundler(ql::circuit& circ)
{
    ql::ir::bundles_t bundles;

    typedef std::vector<ql::gate*> insInOneCycle;
    std::map<size_t,insInOneCycle> insInAllCycles;

    DOUT("Bundler ...");
    size_t TotalCycles = 0;
    for ( auto & gp : circ)
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

/**
 * qasm
 * copied and shrunk from develop kernel.h
 */
std::string qasm(ql::circuit& c, size_t nqubits, std::string& name)
{
    std::stringstream ss;
    ss << "version 1.0\n";
    ss << "qubits " << nqubits << "\n";
    ss << "." << name << "\n";

    for(size_t i=0; i<c.size(); ++i)
    {
        ss << "    " << c[i]->qasm() << "\n";
    }

    return ss.str();
}

// map kernel's circuit in current mapping context as left by initialization and earlier kernels
void MapCircuit(size_t& kernel_qubits, ql::circuit& circ, std::string& kernel_name)
{
    DOUT("==================================");
    DOUT("Mapping circuit ...");
    DOUT("... kernel original virtual number of qubits=" << kernel_qubits);

#ifdef INITIALPLACE
    std::string initialplaceopt = ql::options::get("initialplace");
    if("yes" == initialplaceopt)
    {
        ip.Init(&grid, &platform);

        Virt2Real   v2r;
        DOUT("InitialPlace copy in current Virt2Real mapping ...");
        mainPast.GetV2r(v2r);
        // compute mapping using ip model, may fail
        ipr_t ipok;
        ip.Place(circ, v2r, ipok);
        if (ipok == ipr_newmap)
        {
            DOUT("InitialPlace result is used to update Virt2Real mapping");
            // replace current mapping of mainPast by the result of initial placement
            mainPast.SetV2r(v2r);
        }
        else
        {
            DOUT("InitialPlace: don't use result; continue with current mapping");
        }
    }
#endif
    MapGates(circ, kernel_name);

    std::string mapdecomposeropt = ql::options::get("mapdecomposer");
    if("yes" == mapdecomposeropt)
    {
        Decomposer(circ);   // decompose to primitives as specified in the config file
    }

    kernel_qubits = nqbits; // bluntly, so that all kernels get the same qubit_count

    DOUT("Mapping circuit [DONE]");
    DOUT("==================================");
}   // end MapCircuit

// initialize mapper for whole program
// lots could be split off for the whole program, once that is needed
void Init(const ql::quantum_platform& p)
{
    // DOUT("Mapping initialization ...");
    // DOUT("... Grid initialization: platform qubits->coordinates, ->neighbors, distance ...");
    platform = p;
    nqbits = p.qubit_number;
    // DOUT("... platform/real number of qubits=" << nqbits << ");
    cycle_time = p.cycle_time;

    grid.Init(&platform);

    mainPast.Init(&platform);

    // DOUT("Mapping initialization [DONE]");
}   // end Init


};  // end class Mapper

#endif
