/** \file
 * Clifford sequence optimizer.
 */

#include "clifford.h"

#include "utils/num.h"
#include "circuit.h"
#include "report.h"
#include "kernel.h"
#include "options.h"

namespace ql {

using namespace utils;

class Clifford {
public:

    void clifford_optimize_kernel(
        quantum_kernel &kernel,
        const quantum_platform &platform,
        const Str &passname
    ) {
        QL_DOUT("clifford_optimize_kernel()");

        nq = kernel.qubit_count;
        ct = kernel.cycle_time;
        QL_DOUT("Clifford " << passname << " on kernel " << kernel.name << " ...");

        // copy circuit kernel.c to take input from;
        // output will fill kernel.c again
        circuit input_circuit = kernel.c;
        kernel.c.clear();

        cliffstate.resize(nq, 0);       // 0 is identity; for all qubits accumulated state is set to identity
        cliffcycles.resize(nq, 0);      // for all qubits, no accumulated cycles
        total_saved = 0;                // reset saved, just for reporting

        /*
        The main idea of this optimization is that there are 24 clifford gates and these form a group,
        i.e. any sequence of clifford gates is in effect equivalent to one clifford from the group.

        Make a linear scan from begin to end over the circuit;
        attempt to find sequences of consecutive clifford gates operating on qubit q;
        these series can be interwoven, so have to be found in parallel.
        Each sequence can potentially be replaced by an equivalent shorter one from the group of 24 cliffords,
        reducing the number of cycles that the sequence takes, the circuit latency and the gate count.

        The clifford group is represented by:
        - Int string2cs(Str gname): the clifford state of a gate with the given name; identity is 0
        - a state diagram clifftrans[24][24] that represents for two given clifford (sequences),
          to which clifford the combination is equivalent to;
          so clifford(sequence1; sequence2) == clifftrans[clifford(sequence1)][clifford(sequence2)].
        - UInt cs2cycles(Int cs): the minimum number of cycles needed to implement a clifford of state cs
        - void k.clifford(Int csq, UInt q): generates minimal clifford sequence for state csq and qubit q

        Therefore, maintain for each qubit q while scanning:
        - cliffstate[q]:    clifford state of sequence until now per qubit; initially identity
        - cliffcycles[q]:   number of cycles of the sequence until now per qubit; initially 0
        Each time a clifford c is encountered for qubit q, the clifford c is incorporated into cliffstate[q]
        by making the transition: cliffstate[q] = clifftrans[cliffstate[q]][string2cs(c)],
        and updating cliffcycles[q].
        And when finding a gate that ends a sequence of cliffords ('synchronization point'),
        the minimal sequence corresponding to the accumulated sequence is output before the new gate.

        While scanning the circuit having accumulated the clifford state, for each next gate split out:
        - those potentially affecting all qubits: push out all state, clearing all accumulated state
        - those affecting a particular set of qubits: for those qubits, push out state, clearing their state
        - those affecting a single qubit but not being a clifford: push out state for that qubit, clearing it
        - those affecting a single qubit and being a conditional gate: push out state for that qubit, clearing it
        - remaining case is a single qubit clifford: add it to the state
        */
        for (auto gp : input_circuit) {
            QL_DOUT("... gate: " << gp->qasm());

            if (
                gp->type() == ql::gate_type_t::__classical_gate__   // classical gates (really being pessimistic here about these)
                || gp->operands.empty()                             // gates without operands which may affect ALL qubits
            ) {
                // sync all qubits: create gate sequences corresponding to what was accumulated in cliffstate, for all qubits
                sync_all(kernel);
                kernel.c.push_back(gp);
            } else if (gp->operands.size() != 1) {                 // gates like CNOT/CZ/TOFFOLI
                // sync particular qubits: create gate sequences corresponding to what was accumulated in cliffstate, for those particular operand qubits
                for (auto q : gp->operands) {
                    sync(kernel, q);
                }
                kernel.c.push_back(gp);
            } else {
                // unary quantum gates like x/y/z/h/xm90/y90/s/wait/meas/prepz
                UInt q = gp->operands[0];
                Str gname = gp->name;
                Int cs = string2cs(gname);
                if (
                    cs == -1                                        // non-clifford unary gates (wait, meas, prepz, ...)
                    || gp->is_conditional()                         // conditional unary (clifford) gates
                ) {
                    // sync particular single qubit: create gate sequence corresponding to what was accumulated in cliffstate, for this particular operand qubit
                    QL_DOUT("... unary gate not a clifford gate or conditional: " << gp->qasm());
                    sync(kernel, q);
                    kernel.c.push_back(gp);
                } else {
                    // unary quantum clifford gates like x/y/z/h/xm90/y90/s/...
                    // don't emit gate but accumulate gate in cliffstate
                    // also record accumulated cycles to compute savings
                    cliffcycles[q] += (gp->duration + ct - 1) / ct;
                    Int csq = cliffstate[q];
                    QL_DOUT("... from " << cs2string(csq) << " to " << cs2string(clifftrans[csq][cs]));
                    cliffstate[q] = clifftrans[csq][cs];
                }
            }
            QL_DOUT("... gate: " << gp->qasm() << " DONE");
        }
        sync_all(kernel);
        kernel.cycles_valid = false;

        QL_DOUT("Clifford " << passname << " on kernel " << kernel.name << " saved " << total_saved << " cycles [DONE]");
    }

private:
    UInt nq;
    UInt ct;
    Vec<Int> cliffstate; // current accumulated clifford state per qubit
    Vec<UInt> cliffcycles; // current accumulated clifford cycles per qubit
    UInt total_saved; // total number of cycles saved per kernel

    // create gate sequences for all accumulated cliffords, output them and reset state
    void sync_all(quantum_kernel &k) {
        QL_DOUT("... sync_all");
        for (UInt q = 0; q < nq; q++) {
            sync(k, q);
        }
        QL_DOUT("... sync_all DONE");
    }

    // create gate sequence for accumulated cliffords of qubit q, output it and reset state
    void sync(quantum_kernel &k, UInt q) {
        Int csq = cliffstate[q];
        if (csq != 0) {
            QL_DOUT("... sync q[" << q << "]: generating clifford " << cs2string(csq));
            k.clifford(csq, q);          // generates clifford(csq) in kernel.c
            UInt  acc_cycles = cliffcycles[q];
            UInt  ins_cycles = cs2cycles(csq);
            QL_DOUT("... qubit q[" << q << "]: accumulated: " << acc_cycles << ", inserted: " << ins_cycles);
            if (acc_cycles > ins_cycles) QL_DOUT("... qubit q[" << q << "]: saved " << (acc_cycles - ins_cycles) << " cycles");
            if (acc_cycles < ins_cycles) QL_DOUT("... qubit q[" << q << "]: additional " << (ins_cycles - acc_cycles) << " cycles");
            total_saved += acc_cycles-ins_cycles;
        }
        cliffstate[q] = 0;
        cliffcycles[q] = 0;
    }

    // clifford state transition [from state][accumulating sequence represented as state] => new state
    const Int clifftrans[24][24] = {
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
    static Int string2cs(const Str &gname) {
        if (gname == "identity")         return 0;
        else if (gname == "i")           return 0;
        else if (gname == "pauli_x")     return 3;
        else if (gname == "x")           return 3;
        else if (gname == "rx180")       return 3;
        else if (gname == "pauli_y")     return 6;
        else if (gname == "y")           return 6;
        else if (gname == "ry180")       return 6;
        else if (gname == "pauli_z")     return 9;
        else if (gname == "z")           return 9;
        else if (gname == "rz180")       return 9;
        else if (gname == "hadamard")    return 12;
        else if (gname == "h")           return 12;
        else if (gname == "xm90")        return 13;
        else if (gname == "mrx90")       return 13;
        else if (gname == "s")           return 14;
        else if (gname == "zm90")        return 14;
        else if (gname == "mrz90")       return 14;
        else if (gname == "ym90")        return 15;
        else if (gname == "mry90")       return 15;
        else if (gname == "x90")         return 16;
        else if (gname == "rx90")        return 16;
        else if (gname == "y90")         return 21;
        else if (gname == "ry90")        return 21;
        else if (gname == "sdag")        return 23;
        else if (gname == "z90")         return 23;
        else if (gname == "rz90")        return 23;
        else return -1;
    }

    // find the duration of the gate sequence corresponding to given clifford state
    // should be implemented using configuration file, searching for created gates and retrieving durations
    static UInt cs2cycles(Int cs) {
        switch(cs) {
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
            default: return 100;
        }
    }

    // return the gate sequence as string for debug output corresponding to given clifford state
    static Str cs2string(Int cs) {
        switch(cs) {
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
            default: return "[invalid clifford sequence]";
        }
    }

}; // class Clifford

/**
 * Clifford sequence optimizer.
 */
void clifford_optimize(
    quantum_program *programp,
    const quantum_platform &platform,
    const Str &passname
) {
    if (options::get(passname) == "no") {
        QL_DOUT("Clifford optimization on program " << programp->name << " at "
                                                    << passname << " not DONE");
        return;
    }
    QL_DOUT("Clifford optimization on program " << programp->name << " at "
                                                << passname << " ...");

    report_statistics(programp, platform, "in", passname, "# ");
    report_qasm(programp, platform, "in", passname);

    Clifford cliff;
    for (auto &kernel : programp->kernels) {
        cliff.clifford_optimize_kernel(kernel, platform, passname);
    }

    report_statistics(programp, platform, "out", passname, "# ");
    report_qasm(programp, platform, "out", passname);
}

}
