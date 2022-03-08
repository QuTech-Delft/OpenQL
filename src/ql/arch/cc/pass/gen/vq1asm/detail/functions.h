#pragma once

#include "ql/ir/ir.h"

#include "types.h"
#include "operands.h"

// Constants
// FIXME: move out of .h
#define REG_TMP0 "R63"                          // Q1 register for temporary use
#define REG_TMP1 "R62"                          // Q1 register for temporary use
#define NUM_RSRVD_CREGS 2                       // must match number of REG_TMP*
#define NUM_CREGS (64-NUM_RSRVD_CREGS)
#define NUM_BREGS 1024                          // bregs require mapping to DSM, which introduces holes, so we probably fail before we reach this limit

namespace ql {
namespace arch {
namespace cc {
namespace pass {
namespace gen {
namespace vq1asm {
namespace detail {


class Functions {
public:
    Functions();
    ~Functions();

    void register_();
    void dispatch(const Str &name, const ir::ExpressionRef &expression);


private:  // types
    class  OpArgs {
    public:
        OpArgs(const ir::ExpressionRef &e) : expression(e) {};

//    private:
//        ir::Any<ir::Expression> operands;   // FIXME: use ops
        Operands ops;
        UInt dest_reg;
        Str label_if_false;
        Str operation;  // from function table
        const ir::ExpressionRef &expression; // full expression to generate comments

    };


private:    // vars
    // FIXME
    OperandContext operandContext;                              // context for Operand processing


private:    // methods
    /*
     * operator functions
     *
     * Function naming follows libqasm's func_gen::Function::unique_name
     *     *
     */

    // bitwise inversion
    void op_binv_x(const OpArgs &a);

    // logical inversion
    void op_linv_b(const OpArgs &a);

    // int arithmetic, 2 operands: "+", "-", "&", "|", "^"
    void op_grp_int_2op_rx(const OpArgs &a);
    void op_grp_int_2op_iC(const OpArgs &a);
    void op_sub_lr(const OpArgs &a);    // special case

    // bit arithmetic, 2 operands: "&&", "||", "^^"
    void op_grp_bit_2op(const OpArgs &a);

    // relop, group 1: "==", "!="
    void op_grp_rel1_rx(const OpArgs &a);
    void op_grp_rel1_lr(const OpArgs &a);

    // relop, group 2: ">=", "<"
    void op_grp_rel2_rx(const OpArgs &a);
    void op_grp_rel2_lr(const OpArgs &a);

    // relop, group 3: ">", "<="
    void op_gt_rl(const OpArgs &a);
    void op_gt_rr(const OpArgs &a);
    void op_gt_lr(const OpArgs &a);

    /*
     * Get profile of single function parameter. Encoding:
     * - 'b': bit literal
     * - 'i': int literal
     * - 'B': breg reference
     * - 'C': creg reference
     * - 'x': literal or reference FIXME
     *
     * Inspired by func_gen::Function::generate_impl_footer, but notice that we add 'C' and have sightly different
     * purpose and interpretation
     */
    Str get_profile(const ir::ExpressionRef &op);

    UInt emit_bin_cast(ir::Any<ir::Expression> operands, Int expOpCnt);

    // FIXME: rename to emit_operation_..
    void emit_mnem2args(const OpArgs &a, Int arg0, Int arg1, const Str &target=REG_TMP0);
    void emit_mnem2args(const OpArgs &a, const Str &arg0, const Str &arg1, const Str &target=REG_TMP0);

    Int creg2reg(const ir::Reference &ref);
    Int dest_reg(const ir::ExpressionRef &lhs);

    // Convert integer/creg function_call/operands expression to Q1 instruction argument.
    Str expr2q1Arg(const ir::ExpressionRef &op);


    // helpers to ease nice assembly formatting
    void emit(const Str &labelOrComment, const Str &instr="");
    void emit(const Str &label, const Str &instr, const Str &ops, const Str &comment="");
    void emit(Int slot, const Str &instr, const Str &ops, const Str &comment="");





private:
    // map name to function
};


} // namespace detail
} // namespace vq1asm
} // namespace gen
} // namespace pass
} // namespace cc
} // namespace arch
} // namespace ql
