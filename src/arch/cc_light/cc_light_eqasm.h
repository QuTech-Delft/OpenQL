/** \name
 * CC-light eQASM code emitter.
 */

#pragma once

#include "utils/num.h"
#include "utils/str.h"
#include "utils/pair.h"
#include "utils/vec.h"
#include <sstream>
#include <bitset>
#include "classical.h"

namespace ql {
namespace arch {

typedef utils::Bool bit_t;
typedef utils::Str cc_light_eqasm_instr_t;
typedef utils::Vec<bit_t> bitset_t;

class cc_light_eqasm_instruction;

typedef utils::Vec<cc_light_eqasm_instruction*> cc_light_eqasm_program_t;

// instruction type
typedef enum {
    // classical instructions

    __cc_light_eqasm_add__      ,
    __cc_light_eqasm_sub__      ,
    __cc_light_eqasm_and__      ,
    __cc_light_eqasm_or__       ,
    __cc_light_eqasm_xor_       ,
    __cc_light_eqasm_not__      ,

    __cc_light_eqasm_cmp__      ,
    __cc_light_eqasm_br__       ,
    __cc_light_eqasm_fbr__      ,
    __cc_light_eqasm_fmr__      ,

    __cc_light_eqasm_ldi__      ,
    __cc_light_eqasm_ldui__     ,

    __cc_light_eqasm_smis__     ,
    __cc_light_eqasm_smit__     ,

    // quantum instructions

    __cc_light_eqasm_i__        ,
    __cc_light_eqasm_x__        ,
    __cc_light_eqasm_y__        ,
    __cc_light_eqasm_z__        ,
    __cc_light_eqasm_h__        ,

    __cc_light_eqasm_x90__      ,
    __cc_light_eqasm_mx90__     ,
    __cc_light_eqasm_y90__      ,
    __cc_light_eqasm_my90__     ,

    __cc_light_eqasm_s__        ,
    __cc_light_eqasm_sdag__     ,
    __cc_light_eqasm_t__        ,
    __cc_light_eqasm_tdag__     ,

    __cc_light_eqasm_prepz__    ,
    __cc_light_eqasm_prepx__    ,

    __cc_light_eqasm_measurex__ ,
    __cc_light_eqasm_measurez__ ,

    __cc_light_eqasm_cnot__     ,
    __cc_light_eqasm_cz__       ,
    __cc_light_eqasm_swap__     ,

    __cc_light_eqasm_qwait__    ,
    __cc_light_eqasm_qwaitr__   ,
    __cc_light_eqasm_qnop__

} cc_light_eqasm_instr_type_t;

// operation type
// typedef enum
// {
//    __none__       ,
//    __rf__         ,
//    __flux__       ,
//    __measurement__,
//    __wait__       ,
//    __unknown_operation__
// } operation_type_t;

// #define __operation_types_num__ (4)

// hardware resources
typedef utils::Vec<utils::UInt> qubit_set_t;
typedef utils::Pair<utils::UInt,utils::UInt> qubit_pair_t;
typedef utils::Vec<qubit_pair_t> qubit_pair_set_t;
typedef utils::Str mask_t;

/**
 * qubit mask
 */
class single_qubit_mask {
public:
    qubit_set_t qs;
    single_qubit_mask(qubit_set_t &&qs);
    single_qubit_mask(utils::UInt qubit);
    mask_t get_mask(utils::UInt reg);
};

/**
 * two qubits mask
 */
class two_qubit_mask {
public:
    qubit_pair_set_t qs;
    two_qubit_mask(qubit_pair_set_t &&qs);
    two_qubit_mask(qubit_pair_t p);
    mask_t get_mask(utils::UInt reg);
};

/**
 * cc_light_eqasm instruction interface
 */
class cc_light_eqasm_instruction {
public:

    qubit_set_t                 used_qubits;

    utils::UInt                 duration;
    utils::UInt                 latency = 0;
    utils::UInt                 start = 0;
    utils::UInt                 codeword = 0;
    utils::UInt                 opcode = 0;
    utils::UInt                 condition = 0;

    operation_type_t            operation_type;
    cc_light_eqasm_instr_type_t instr_type;

    utils::Str                  qasm_label;

    utils::Bool                 latency_compensated = false;

    utils::Str                  name;

public:

    /**
     * emit cc_light_eqasm code
     */
    virtual cc_light_eqasm_instr_t code() = 0;

    // /**
    //  * return instruction trace
    //  */
    // virtual instruction_traces_t trace() = 0;

    /**
     *  compensate for latency
     */
    virtual void compensate_latency();

    /**
     * set start
     */
    virtual void set_start(utils::UInt t);

    /**
     * decompose meta-instructions
     */
    virtual cc_light_eqasm_program_t decompose();

    /**
     * return cc_light_eqasm instruction type
     */
    virtual cc_light_eqasm_instr_type_t get_instruction_type() const;

    /**
     * return operation type
     */
    virtual operation_type_t get_operation_type() const;

};


/**
 * cc_light_single_qubit_gate
 */
class cc_light_single_qubit_gate : public cc_light_eqasm_instruction {
public:
    single_qubit_mask mask;
    cc_light_single_qubit_gate(const utils::Str &name, single_qubit_mask &&mask);

    /**
     * emit cc_light_eqasm code
     */
    cc_light_eqasm_instr_t code() override;
};

/**
 * cc_light_two_qubit_gate
 */
class cc_light_two_qubit_gate : public cc_light_eqasm_instruction {
public:
    two_qubit_mask mask;
    cc_light_two_qubit_gate(const utils::Str &name, two_qubit_mask &&mask);

    /**
     * emit cc_light_eqasm code
     */
    cc_light_eqasm_instr_t code();
};

} // namespace arch
} // namespace ql
