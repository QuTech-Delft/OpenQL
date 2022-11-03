/** \file
 * Clifford sequence optimizer.
 */

#include "clifford.h"
#include "ql/ir/cqasm/write.h"
#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/com/options.h"
#include "ql/ir/ops.h"

namespace ql {
namespace pass {
namespace opt {
namespace new_clifford {
namespace optimize {
namespace detail {

using namespace utils;

void Clifford::add_instruction(const ir::Ref &ir, const ir::BlockRef &block, const utils::Str& gate_name, UInt qubit) {
    utils::Any<ir::Expression> qubit_operand;
    qubit_operand.add(ir::make_qubit_ref(ir, qubit));
    ir::ExpressionRef condition;  // By leaving the condition emtpy it will be inferred by make_instruction()
    auto insn = make_instruction(ir, gate_name, qubit_operand, condition, true, true);
    if (insn->as_custom_instruction()) {
        block->statements.add(insn);
    } else {
        QL_USER_ERROR("Not a custom instruction!");
    }
}

void Clifford::rx90(const ir::Ref &ir, const ir::BlockRef &b, UInt qubit) {
    add_instruction(ir, b, "rx90", qubit);
}

void Clifford::ry90(const ir::Ref &ir, const ir::BlockRef &b, UInt qubit) {
    add_instruction(ir, b, "ry90", qubit);
}

void Clifford::mrx90(const ir::Ref &ir, const ir::BlockRef &b, UInt qubit) {
    add_instruction(ir, b, "mrx90", qubit);
}

void Clifford::mry90(const ir::Ref &ir, const ir::BlockRef &b, UInt qubit) {
    add_instruction(ir, b, "mry90", qubit);
}

void Clifford::rx180(const ir::Ref &ir, const ir::BlockRef &b, UInt qubit) {
    add_instruction(ir, b, "rx180", qubit);
}

void Clifford::ry180(const ir::Ref &ir, const ir::BlockRef &b, UInt qubit) {
    add_instruction(ir, b, "ry180", qubit);
}

void Clifford::add_clifford_instruction(const ir::Ref &ir, const ir::BlockRef &b, Int id, UInt qubit) {
    switch (id) {
        case 0 :
            break;              //  ['I']
        case 1 :
            ry90(ir, b, qubit);
            rx90(ir, b, qubit);
            break;              //  ['Y90', 'X90']
        case 2 :
            mrx90(ir, b, qubit);
            mry90(ir, b, qubit);
            break;              //  ['mX90', 'mY90']
        case 3 :
            rx180(ir, b, qubit);
            break;              //  ['X180']
        case 4 :
            mry90(ir, b, qubit);
            mrx90(ir, b, qubit);
            break;              //  ['mY90', 'mX90']
        case 5 :
            rx90(ir, b, qubit);
            mry90(ir, b, qubit);
            break;              //  ['X90', 'mY90']
        case 6 :
            ry180(ir, b, qubit);
            break;              //  ['Y180']
        case 7 :
            mry90(ir, b, qubit);
            rx90(ir, b, qubit);
            break;              //  ['mY90', 'X90']
        case 8 :
            rx90(ir, b, qubit);
            ry90(ir, b, qubit);
            break;              //  ['X90', 'Y90']
        case 9 :
            rx180(ir, b, qubit);
            ry180(ir, b, qubit);
            break;              //  ['X180', 'Y180']
        case 10:
            ry90(ir, b, qubit);
            mrx90(ir, b, qubit);
            break;              //  ['Y90', 'mX90']
        case 11:
            mrx90(ir, b, qubit);
            ry90(ir, b, qubit);
            break;              //  ['mX90', 'Y90']
        case 12:
            ry90(ir, b, qubit);
            rx180(ir, b, qubit);
            break;              //  ['Y90', 'X180']
        case 13:
            mrx90(ir, b, qubit);
            break;              //  ['mX90']
        case 14:
            rx90(ir, b, qubit);
            mry90(ir, b, qubit);
            mrx90(ir, b, qubit);
            break;              //  ['X90', 'mY90', 'mX90']
        case 15:
            mry90(ir, b, qubit);
            break;              //  ['mY90']
        case 16:
            rx90(ir, b, qubit);
            break;              //  ['X90']
        case 17:
            rx90(ir, b, qubit);
            ry90(ir, b, qubit);
            rx90(ir, b, qubit);
            break;              //  ['X90', 'Y90', 'X90']
        case 18:
            mry90(ir, b, qubit);
            rx180(ir, b, qubit);
            break;              //  ['mY90', 'X180']
        case 19:
            rx90(ir, b, qubit);
            ry180(ir, b, qubit);
            break;              //  ['X90', 'Y180']
        case 20:
            rx90(ir, b, qubit);
            mry90(ir, b, qubit);
            rx90(ir, b, qubit);
            break;              //  ['X90', 'mY90', 'X90']
        case 21:
            ry90(ir, b, qubit);
            break;              //  ['Y90']
        case 22:
            mrx90(ir, b, qubit);
            ry180(ir, b, qubit);
            break;              //  ['mX90', 'Y180']
        case 23:
            rx90(ir, b, qubit);
            ry90(ir, b, qubit);
            mrx90(ir, b, qubit);
            break;              //  ['X90', 'Y90', 'mX90']
        default:
            break;
    }
}

/**
 * Create gate sequences for all accumulated cliffords, output them and
 * reset state.
 */
void Clifford::sync_all(const ir::Ref &ir, const ir::BlockRef &b) {
    QL_DOUT("... sync_all");
    for (UInt q = 0; q < nq; q++) {
        sync(ir, b, q);
    }
    QL_DOUT("... sync_all DONE");
}

/**
* Create gate sequence for accumulated cliffords of qubit q, output it and
* reset state.
*/
void Clifford::sync(const ir::Ref &ir, const ir::BlockRef &b, utils::UInt q) {
    Int csq = cliffstate[q];
    if (csq != 0) {
        QL_DOUT("... sync q[" << q << "]: generating clifford " << cs2string(csq));
        add_clifford_instruction(ir, b, csq, q);          // generates clifford(csq) in kernel.c
        UInt acc_cycles = cliffcycles[q];
        UInt ins_cycles = cs2cycles(csq);
        QL_DOUT("... qubit q[" << q << "]: accumulated: " << acc_cycles << ", inserted: "
                               << ins_cycles);
        if (acc_cycles > ins_cycles) QL_DOUT(
                    "... qubit q[" << q << "]: saved " << (acc_cycles - ins_cycles)
                                   << " cycles");
        if (acc_cycles < ins_cycles) QL_DOUT(
                    "... qubit q[" << q << "]: additional " << (ins_cycles - acc_cycles)
                                   << " cycles");
        total_saved += acc_cycles - ins_cycles;
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
utils::Int Clifford::statement2cs(const ir::InstructionType &instructionType) {
    auto gname = instructionType.name;
    if (gname == "identity") return 0;
    else if (gname == "i") return 0;
    else if (gname == "pauli_x") return 3;
    else if (gname == "x") return 3;
    else if (gname == "rx180") return 3;
    else if (gname == "pauli_y") return 6;
    else if (gname == "y") return 6;
    else if (gname == "ry180") return 6;
    else if (gname == "pauli_z") return 9;
    else if (gname == "z") return 9;
    else if (gname == "rz180") return 9;
    else if (gname == "hadamard") return 12;
    else if (gname == "h") return 12;
    else if (gname == "xm90") return 13;
    else if (gname == "mrx90") return 13;
    else if (gname == "s") return 14;
    else if (gname == "zm90") return 14;
    else if (gname == "mrz90") return 14;
    else if (gname == "ym90") return 15;
    else if (gname == "mry90") return 15;
    else if (gname == "x90") return 16;
    else if (gname == "rx90") return 16;
    else if (gname == "y90") return 21;
    else if (gname == "ry90") return 21;
    else if (gname == "sdag") return 23;
    else if (gname == "z90") return 23;
    else if (gname == "rz90") return 23;
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
    switch (cs) {
        case 0 :
            return 0;
        case 1 :
            return 2;
        case 2 :
            return 2;
        case 3 :
            return 1;
        case 4 :
            return 2;
        case 5 :
            return 2;
        case 6 :
            return 1;
        case 7 :
            return 2;
        case 8 :
            return 2;
        case 9 :
            return 2;
        case 10:
            return 2;
        case 11:
            return 2;
        case 12:
            return 2;
        case 13:
            return 1;
        case 14:
            return 3;
        case 15:
            return 1;
        case 16:
            return 1;
        case 17:
            return 3;
        case 18:
            return 2;
        case 19:
            return 2;
        case 20:
            return 3;
        case 21:
            return 1;
        case 22:
            return 2;
        case 23:
            return 3;
        default:
            return 100;
    }
}

// return the gate sequence as string for debug output corresponding to given clifford state
Str Clifford::cs2string(Int cs) {
    switch (cs) {
        case 0 :
            return ("[id;]");
        case 1 :
            return ("[y90; x90;]");
        case 2 :
            return ("[xm90; ym90;]");
        case 3 :
            return ("[x180;]");
        case 4 :
            return ("[ym90; xm90;]");
        case 5 :
            return ("[x90; ym90;]");
        case 6 :
            return ("[y180;]");
        case 7 :
            return ("[ym90; x90;]");
        case 8 :
            return ("[x90; y90;]");
        case 9 :
            return ("[x180; y180;]");
        case 10:
            return ("[y90; xm90;]");
        case 11:
            return ("[xm90; y90;]");
        case 12:
            return ("[y90; x180;]");
        case 13:
            return ("[xm90;]");
        case 14:
            return ("[x90; ym90; xm90;]");
        case 15:
            return ("[ym90;]");
        case 16:
            return ("[x90;]");
        case 17:
            return ("[x90; y90; x90;]");
        case 18:
            return ("[ym90; x180;]");
        case 19:
            return ("[x90; y180;]");
        case 20:
            return ("[x90; ym90; x90;]");
        case 21:
            return ("[y90;]");
        case 22:
            return ("[xm90; y180;]");
        case 23:
            return ("[x90; y90; xm90;]");
        default:
            return "[invalid clifford sequence]";
    }
}

/**
 * Find the qubit number of the operand
 * FixMe deal with virtual qubits as well
*/
utils::Int Clifford::qubit_nr(utils::One<ir::Expression> &operand) {
    QL_DOUT("qubit_nr: qubit:");
    if (auto *reference = operand->as_reference()) {
        auto indices = reference->indices;
        if (indices.size() == 1) {
            auto value_expr = indices[0];
            if (auto int_lit = value_expr->as_int_literal()) {
                return int_lit->value;
            } else {
                QL_USER_ERROR("qubit index expression is not an integer literal");
            }
        } else {
            QL_USER_ERROR("qubit has more than one index");
        }
    } else {
        QL_USER_ERROR("qubit operand expression is not a reference");
    }
    return -1;
}

/**
* Optimizes all blocks in a program, returning how many cycles were saved.
*/
utils::UInt Clifford::optimize_blocks(const ir::Ref &ir) {

    QL_DOUT("new_clifford_optimize_blocks(). Program name: " << ir->program->name);

    utils::UInt cycles_saved = 0;


    for (auto &block: ir->program->blocks) {
        cycles_saved += optimize_one_block(block, ir);
    }
    return cycles_saved;
}

/**
* Optimise one block, returning how many cycles were saved
*/
utils::UInt Clifford::optimize_one_block(ir::BlockRef &block, const ir::Ref &ir) {
    QL_DOUT("new_clifford_optimize_one_block(). Blockname: " << block->name);

    auto &qubits_reg = ir->platform->qubits;
    /*
    * * FixMe: we have to check that shape has one element. And it's an anomaly if it has different number!
    */
    nq = qubits_reg->shape[0]; // FixMe take virtual qubits into account as well.

    ct = 1; //FixMe: the old IR has cycle time explicit, the new IR does not.

    // copy circuit block->statements to take input from;
    // output will fill block->statements again
    auto input_statements = block->statements;
    block->statements.reset();

    cliffstate.resize(nq, 0);   // 0 is identity; for all qubits accumulated state is set to identity
    cliffcycles.resize(nq, 0);  // for all qubits, no accumulated cycles
    total_saved = 0;                       // reset saved, just for reporting

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
    for (const auto &stmt: input_statements) {

        /*
         * Cases
         *  - gate is classical: sync the whole block in one sweep
         *  - gate with no operands: sync the whole block in one sweep
         *  - gate has multiple operands (more than one): sync each operand individually
         *  - unary quantum gates:
         *    - if not a clifford gate: sync qubit
         *    - if it is a conditional gate: sync qbit
         *  - otherwise (a true clifford gate): add and update the clifford gate accumulation
         */
        auto cstm_inst = stmt->as_custom_instruction();
        if (!cstm_inst                      // classical gates (really being pessimistic here about these)
            ||
            cstm_inst->operands.empty()  // gates without operands which may affect ALL qubits
                ) {
            sync_all(ir, block);
            block->statements.add(stmt);
        } else if (cstm_inst->operands.size() != 1) {                // gates like CNOT/CZ/TOFFOLI
            // sync particular qubits: create gate sequences corresponding to what was accumulated in cliffstate, for those particular operand qubits
            for (auto &operand: cstm_inst->operands) {
                sync(ir, block, qubit_nr(operand));
            }
            block->statements.add(stmt);
        } else {
            utils::UInt q;
            // unary quantum gates like x/y/z/h/xm90/y90/s/wait/meas/prepz
            if (auto op = (cstm_inst->operands[0])->as_reference()) {
                if (auto ind = op->indices[0]->as_int_literal()) {
                    q = ind->value;
                } else {
                    QL_USER_ERROR("operand[0] is not an int_literal: dump");
                    ind->dump();
                }
            } else {
                QL_USER_ERROR("operand[0] is not a reference");
                cstm_inst->operands[0]->dump();
            }
            QL_DOUT("Qbit nr" << q);
            Int cs = statement2cs(*cstm_inst->instruction_type);
            if (cs ==
                -1                                            // non-clifford unary gates (wait, meas, prepz, ...)
                ||
                !cstm_inst->condition->as_bit_literal()->value  // conditionals with a non-literal 'true' as the condition
                    ) {
//                // sync particular single qubit: create gate sequence corresponding to what was accumulated in cliffstate, for this particular operand qubit
                QL_DOUT("... unary gate not a clifford gate or conditional: " << cstm_inst->instruction_type->name);
                sync(ir, block, qubit_nr(cstm_inst->operands[0]));
                block->statements.add(stmt);
            } else {
                // unary quantum clifford gates like x/y/z/h/xm90/y90/s/...
                // don't emit gate but accumulate gate in cliffstate
                // also record accumulated cycles to compute savings
                cliffcycles[q] += (cstm_inst->instruction_type->duration + ct - 1) / ct;
                Int csq = cliffstate[q];
                QL_DOUT("... from " << cs2string(csq) << " to "
                                    << cs2string(TRANSITION_TABLE[csq][cs]));
                cliffstate[q] = TRANSITION_TABLE[csq][cs];
            }
        }
        QL_DOUT("... gate: " << cstm_inst->instruction_type->name << " DONE");
        sync_all(ir, block);
    }
    QL_DOUT("Clifford optimizer on kernel " << block->name << " saved " << total_saved
                                            << " cycles [DONE]");

    return total_saved;
}


} // namespace detail
} // namespace optimize
} // namespace clifford
} // namespace opt
} // namespace pass
} // namespace ql
