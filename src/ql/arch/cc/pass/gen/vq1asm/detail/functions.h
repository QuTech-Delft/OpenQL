#pragma once

#include "ql/ir/ir.h"

#include "types.h"
#include "operands.h"
#include "datapath.h"
#include "codesection.h"

namespace ql {
namespace arch {
namespace cc {
namespace pass {
namespace gen {
namespace vq1asm {
namespace detail {

class Codegen;  // to prevent recursive include loop


class Functions {
public:
    explicit Functions(const OperandContext &operandContext, const Datapath &dp, CodeSection &cs);
    ~Functions() = default;

    /*
     * FIXME: add comment
     */
    void dispatch(const ir::ExpressionRef &lhs, const ir::FunctionCall *fn, const Str &describe);
    void dispatch(const ir::FunctionCall *fn, const Str &label_if_false, const Str &describe);

    /*
     * Cast bregs to bits in REG_TMP0, i.e. transfer them from DSM to processor.
     * Returns a mask of bits set in REG_TMP0. FIXME: provides no knowledge about relation with DSM bits
     *
     * FIXME: We don't have a matching quantum instruction for this cast (formerly, we had 'if_1_break' etc), but do
     *  take up 'quantum' time, so the timeline is silently shifted
     */
    UInt emit_bin_cast(utils::Vec<utils::UInt> bregs, UInt expOpCnt);


private:  // types
    // arguments for a tOpFunc
    struct  FncArgs {
        Operands ops;
        UInt dest_reg = 0;
        Str label_if_false;
        Str operation;  // from function table
        Str describe; // to generate comments

        FncArgs(const OperandContext &operandContext, ir::Any<ir::Expression> operands, const Str &describe)
            : describe(describe) {
            // split operands into different types, and determine profile
            for(auto &op : operands) {
                ops.append(operandContext, op);
            }
        }
    };

    // function pointer for dispatch()
    typedef void (Functions::*tOpFunc)(const FncArgs &a);

    struct FuncInfo {
        tOpFunc func;
        Str operation;
    };

    using FuncMap = std::map<Str, FuncInfo>;

private:    // vars
    // references to object instances needed
    const OperandContext &operandContext;                       // context for Operand processing
    const Datapath &dp;                                         // handling of CC datapath
    CodeSection &cs;                                            // handling of code section

    FuncMap func_map_int;                                       // map name to function info, see register_functions()
    FuncMap func_map_bit;                                       // idem, for functions returning a bit

private:    // operator functions
    /*
     * operator functions for code generation in expressions
     *
     * Naming conventions:
     * - functions handling a single operator have naming inspired by libqasm's func_gen::Function::unique_name
     * - functions handling a group of operators are named "op_grp_<groupName>"
     * - the suffix defines the argument profile(s), using the naming from Operands::profile()
     */

    /*
     *  Functions returning a bit
     */

    // logical inversion
    void op_linv_B(const FncArgs &a);

    // bit arithmetic, 2 operands: "&&", "||", "^^"
    void op_grp_bit_2op_BB(const FncArgs &a);

    // relop, group 1: "==", "!="
    void op_grp_rel1_tail(const FncArgs &a); // common tail for functions below
    void op_grp_rel1_CC(const FncArgs &a);
    void op_grp_rel1_Ci_iC(const FncArgs &a);

    // relop, group 2: ">=", "<"
    void op_grp_rel2_CC(const FncArgs &a);
    void op_grp_rel2_Ci_iC(const FncArgs &a);

    // relop, group 3: ">", "<="
    void op_gt_CC(const FncArgs &a);
    void op_gt_Ci(const FncArgs &a);
    void op_gt_iC(const FncArgs &a);

    /*
     *  Functions returning an int
     */

    // bitwise inversion
    void op_binv_C(const FncArgs &a);

    // int arithmetic, 2 operands: "+", "-", "&", "|", "^"
    void op_grp_int_2op_CC(const FncArgs &a);
    void op_grp_int_2op_Ci_iC(const FncArgs &a);
    void op_sub_iC(const FncArgs &a);    // special case

    /*
     * other functions returning an int, for code generation in expressions
     */
#if OPT_CC_USER_FUNCTIONS
    void rnd_seed_C(const FncArgs &a);
    void rnd_seed_i(const FncArgs &a);
    void rnd_C(const FncArgs &a);
    void rnd_i(const FncArgs &a);
#endif


private:    // functions
    /*
     * Register the functions supported by the CC backend
     */
    void register_functions();

    void do_dispatch(const FuncMap &func_map, const Str &name, FncArgs &args, const Str & return_type);

    // FIXME: rename to emit_operation_..
    void emit_mnem2args(const FncArgs &a, const Str &arg0, const Str &arg1, const Str &target=REG_TMP0);
};

} // namespace detail
} // namespace vq1asm
} // namespace gen
} // namespace pass
} // namespace cc
} // namespace arch
} // namespace ql
