/**
 * @file   clifford.h
 * @date   05/2019
 * @author Hans van Someren
 * @brief  clifford sequence optimizer
 */
#ifndef CLIFFORD_H
#define CLIFFORD_H

#include "utils.h"
#include "circuit.h"
#include "kernel.h"


namespace ql
{

/*
 * clifford sequence optimizer
 */
class Clifford
{
public:

    void Optimize(quantum_kernel& kernel, std::string fromwhere)
    {
        nq = kernel.qubit_count;
        ct = kernel.cycle_time;
        DOUT("Clifford " << fromwhere << " on kernel " << kernel.name << " ...");

        // copy circuit kernel.c to take input from
        // output will fill kernel.c again
        ql::circuit input_circuit = kernel.c;
        kernel.c.clear();

        cliffstate.resize(nq, 0);       // 0 is identity; for all qubits state is identity
        cliffcycles.resize(nq, 0);      // for all qubits, no accumulated cycles
        total_saved = 0;                // reset saved, just for reporting

        for (auto gp: input_circuit)
        {
            DOUT("... gate: " << gp->qasm());

            if (gp->type() == ql::gate_type_t::__classical_gate__
                || gp->operands.size() == 0
               )
            {
                // classical gates
                // and quantum gates like wait/display without operands
                // interpret cliffstate and create corresponding gate sequence, for all qubits
                sync_all(kernel);
                kernel.c.push_back(gp);
            }
            else if (gp->operands.size() != 1)                      // gates like CNOT/CZ/TOFFOLI
            {
                // non-unary quantum gates like wait/cnot/cz/toffoli
                // interpret cliffstate and create corresponding gate sequence, for each operand qubit
                for (auto q: gp->operands)
                {
                    sync(kernel, q);
                }
                kernel.c.push_back(gp);
            }
            else
            {
                // unary quantum gates like x/y/z/h/xm90/y90/s/wait/meas/prepz
                size_t q = gp->operands[0];
                std::string gname = gp->name;
                int cl = string2c(gname);
                if (cl != -1)
                {
                    // unary quantum clifford gates like x/y/z/h/xm90/y90/s/...
                    // don't copy gate to output but accumulate gate in cliffstate
                    // also record accumulated cycles to compute savings
                    cliffcycles[q] += (gp->duration+ct-1)/ct;
                    int s = cliffstate[q];
                    DOUT("... from " << c2string(s) << " to " << c2string(clifftrans[s][cl]));
                    cliffstate[q]  = clifftrans[s][cl];
                }
                else
                {
                    // unary quantum non-clifford gates like wait/meas/prepz/...
                    // interpret cliffstate and create corresponding gate sequence, for this operand qubit
                    sync(kernel, q);
                    kernel.c.push_back(gp);
                }
            }
            DOUT("... gate: " << gp->qasm() << " DONE");
        }
        sync_all(kernel);
        DOUT("Clifford " << fromwhere << " on kernel " << kernel.name << " saved " << total_saved << " cycles [DONE]");
    }

private:
    size_t  nq;
    size_t  ct;
    std::vector<int>    cliffstate;                    // current accumulated clifford state per qubit
    std::vector<size_t> cliffcycles;                   // current accumulated clifford cycles per qubit
    size_t  total_saved;                               // total number of cycles saved per kernel

    // create gate sequences for all accumulated cliffords, output them and reset state
    void sync_all(quantum_kernel& k)
    {
        DOUT("... sync_all");
        for (size_t q = 0; q < nq; q++)
        {
            sync(k, q);
        }
        DOUT("... sync_all DONE");
    }

    // create gate sequence for accumulated cliffords of qubit q, output it and reset state
    void sync(quantum_kernel& k, size_t q)
    {
        int s = cliffstate[q];
        if (s != 0)
        {
            DOUT("... sync q[" << q << "]: generating clifford " << c2string(s));
            k.clifford(s, q);          // generates clifford(s) in kernel.c
            size_t  acc_cycles = cliffcycles[q];
            size_t  ins_cycles = c2cycles(s);
            DOUT("... qubit q[" << q << "]: accumulated: " << acc_cycles << ", inserted: " << ins_cycles);
            if (acc_cycles > ins_cycles) DOUT("... qubit q[" << q << "]: saved " << (acc_cycles-ins_cycles) << " cycles");
            if (acc_cycles < ins_cycles) DOUT("... qubit q[" << q << "]: additional " << (ins_cycles-acc_cycles) << " cycles");
            total_saved += acc_cycles-ins_cycles;
        }
        cliffstate[q] = 0;
        cliffcycles[q] = 0;
    }

    // clifford state transition [from state][accumulating sequence represented as state] => new state
    const int clifftrans[24][24] = {
        {  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23 },
        {  1, 2, 0,10,11, 9, 4, 5, 3, 7, 8, 6,23,21,22,14,12,13,20,18,19,17,15,16 },
        {  2, 0, 1, 8, 6, 7,11, 9,10, 5, 3, 4,16,17,15,22,23,21,19,20,18,13,14,12 },
        {  3, 4, 5, 0, 1, 2, 9,10,11, 6, 7, 8,15,16,17,12,13,14,21,22,23,18,19,20 },
        {  4, 5, 3, 7, 8, 6, 1, 2, 0,10,11, 9,20,18,19,17,15,16,23,21,22,14,12,13 },
        {  5, 3, 4,11, 9,10, 8, 6, 7, 2, 0, 1,13,14,12,19,20,18,22,23,21,16,17,15 },
        {  6, 7, 8, 9,10,11, 0, 1, 2, 3, 4, 5,18,19,20,21,22,23,12,13,14,15,16,17 },
        {  7, 8, 6, 4, 5, 3,10,11, 9, 1, 2, 0,17,15,16,20,18,19,14,12,13,23,21,22 },
        {  8, 6, 7, 2, 0, 1, 5, 3, 4,11, 9,10,22,23,21,16,17,15,13,14,12,19,20,18 },
        {  9,10,11, 6, 7, 8, 3, 4, 5, 0, 1, 2,21,22,23,18,19,20,15,16,17,12,13,14 },
        { 10,11, 9, 1, 2, 0, 7, 8, 6, 4, 5, 3,14,12,13,23,21,22,17,15,16,20,18,19 },
        { 11, 9,10, 5, 3, 4, 2, 0, 1, 8, 6, 7,19,20,18,13,14,12,16,17,15,22,23,21 },
        { 12,13,14,21,22,23,18,19,20,15,16,17, 0, 1, 2, 9,10,11, 6, 7, 8, 3, 4, 5 },
        { 13,14,12,16,17,15,22,23,21,19,20,18, 5, 3, 4, 2, 0, 1, 8, 6, 7,11, 9,10 },
        { 14,12,13,20,18,19,17,15,16,23,21,22,10,11, 9, 4, 5, 3, 7, 8, 6, 1, 2, 0 },
        { 15,16,17,18,19,20,21,22,23,12,13,14, 3, 4, 5, 6, 7, 8, 9,10,11, 0, 1, 2 },
        { 16,17,15,13,14,12,19,20,18,22,23,21, 2, 0, 1, 5, 3, 4,11, 9,10, 8, 6, 7 },
        { 17,15,16,23,21,22,14,12,13,20,18,19, 7, 8, 6, 1, 2, 0,10,11, 9, 4, 5, 3 },
        { 18,19,20,15,16,17,12,13,14,21,22,23, 6, 7, 8, 3, 4, 5, 0, 1, 2, 9,10,11 },
        { 19,20,18,22,23,21,16,17,15,13,14,12,11, 9,10, 8, 6, 7, 2, 0, 1, 5, 3, 4 },
        { 20,18,19,14,12,13,23,21,22,17,15,16, 4, 5, 3,10,11, 9, 1, 2, 0, 7, 8, 6 },
        { 21,22,23,12,13,14,15,16,17,18,19,20, 9,10,11, 0, 1, 2, 3, 4, 5, 6, 7, 8 },
        { 22,23,21,19,20,18,13,14,12,16,17,15, 8, 6, 7,11, 9,10, 5, 3, 4, 2, 0, 1 },
        { 23,21,22,17,15,16,20,18,19,14,12,13, 1, 2, 0, 7, 8, 6, 4, 5, 3,10,11, 9 }
    };

    // find the clifford state from identity to given clifford gate by name
    int string2c(std::string gname)
    {
        if (gname == "identity")    return 0;
        if (gname == "i")           return 0;
        if (gname == "pauli_x")     return 3;
        if (gname == "x")           return 3;
        if (gname == "rx")          return 3;
        if (gname == "rx180")       return 3;
        if (gname == "pauli_y")     return 6;
        if (gname == "y")           return 6;
        if (gname == "ry")          return 6;
        if (gname == "ry180")       return 6;
        if (gname == "pauli_z")     return 9;
        if (gname == "z")           return 9;
        if (gname == "rz")          return 9;
        if (gname == "hadamard")    return 12;
        if (gname == "h")           return 12;
        if (gname == "mrx90")       return 13;
        if (gname == "s")           return 14;
        if (gname == "mry90")       return 15;
        if (gname == "rx90")        return 16;
        if (gname == "ry90")        return 21;
        if (gname == "sdag")        return 23;
        return -1;
    }

    // find the duration of the gate sequence corresponding to given clifford state
    // should be implemented using configuration file, searching for created gates and retrieving durations
    size_t c2cycles(int cl)
    {
        switch(cl) {
        case 0 : return 0;
        case 1 : return 2;
        case 2 : return 2;
        case 3 : return 1;
        case 4 : return 2;
        case 5 : return 2;
        case 6 : return 1;
        case 7 : return 2;
        case 8 : return 2;
        case 9 : return 2;
        case 10: return 2;
        case 11: return 2;
        case 12: return 2;
        case 13: return 1;
        case 14: return 3;
        case 15: return 1;
        case 16: return 1;
        case 17: return 3;
        case 18: return 2;
        case 19: return 2;
        case 20: return 3;
        case 21: return 1;
        case 22: return 2;
        case 23: return 3;
        }
        return 100;
    }

    // return the gate sequence as string for debug output corresponding to given clifford state
    std::string c2string(int cl)
    {
        switch(cl) {
        case 0 : return("[id;]");
        case 1 : return("[y90; x90;]");
        case 2 : return("[xm90; ym90;]");
        case 3 : return("[x180;]");
        case 4 : return("[ym90; xm90;]");
        case 5 : return("[x90; ym90;]");
        case 6 : return("[y180;]");
        case 7 : return("[ym90; x90;]");
        case 8 : return("[x90; y90;]");
        case 9 : return("[x180; y180;]");
        case 10: return("[y90; xm90;]");
        case 11: return("[xm90; y90;]");
        case 12: return("[y90; x180;]");
        case 13: return("[xm90;]");
        case 14: return("[x90; ym90; xm90;]");
        case 15: return("[ym90;]");
        case 16: return("[x90;]");
        case 17: return("[x90; y90; x90;]");
        case 18: return("[ym90; x180;]");
        case 19: return("[x90; y180;]");
        case 20: return("[x90; ym90; x90;]");
        case 21: return("[y90;]");
        case 22: return("[xm90; y180;]");
        case 23: return("[x90; y90; xm90;]");
        }
        return "[invalid clifford sequence]";
    }
};
}

#endif // CLIFFORD_H
