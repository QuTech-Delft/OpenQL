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

class Mapper
{
private:

    size_t nQbits;
    					// typedef std::vector<gate*> circuit; (see circuit.h)
    ql::circuit& inCirc;		// ref to input gate stream, not mapped; at end updated
    size_t cycle_time;
    ql::quantum_platform platform;

    size_t nx;				// length of x dimension (x coordinates count 0..nx-1)
    size_t ny;				// length of y dimension (y coordinates count 0..ny-1)
    std::map<size_t,size_t> x;		// x[i] is x coordinate of qubit i
    std::map<size_t,size_t> y;		// y[i] is y coordinate of qubit i
    typedef std::list<size_t> neighbors_t; // neighbors is a list of qubits
    std::map<size_t,neighbors_t> nb;	// nb[i] is neighbors of qubit i, a list of qubits
					// one-step reachable (over an edge) qubits from qubit i

    std::vector<size_t> virt2Real;	// virt2Real[virtual qubit index] -> real qubit index

    // return the virtual qubit index that virt2Real maps to real qubit r
    size_t real2Virt(size_t r)
    {
	for (size_t v=0; v<nQbits; v++)
	{
	    if (virt2Real[v] == r) return v;
	}
	assert(0);
	return 0;
    }

    // r0 and r1 are real qubit indices
    // after a swap(r0,r1) their states were exchanged, 
    // so when v0 was in r0 and v1 was in r1, now v0 is in r1 and v1 is in r0
    // update virt2Real accordingly
    void virt2RealSwap(size_t r0, size_t r1)
    {
	size_t v0 = real2Virt(r0);
	size_t v1 = real2Virt(r1);
	DOUT("... remap("<< v0<<"->"<<r0<<","<<v1<<"->"<<r1<<")->("<<v0<<"->"<<r1<<","<<v1<<"->"<<r0<<" )");
	virt2Real[v0] = r1;
	virt2Real[v1] = r0;
    }

    void printVirt2Real(std::string s)
    {
        std::cout << "... virt2real(v->r) " << s << ":";
	for (size_t v=0; v<nQbits; v++)
	{
	    size_t r = virt2Real[v];
	    std::cout << " (" << v << "->" << r << ")";
	}
        std::cout << std::endl;
#ifdef debug
        std::cout << "... real2virt(r->v) " << s << ":";
	for (size_t r=0; r<nQbits; r++)
	{
	    size_t v = real2Virt(r);
	    std::cout << " (" << r << "->" << v << ")";
	}
        std::cout << std::endl;
#endif	// debug
    }

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
    // Mapper constructor initializes program-wide data, e.g. grid related
    // Mapper.Init initializes data to map a particular kernel
    Mapper( size_t nqbits, ql::circuit& ckt, ql::quantum_platform pf) :
	nQbits(nqbits), inCirc(ckt), cycle_time(platform.cycle_time), platform(pf)
    {
        DOUT("==================================");
        DOUT("Mapper creation ...");
        DOUT("... Grid initialization: qubits->coordinates, ->neighbors, distance ...");

        nx = platform.topology["x_size"];
        ny = platform.topology["y_size"];
        DOUT("nx=" << nx << "; ny=" << ny);

	for (auto & aqbit : platform.topology["qubits"] )
	{
	    size_t qi = aqbit["id"];
	    size_t qx = aqbit["x"];
	    size_t qy = aqbit["y"];
	    x[qi] = qx;
	    y[qi] = qy;
	    assert(0<=qi && qi<nqbits);
	    assert(0<=qx && qx<nx);
	    assert(0<=qy && qy<ny);
	}
	for (auto & anedge : platform.topology["edges"] )
	{
	    size_t es = anedge["src"];
	    size_t ed = anedge["dst"];
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
#endif	// debug

        DOUT("... Mapper creation [DONE]");
    }

    void Init()
    {
        DOUT("Initialize mapper data: map(virtual->real)");
        // DOUT("... start with trivial mapping (virtual==real), nQbits=" << nQbits);
	virt2Real.resize(nQbits);
	for (size_t i=0; i<nQbits; i++)
	{
	    virt2Real[i] = i;
	}
        DOUT("Mapper initialization [DONE]");
    }

    void MapCircuit()
    {
        DOUT("Mapping circuit ...");
        printVirt2Real("starting mapping");
        ql::circuit outCirc;	// output gate stream, mapped; will be swapped with inCirc on return

        for( auto g : inCirc )
        {
            auto& q = g->operands;
            size_t operandCount = q.size();
	    if (operandCount == 1)
	    {
                DOUT(" gate: " << g->qasm() << " (virt " << q[0] << ")");
	        q[0] = virt2Real[q[0]];
                DOUT(" ... mapped gate: " << g->qasm() << " (real " << g->operands[0] << ")");
		outCirc.push_back(g);
	    }
	    else
	    {
		size_t rq0 = virt2Real[q[0]];
		size_t rq1 = virt2Real[q[1]];
                DOUT(" gate: " << g->qasm() << " (virt " << q[0] << ", virt " << q[1] << ") in (real " << rq0 << ", real " << rq1 << ")" );
		size_t d = distance(rq0,rq1);
                // DOUT(" ... distance(real " << rq0 << ", real " << rq1 << ")=" << d);
		while (d > 1)
		{
		    for( auto rq0nb : nb[rq0] )
		    {
			size_t dnb = distance(rq0nb,rq1);
		        if (dnb < d)
			{
                            // DOUT(" ... distance(real " << rq0nb << ", real " << rq1 << ")=" << dnb);
        		    outCirc.push_back(new ql::swap(rq0, rq0nb) );
			    virt2RealSwap(rq0, rq0nb);
                            printVirt2Real("mapping after swap");
			    rq0 = rq0nb;
			    break;
			}
		    }
		    d = distance(rq0,rq1);
                    // DOUT(" ... new distance(real " << rq0 << ", real " << rq1 << ")=" << d);
		}
	        q[0] = virt2Real[q[0]];
	        q[1] = virt2Real[q[1]];
                DOUT(" ... mapped gate: " << g->qasm() << " (real " << g->operands[0] << ", real " << g->operands[1] << ")");
		outCirc.push_back(g);
	    }
        } // end of instruction for

        printVirt2Real("end mapping");

        DOUT("... swapping outCirc with inCirc");
	inCirc.swap(outCirc);

        DOUT("... Start circuit after mapping:");
        for( auto g : inCirc )
	{
            DOUT("\t" << g->qasm() );
	}
        DOUT("... End circuit after mapping");
        DOUT("Mapping circuit [DONE]");
        DOUT("==================================");
    }
};

#endif
