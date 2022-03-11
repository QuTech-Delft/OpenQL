#pragma once

#include "ql/ir/ir.h"

#include "types.h"
#include "operands.h"
#include "datapath.h"
//#include "codegen.h"

// Constants
// FIXME: move out of .h
#define REG_TMP0 "R63"                          // Q1 register for temporary use
#define REG_TMP1 "R62"                          // Q1 register for temporary use
#define NUM_RSRVD_CREGS 2                       // must match number of REG_TMP*
#define NUM_CREGS (64-NUM_RSRVD_CREGS)          // starting from R0
#define NUM_BREGS 1024                          // bregs require mapping to DSM, which introduces holes, so we probably fail before we reach this limit

namespace ql {
namespace arch {
namespace cc {
namespace pass {
namespace gen {
namespace vq1asm {
namespace detail {

class Codegen;  // FIXME, to prevent include loop

class Functions {
public:
    explicit Functions(OperandContext &operandContext, Datapath &dp, Codegen &cg);
    ~Functions() = default;

//    void register_();
    /*
     * Notes:
     * - expression->as_function_call() must be non-zero
     */
    void dispatch(const Str &name, const ir::ExpressionRef &lhs, const ir::ExpressionRef &expression);


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

    typedef void (Functions::*tOpFunc)(const OpArgs &a);

    struct FuncInfo {
        tOpFunc func;
        Str operation;
    };


private:    // vars
    // Object instances needed
    OperandContext &operandContext;                             // context for Operand processing
    Datapath &dp;                                               // handling of CC datapath
    Codegen &cg;                                                // code generation, used for helpers

    // map name to function, see register_functions()
    std::map<Str, FuncInfo> func_map;

private:    // methods
    /*
     * operator functions for code generation in expressions
     *
     * Naming conventions:
     * - functions handling a single operator have naming inspired by libqasm's func_gen::Function::unique_name
     * - functions handling a group of operators are named "op_grp_<groupName>"
     * - the suffix defines the argument profile(s), using the naming from get_operand_type()
     */

    // bitwise inversion
    void op_binv_C(const OpArgs &a);

    // logical inversion
    void op_linv_B(const OpArgs &a);

    // int arithmetic, 2 operands: "+", "-", "&", "|", "^"
    void op_grp_int_2op_CC(const OpArgs &a);
    void op_grp_int_2op_Ci_iC(const OpArgs &a);
    void op_sub_iC(const OpArgs &a);    // special case

    // bit arithmetic, 2 operands: "&&", "||", "^^"
    void op_grp_bit_2op_BB(const OpArgs &a);

    // relop, group 1: "==", "!="
    void op_grp_rel1_tail(const OpArgs &a); // common tail for functions below
    void op_grp_rel1_CC(const OpArgs &a);
    void op_grp_rel1_Ci_iC(const OpArgs &a);

    // relop, group 2: ">=", "<"
    void op_grp_rel2_CC(const OpArgs &a);
    void op_grp_rel2_Ci_iC(const OpArgs &a);

    // relop, group 3: ">", "<="
    void op_gt_CC(const OpArgs &a);
    void op_gt_Ci(const OpArgs &a);
    void op_gt_iC(const OpArgs &a);

    /*
     * other functions for code generation in expressions
     */
#if OPT_CC_USER_FUNCTIONS
    void rnd_seed_C(const OpArgs &a);
    void rnd_seed_i(const OpArgs &a);
    void rnd(const OpArgs &a);
#endif

    /*
     * Register the functions supported by the CC backend
     */
    void register_functions();

    /*
     * Get type of single function operand. Encoding:
     * - 'b': bit literal
     * - 'i': int literal
     * - 'B': breg reference
     * - 'C': creg reference
     *
     * Inspired by func_gen::Function::generate_impl_footer and cqasm::types::from_spec, but notice that we add 'C' and
     * have sightly different purpose and interpretation
     */
    Str get_operand_type(const ir::ExpressionRef &op);

    /*
     * Cast bregs to bits in REG_TMP0, i.e. transfer them from DSM to processor.
     * Returns a mask of bits set in REG_TMP0. FIXME: provides no knowledge about relation with DSM bits
     *
     * FIXME: We don't have a matching quantum instruction for this cast (formerly, we had 'if_1_break' etc), but do
     *  take up 'quantum' time, so the timeline is silently shifted
     */
    UInt emit_bin_cast(utils::Vec<utils::UInt> bregs, Int expOpCnt);

    // FIXME: rename to emit_operation_..
//    void emit_mnem2args(const OpArgs &a, Int arg0, Int arg1, const Str &target=REG_TMP0);
    void emit_mnem2args(const OpArgs &a, const Str &arg0, const Str &arg1, const Str &target=REG_TMP0);

    Int creg2reg(const ir::Reference &ref);
//    Int dest_reg(const ir::ExpressionRef &lhs);

    // Convert integer/creg function_call/operands expression to Q1 instruction argument.
//    Str expr2q1Arg(const ir::ExpressionRef &op);


    // helpers to ease nice assembly formatting
//    void emit(const Str &labelOrComment, const Str &instr="");
//    void emit(const Str &label, const Str &instr, const Str &ops, const Str &comment="");
//    void emit(Int slot, const Str &instr, const Str &ops, const Str &comment="");
};


} // namespace detail
} // namespace vq1asm
} // namespace gen
} // namespace pass
} // namespace cc
} // namespace arch
} // namespace ql
