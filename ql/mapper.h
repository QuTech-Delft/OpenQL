/**
 * @file   mapper.h
 * @date   06/2018
 * @author Hans van Someren
 * @brief  mapping qubits
 */

#ifndef _MAPPER_H
#define _MAPPER_H

// #include <assert.h>

#include "ql/utils.h"
#include "ql/gate.h"
#include "ql/circuit.h"
#include "ql/ir.h"
// #include "ql/scheduler.h"
// #include "ql/arch/cc_light_resource_manager.h"

// using namespace std;

// =========================================================================================
class Virt2Real
{
private:

    size_t              nq;                    // size of the map; after initialization, will always be the same
    std::vector<size_t> v2rMap;                // v2rMap[virtual qubit index] -> real qubit index

public:

// default constructor, creating empty Virt2Real
Virt2Real() : nq(0)
{
    DOUT("Virt2Real default constructor");
}

// expand to desired size and initialize to trivial (1-1) mapping
void
Init(size_t n)
{
    DOUT("Virt2Real::Init(n=" << n << "), initializing 1-1 mapping");
    nq = n;
    v2rMap.resize(n);
    for (size_t i=0; i<n; i++)
    {
        v2rMap[i] = i;
    }
}

// copy constructor, copies original to continue with original's mapping
// synthesized copy constructor would do but this one prints for debugging
Virt2Real(Virt2Real &orig) : nq(orig.nq), v2rMap(orig.v2rMap)
{
    DOUT("Virt2Real copy constructor");
}

// map virtual qubit index to real qubit index
size_t& operator[] (size_t v)
{
    return v2rMap[v];
}

// map real qubit to the virtual qubit index that is mapped to it (i.e. backward map)
size_t GetVirt(size_t r)
{
    for (size_t v=0; v<nq; v++)
    {
        if (v2rMap[v] == r) return v;
    }
    assert(0);
    return 0;
}

// r0 and r1 are real qubit indices
// after a swap(r0,r1) gate application their states were exchanged,
// so when v0 was in r0 and v1 was in r1, then v0 is now in r1 and v1 is in r0
// update v2r accordingly
void Swap(size_t r0, size_t r1)
{
    size_t v0 = GetVirt(r0);
    size_t v1 = GetVirt(r1);
    DOUT("... swap virtual indices from ("<< v0<<"->"<<r0<<","<<v1<<"->"<<r1<<") to ("<<v0<<"->"<<r1<<","<<v1<<"->"<<r0<<" )");
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
class FreeCycle
{
private:

    size_t              nq;     // size of the map; after initialization, will always be the same
    size_t              ct;     // multiplication factor from cycles to nano-seconds (unit of duration)
    std::vector<size_t> fc;     // fc[real qubit index i]: qubit i is free from this cycle on

public:

// first-time constructor, creates an empty but non-sense FreeCycle
FreeCycle(): nq(0), ct(1)
{
    DOUT("FreeCycle default constructor");
}

void Init(size_t n, size_t c)
{
    DOUT("FreeCycle::Init(n=" << n << ", c=" << c << "), initializing to all 0 cycles");
    nq = n;
    ct = c;
    fc.resize(n, 0);
}

// copy constructor, copies original to continue with original's cycle values
// synthesized copy constructor would do but this one prints for debugging
FreeCycle(FreeCycle &orig) : nq(orig.nq), ct(orig.ct), fc(orig.fc)
{
    DOUT("FreeCycle copy constructor");
}

// access free cycle value of qubit i
size_t& operator[] (size_t i)
{
    return fc[i];
}

// depth of the FreeCycle map
// equals the max of all entries minus the min of all entries
size_t Depth()
{
    size_t  minFreeCycle = SIZE_MAX;
    size_t  maxFreeCycle = 0;
    for (auto& v : fc)
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

// max of the FreeCycle map
// equals the max of all entries
size_t Max()
{
    size_t  maxFreeCycle = 0;
    for (auto& v : fc)
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
        size_t v = fc[i];
        std::cout << " " << v;
    }
    std::cout << std::endl;
}

// schedule gate g in the FreeCycle map; return its start cycle
// gate operands are real qubit indices
size_t Add(ql::gate *g)
{
    size_t  startCycle;
    auto& q = g->operands;
    size_t operandCount = q.size();
    if (operandCount == 1)
    {
        startCycle = fc[q[0]];
        fc[q[0]] = startCycle + (g->duration+ct-1)/ct;   // rounded-up unsigned integer division 
    }
    else // if (operandCount == 2)
    {
        startCycle = std::max<size_t>(fc[q[0]], fc[q[1]]);
        fc[q[0]] = startCycle + (g->duration+ct-1)/ct;   // rounded-up unsigned integer division
        fc[q[1]] = fc[q[0]];
    }
    // else
    // {
    //     assert(0);  // has already been checked when reading mapper input
    // }
    DOUT(" gate: " << g->qasm() << "@FreeCycle=" << startCycle);
    return startCycle;
}

};  // end class FreeCycle

// =========================================================================================
class Past
{
private:

	size_t                  nq;         // width of Past in qubits
	size_t                  ct;         // cycle time, multiplier from cycles to nano-seconds
	Virt2Real               v2r;        // Virt2Real map applying to this Past
	FreeCycle               fc;         // FreeCycle map applying to this Past
	typedef ql::gate        *gate_p;
	std::list<gate_p>       lg;         // list of gates in this Past, ordered by their (start) cycle values
	std::map<gate_p,size_t> cycle;      // gate to cycle map, cycle value of each gate in lg: cycle[g]
    ql::circuit             *outCircp;  // output stream after past

public:

// first-time constructor
Past()
{
    DOUT("Pastconstructor");
}

// copy constructor, copies original to continue with original's cycle values
// synthesized copy constructor would do but this one prints for debugging
Past(Past &orig) : nq(orig.nq), ct(orig.ct), v2r(orig.v2r), fc(orig.fc), lg(orig.lg)
{
    DOUT("Past copy constructor");
}

// past initializer sets nq and ct and initializes all (default-constructed) composite members
void Init(size_t n, size_t c)
{
    DOUT("past::Init(n=" << n << ", c=" << c << ") ");
    nq = n;
    ct = c;
    v2r.Init(n);
    fc.Init(n,c);
    // lg is initialized to empty list
    // cycle is initialized to empty map
}

~Past()
{
}

void Print(std::string s)
{
    std::cout << "... Past " << s << ":";
    v2r.Print("");
    fc.Print("");
}

void Output(ql::circuit& ct)
{
    outCircp = &ct;
}

void AddSwap(size_t r0, size_t r1)
{
    gate_p  gp;

    gp = new ql::swap(r0,r1);
    gp->duration = 400;
    DOUT("... swap(q" << r0 << ",q" << r1 << ") with duration=" << gp->duration << ", cycles=" << ((gp->duration+ct-1)/ct) );
    v2r.Swap(r0,r1);
    Add(gp);
}

void Add(gate_p gp)
{
    size_t startCycle = fc.Add(gp);
    cycle[gp] = startCycle;
    DOUT("... add " << gp->qasm() << "@ startcycle=" << startCycle);

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
            lg.insert(rigp.base(), gp);
            break;
        }
    }
    // when list was empty or no element was found, just put it in front
    if (rigp == lg.rend())
    {
        lg.push_front(gp);
    }

#if debug
    std::cout << "... new schedule: ";
    for (auto g2p : lg)
    {
        if (g2p == gp)
        {
            std::cout << "[" << cycle[g2p] << "] ";
        }
        else
        {
            std::cout << "" << cycle[g2p] << " ";
        }
    }
    std::cout << std::endl;
#endif
}

size_t Map(size_t v)
{
    return v2r[v];
}

void Flush()
{
    for( auto gp : lg )
    {
        outCircp->push_back(gp);
    }
}

};  // end class Past

// =========================================================================================
class Mapper
{
private:

                                        // OpenQL wide configuration, all constant after initialization
size_t nqbits;                          // number of qubits in the system
size_t cycle_time;                      // length in ns of a single cycle; is divisor of duration in ns to convert it to cycles
ql::quantum_platform platform;          // current platform: topology and gates' duration

                                        // Grid configuration, all constant after initialization
size_t nx;                              // length of x dimension (x coordinates count 0..nx-1)
size_t ny;                              // length of y dimension (y coordinates count 0..ny-1)
std::map<size_t,size_t> x;              // x[i] is x coordinate of qubit i
std::map<size_t,size_t> y;              // y[i] is y coordinate of qubit i
typedef std::list<size_t> neighbors_t;  // neighbors is a list of qubits
std::map<size_t,neighbors_t> nb;        // nb[i] is list of neighbor qubits of qubit i

                                        // Mapper dynamic state
Past   past;                            // prime past window; all path alternatives start off as clones of it
                                        // Past contains gates of which schedule might influence path selected for mapping binary gates
                                        // It maintains for each qubit from which cycle on it is free, so that swap insertion
                                        // can exploit this to hide its overall circuit latency overhead by increasing ILP.
                                        // Also it maintains the 1 to 1 (reversible) virtual to real qubit map: all gates in past
                                        // and beyond are mapped and have real qubits as operands.
                                        // While experimenting with path alternatives, a clone is made of the prime past,
                                        // to insert swaps and evaluate the latency effects; note that inserting swaps changes mapping.

// initialize mapper internal grid maps from configuration
// this remains constant over multiple kernels on the same platform
void GridInit()
{
    nx = platform.topology["x_size"];
    ny = platform.topology["y_size"];
    DOUT("... nx=" << nx << "; ny=" << ny);

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
        nb[es].push_back(ed);
    }

#ifdef debug
    for (size_t i=0; i<nqbits; i++)
    {
        DOUT("qubit[" << i << "]: x=" << x[i] << "; y=" << y[i]);
        std::cout << "... connects to ";
        for (auto n : nb[i])
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
// implementation is for "cross" and "star" grids and assumes bidirectional edges and convex grid
// for "plus" grids, replace "std::max" by "+"
size_t GridDistance(size_t from, size_t to)
{
    return std::max(
               std::abs( ptrdiff_t(x[to]) - ptrdiff_t(x[from]) ),
               std::abs( ptrdiff_t(y[to]) - ptrdiff_t(y[from]) ));
}

// map the operands of a single gate
// if necessary, insert swaps
void MapGate(ql::gate* gp)
{
    auto& q = gp->operands;
    size_t operandCount = q.size();
    if (operandCount == 1)
    {
        DOUT(" gate: " << gp->qasm() );
        q[0] = past.Map(q[0]);
        DOUT(" ... mapped gate: " << gp->qasm() );
        past.Add(gp);
    }
    else if (operandCount == 2)
    {
        size_t rq0 = past.Map(q[0]);
        size_t rq1 = past.Map(q[1]);
        size_t d = GridDistance(rq0,rq1);
        DOUT(" gate: " << gp->qasm() << " in real (q" << rq0 << ",q" << rq1 << ") at distance=" << d );
        while (d > 1)
        {
            for( auto rq0nb : nb[rq0] )
            {
                size_t dnb = GridDistance(rq0nb,rq1);
                if (dnb < d)
                {
                    // DOUT(" ... distance(real " << rq0nb << ", real " << rq1 << ")=" << dnb);
                    past.AddSwap(rq0, rq0nb);
                    past.Print("mapping after swap");
                    rq0 = rq0nb;
                    break;
                }
            }
            d = GridDistance(rq0,rq1);
            // DOUT(" ... new distance(real " << rq0 << ", real " << rq1 << ")=" << d);
        }
        q[0] = past.Map(q[0]);
        q[1] = past.Map(q[1]);
        DOUT(" ... mapped gate: " << gp->qasm() );
        past.Add(gp);
    }
    else
    {
        EOUT(" gate: " << gp->qasm() << " has more than 2 operand qubits; please decompose such gates first before mapping.");
        throw ql::exception("Error: gate with more than 2 operand qubits; please decompose such gates first before mapping.", false);
    }
}

public:

// Mapper constructor initializes constant program-wide data, e.g. grid related
Mapper( size_t nq, ql::quantum_platform pf) :
    nqbits(nq), cycle_time(pf.cycle_time), platform(pf)
{
    DOUT("==================================");
    DOUT("Mapper creation ...");
    DOUT("... nqbits=" << nqbits << ", cycle_time=" << cycle_time);
    DOUT("... Grid initialization: qubits->coordinates, ->neighbors, distance ...");

    GridInit();

    DOUT("Mapper creation [DONE]");
}

// initialize program-wide data that is passed around between kernels
// initial program-wide mapping could be computed here
void MapInit()
{
    DOUT("Mapping initialization ...");
    DOUT("... Initialize map(virtual->real)");
    DOUT("... with trivial mapping (virtual==real), nqbits=" << nqbits);
    past.Init(nqbits, cycle_time);
    past.Print("initial mapping");
    DOUT("Mapping initialization [DONE]");
}

// map kernel's circuit in current mapping context as left by initialization and earlier kernels
void MapCircuit(ql::circuit& inCirc)
{
    DOUT("Mapping circuit ...");
    past.Print("start mapping");

    ql::circuit outCirc;        // output gate stream, mapped; will be swapped with inCirc on return
    past.Output(outCirc);       // past window will flush into outCirc

    for( auto gp : inCirc )
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
        //  past.Flush();
        //  deal with *gp
        // }
    }
    past.Flush();

    past.Print("end mapping");
    DOUT("... swapping outCirc with inCirc");
    inCirc.swap(outCirc);

    // DOUT("... Start circuit (size=" << inCirc.size() << ") after mapping:");
    // for( auto& g : inCirc )
    // {
        // DOUT("\t" << g->qasm() );
    // }
    // DOUT("... End circuit after mapping");
    DOUT("Mapping circuit [DONE]");
    DOUT("==================================");
}   // end MapCircuit

};  // end class Mapper

#endif
