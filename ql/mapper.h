/**
 * @file   mapper.h
 * @date   06/2018
 * @author Hans van Someren
 * @brief  mapping qubits
 */

#ifndef _MAPPER_H
#define _MAPPER_H

#include "ql/utils.h"
#include "ql/gate.h"
#include "ql/circuit.h"
#include "ql/ir.h"
#include "ql/scheduler.h"
#include "ql/arch/cc_light_resource_manager.h"

using namespace std;

class Mapper
{
private:

    size_t num_qubits;
    ql::circuit& circ;
    size_t cycle_time;

    size_t nx;				// length of x dimension (x coordinates count 0..nx-1)
    size_t ny;				// length of y dimension (y coordinates count 0..ny-1)
    std::map<size_t,size_t> x;		// x coordinate of qubit i
    std::map<size_t,size_t> y;		// y coordinate of qubit i
    typedef std::list<size_t> neighbors_t;	// list of neighbors
    std::map<size_t,neighbors_t> nb;		// neighbors of qubit i

public:
    Mapper( size_t nQubits, ql::circuit& ckt, ql::quantum_platform platform) :
	num_qubits(nQubits), circ(ckt), cycle_time(platform.cycle_time)
    {
        DOUT("Mapper creation ...");

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
	}
	for (auto & anedge : platform.topology["edges"] )
	{
	    size_t es = anedge["src"];
	    size_t ed = anedge["dst"];
	    nb[es].push_back(ed);
	}
	for (size_t i=0; i<num_qubits; i++)
	{
            DOUT("qubit[" << i << "]: x=" << x[i] << "; y=" << y[i]);
	    for (auto n : nb[i])
	    {
                DOUT("... connects to " << n);
	    }
	}
    }

    void Init()
    {
        DOUT("Grid configuration from json: grid size, nx, ny, map(coord->qubit), nqubits, map(qubit->coord) ...");
        DOUT("Grid initialization: neighbors, distance ...");
        DOUT("Initialize mapper data: map(virtual->real)");
        DOUT("Mapper initialization [DONE]");
    }

    void MapCircuit()
    {
        DOUT("Mapping circuit ...");
	DOUT("do ALAP schedule");
	DOUT("do initial map using ILP");
        for( auto ins : circ )
        {
            DOUT("Current instruction : " << ins->qasm());
	    DOUT("map instruction, inserting swaps when required");
	    DOUT("add instruction to new circuit or replace it");
        } // end of instruction for

        DOUT("Mapping circuit [DONE]");
    }

};

#endif
