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

class virt2real
{
private:
size_t              nqbits;                // size of the map
std::vector<size_t> v2rMap;                // v2rMap[virtual qubit index] -> real qubit index

public:

size_t& operator[] (size_t v)
{
    return v2rMap[v];
}

const size_t& operator[] (size_t v) const
{
    return v2rMap[v];
}

void Resize(size_t s)
{
    nqbits = s;
    v2rMap.resize(s);
}

// return the virtual qubit index that v2r maps to real qubit r
size_t GetVirt(size_t r)
{
    for (size_t v=0; v<nqbits; v++)
    {
        if (v2rMap[v] == r) return v;
    }
    assert(0);
    return 0;
}

// r0 and r1 are real qubit indices
// after a swap(r0,r1) their states were exchanged,
// so when v0 was in r0 and v1 was in r1, now v0 is in r1 and v1 is in r0
// update v2r accordingly
void Swap(size_t r0, size_t r1)
{
    size_t v0 = GetVirt(r0);
    size_t v1 = GetVirt(r1);
    DOUT("... remap from ("<< v0<<"->"<<r0<<","<<v1<<"->"<<r1<<") to ("<<v0<<"->"<<r1<<","<<v1<<"->"<<r0<<" )");
    v2rMap[v0] = r1;
    v2rMap[v1] = r0;
}

void Print(std::string s)
{
    std::cout << "... virt2real(v->r) " << s << ":";
    for (size_t v=0; v<nqbits; v++)
    {
        size_t r = v2rMap[v];
        std::cout << " (" << v << "->" << r << ")";
    }
    std::cout << std::endl;
#ifdef debug
    std::cout << "... real2virt(r->v) " << s << ":";
    for (size_t r=0; r<nqbits; r++)
    {
        size_t v = GetVirt(r);
        std::cout << " (" << r << "->" << v << ")";
    }
    std::cout << std::endl;
#endif        // debug
}

};   // end class virt2real


class Mapper
{
private:

size_t nqbits;                          // number of qubits in the system
size_t cycle_time;                      // length in ns of a single cycle; is divisor of duration in ns to convert it to cycles
ql::quantum_platform platform;          // current platform: topology and gates' duration

size_t nx;                              // length of x dimension (x coordinates count 0..nx-1)
size_t ny;                              // length of y dimension (y coordinates count 0..ny-1)
std::map<size_t,size_t> x;              // x[i] is x coordinate of qubit i
std::map<size_t,size_t> y;              // y[i] is y coordinate of qubit i
typedef std::list<size_t> neighbors_t;  // neighbors is a list of qubits
std::map<size_t,neighbors_t> nb;        // nb[i] is list of neighbor qubits of qubit i

virt2real   v2r;

// distance between two qubits
// implementation is for "cross" and "star" grids and assumes bidirectional edges and convex grid
// for "plus" grids, replace "std::max" by "+"
size_t distance(size_t from, size_t to)
{
    return std::max(
               std::abs( ptrdiff_t(x[to]) - ptrdiff_t(x[from]) ),
               std::abs( ptrdiff_t(y[to]) - ptrdiff_t(y[from]) ));
}

public:


// Mapper constructor initializes constant program-wide data, e.g. grid related
Mapper( size_t nq, ql::quantum_platform pf) :
    nqbits(nq), cycle_time(platform.cycle_time), platform(pf)
{
    DOUT("==================================");
    DOUT("Mapper creation ...");
    DOUT("... Grid initialization: qubits->coordinates, ->neighbors, distance ...");

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

    DOUT("Mapper creation [DONE]");
}

// initialize program-wide data that is passed around between kernels
// initial program-wide mapping could be computed here
void MapInit()
{
    DOUT("Mapping initialization ...");
    DOUT("... Initialize map(virtual->real)");
    DOUT("... with trivial mapping (virtual==real), nqbits=" << nqbits);
    v2r.Resize(nqbits);
    for (size_t i=0; i<nqbits; i++)
    {
        v2r[i] = i;
    }
    v2r.Print("starting mapping");
    DOUT("Mapping initialization [DONE]");
}

// map kernel's circuit in current mapping context as left by initialization and earlier kernels
void MapCircuit(ql::circuit& inCirc)
{
    DOUT("Mapping circuit ...");
    v2r.Print("start mapping");

    ql::circuit outCirc;        // output gate stream, mapped; will be swapped with inCirc on return

    for( auto &g : inCirc )
    {
        auto& q = g->operands;
        size_t operandCount = q.size();
        if (operandCount == 1)
        {
            DOUT(" gate: " << g->qasm() );
            q[0] = v2r[q[0]];
            DOUT(" ... mapped gate: " << g->qasm() );
            outCirc.push_back(g);
        }
        else if (operandCount == 2)
        {
            size_t rq0 = v2r[q[0]];
            size_t rq1 = v2r[q[1]];
            size_t d = distance(rq0,rq1);
            DOUT(" gate: " << g->qasm() << " in real (q" << rq0 << ",q" << rq1 << ") at distance=" << d );
            while (d > 1)
            {
                for( auto rq0nb : nb[rq0] )
                {
                    size_t dnb = distance(rq0nb,rq1);
                    if (dnb < d)
                    {
                        // DOUT(" ... distance(real " << rq0nb << ", real " << rq1 << ")=" << dnb);
                        outCirc.push_back(new ql::swap(rq0, rq0nb) );
                        v2r.Swap(rq0, rq0nb);
                        v2r.Print("mapping after swap");
                        rq0 = rq0nb;
                        break;
                    }
                }
                d = distance(rq0,rq1);
                // DOUT(" ... new distance(real " << rq0 << ", real " << rq1 << ")=" << d);
            }
            q[0] = v2r[q[0]];
            q[1] = v2r[q[1]];
            DOUT(" ... mapped gate: " << g->qasm() );
            outCirc.push_back(g);
        }
        else
        {
            EOUT(" gate: " << g->qasm() << " has more than 2 operand qubits; this is not supported by the mapper.");
            throw ql::exception("Error: gates with more than 2 operand qubits not supported by mapper.", false);
        }
    }   // end for all instructions in circuit

    v2r.Print("end mapping");

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
