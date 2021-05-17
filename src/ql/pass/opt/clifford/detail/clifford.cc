/** \file
 * Clifford sequence optimizer.
 */

#include "clifford.h"

#include "ql/utils/num.h"
#include "ql/com/options.h"

namespace ql {
namespace pass {
namespace opt {
namespace clifford {
namespace optimize {
namespace detail {

using namespace utils;

/**
 * Create gate sequences for all accumulated cliffords, output them and
 * reset state.
 */
void Clifford::sync_all(const ir::KernelRef &k) {
    QL_DOUT("... sync_all");
    for (UInt q = 0; q < nq; q++) {
        sync(k, q);
    }
    QL_DOUT("... sync_all DONE");
}

/**
 * Create gate sequence for accumulated cliffords of qubit q, output it and
 * reset state.
 */
void Clifford::sync(const ir::KernelRef &k, UInt q) {
    Int csq = cliffstate[q];
    if (csq != 0) {
        QL_DOUT("... sync q[" << q << "]: generating clifford " << cs2string(csq));
        k->clifford(csq, q);          // generates clifford(csq) in kernel.c
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

/**
 * Find the clifford state from identity to given gate, or return -1 if
 * unknown or the gate is not in C1.
 *
 * TODO: this currently infers the Clifford index by gate name; instead
 *  semantics like this should be in the config file somehow.
 */
Int Clifford::gate2cs(const ir::GateRef &gate) {
    auto gname = gate->name;
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

/**
 * Find the duration of the gate sequence corresponding to given clifford
 * state.
 *
 * TODO: should be implemented using configuration file, searching for
 *  created gates and retrieving durations
 */
UInt Clifford::cs2cycles(Int cs) {
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
Str Clifford::cs2string(Int cs) {
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

/**
 * Optimizes the given kernel, returning how many cycles were saved.
 */
utils::UInt Clifford::optimize_kernel(const ir::KernelRef &kernel) {
    QL_DOUT("clifford_optimize_kernel()");

    nq = kernel->qubit_count;
    ct = kernel->platform->cycle_time;
    QL_DOUT("Clifford optimizer on kernel " << kernel->name << " ...");

    // copy circuit kernel.c to take input from;
    // output will fill kernel.c again
    auto input_gates = kernel->gates;
    kernel->gates.reset();

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
    for (const auto &gate : input_gates) {
        QL_DOUT("... gate: " << gate->qasm());

        if (
            gate->type() == ir::GateType::CLASSICAL               // classical gates (really being pessimistic here about these)
            || gate->operands.empty()                             // gates without operands which may affect ALL qubits
        ) {
            // sync all qubits: create gate sequences corresponding to what was accumulated in cliffstate, for all qubits
            sync_all(kernel);
            kernel->gates.add(gate);
        } else if (gate->operands.size() != 1) {                 // gates like CNOT/CZ/TOFFOLI
            // sync particular qubits: create gate sequences corresponding to what was accumulated in cliffstate, for those particular operand qubits
            for (auto q : gate->operands) {
                sync(kernel, q);
            }
            kernel->gates.add(gate);
        } else {
            // unary quantum gates like x/y/z/h/xm90/y90/s/wait/meas/prepz
            UInt q = gate->operands[0];
            Int cs = gate2cs(gate);
            if (
                cs == -1                                        // non-clifford unary gates (wait, meas, prepz, ...)
                || gate->is_conditional()                         // conditional unary (clifford) gates
            ) {
                // sync particular single qubit: create gate sequence corresponding to what was accumulated in cliffstate, for this particular operand qubit
                QL_DOUT("... unary gate not a clifford gate or conditional: " << gate->qasm());
                sync(kernel, q);
                kernel->gates.add(gate);
            } else {
                // unary quantum clifford gates like x/y/z/h/xm90/y90/s/...
                // don't emit gate but accumulate gate in cliffstate
                // also record accumulated cycles to compute savings
                cliffcycles[q] += (gate->duration + ct - 1) / ct;
                Int csq = cliffstate[q];
                QL_DOUT("... from " << cs2string(csq) << " to " << cs2string(TRANSITION_TABLE[csq][cs]));
                cliffstate[q] = TRANSITION_TABLE[csq][cs];
            }
        }
        QL_DOUT("... gate: " << gate->qasm() << " DONE");
    }
    sync_all(kernel);
    kernel->cycles_valid = false;

    QL_DOUT("Clifford optimizer on kernel " << kernel->name << " saved " << total_saved << " cycles [DONE]");

    return total_saved;
}

} // namespace detail
} // namespace optimize
} // namespace clifford
} // namespace opt
} // namespace pass
} // namespace ql
