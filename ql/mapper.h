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

public:
    Mapper( size_t nQubits, ql::circuit& ckt, ql::quantum_platform platform) :
	num_qubits(nQubits), circ(ckt), cycle_time(platform.cycle_time) { }

    void Init()
    {
        DOUT("Mapper initialization ...");
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
