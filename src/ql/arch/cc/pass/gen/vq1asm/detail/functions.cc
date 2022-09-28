#include "ql/arch/cc/pass/gen/vq1asm/detail/functions.h"
#include "ql/arch/cc/pass/gen/vq1asm/detail/codegen.h"

#include "ql/ir/describe.h"

namespace ql {
namespace arch {
namespace cc {
namespace pass {
namespace gen {
namespace vq1asm {
namespace detail {

using namespace utils;


Functions::Functions(const OperandContext &operandContext, const Datapath &dp, CodeSection &cs)
    : operandContext(operandContext)
    , dp(dp)
    , cs(cs) {
    register_functions();
}

void Functions::do_dispatch(const FuncMap &func_map, const Str &name, FncArgs &args, const Str & return_type) {
    // create key from name and operand profile, using the same encoding as register_functions()
    Str key = name + "_" + args.ops.profile;

    // lookup key
    auto it = func_map.find(key);
    if (it == func_map.end()) {
        // NB: if we arrive here, there's an inconsistency between the functions registered in 'ql::ir::cqasm:read()'
        // (see comment before Codegen::handle_set_instruction) and those available in func_map.
        QL_ICE(
            "function '" << name
            << "' with profile '" << args.ops.profile
            << " and return type '" << return_type
            << "' not supported by CC backend, but it should be"
        );
    }

    // finish arguments
    args.operation = it->second.operation;

    // call the function
    (this->*it->second.func)(args);
}

void Functions::dispatch(const ir::ExpressionRef &lhs, const ir::FunctionCall *fn, const Str &describe) {
    // collect arguments for operator functions
    FncArgs args(operandContext, fn->operands, describe);
    args.dest_reg = cs.dest_reg(lhs);

    // FIXME: check creg, breg range? Already done, everywhere?

    do_dispatch(func_map_int, fn->function_type->name, args, "int");
}

void Functions::dispatch(const ir::FunctionCall *fn, const Str &label_if_false, const Str &describe) {
    // collect arguments for operator functions
    FncArgs args(operandContext, fn->operands, describe);
    args.label_if_false = label_if_false;

    // FIXME: check creg, breg range? Already done, everywhere?

    do_dispatch(func_map_bit, fn->function_type->name, args, "bit");
}


UInt Functions::emit_bin_cast(utils::Vec<utils::UInt> bregs, Int expOpCnt) {
    if(bregs.size() != expOpCnt) {
        QL_ICE("Expected " << expOpCnt << " breg operands, got " << bregs.size());
    }

    // Compute DSM address and mask for operands.
    UInt smAddr = 0;
    UInt mask = 0;      // mask for used SM bits in 32 bit word transferred using move_sm
    Str descr;
    for (Int i=0; i<bregs.size(); i++) {
        auto breg = bregs[i];
        if(breg >= NUM_BREGS) {
            QL_INPUT_ERROR("bit register index " << breg << " exceeds maximum of " << NUM_BREGS-1);   // FIXME: cleanup "breg" vs. "bit register index"
        }

        // get SM bit for classic operand (allocated during readout)
        UInt smBit = dp.getSmBit(breg);
        descr += QL_SS2S("b[" << breg << "]=DSMbit[" << smBit << "]; ");

        // compute and check SM address
        UInt mySmAddr = smBit / 32;    // 'seq_cl_sm' is addressable in 32 bit words
        if(i==0) {
            smAddr = mySmAddr;
        } else {
            if(smAddr != mySmAddr) {
                QL_USER_ERROR("Cannot access DSM address " << smAddr << " and " << mySmAddr << " in single transfer");
                // NB: we could setup several transfers
            }
        }

        // update mask of used bits
        mask |= 1ul << (smBit % 32);
    }

    /*
     * Code inserted here:
     *      seq_cl_sm   S<address>          ; pass 32 bit SM-data to Q1 ...
     *      seq_wait    3                   ; prevent starvation of real time part during instructions below: 4 classic instructions + 1 branch
     *      move_sm     Ra                  ; ... and move to register
     *      nop                             ; register dependency Ra
     *
     * Example code to be added by caller
     *      and         Ra,<mask>,Rb        ; mask depends on DSM bit location
     *      nop                             ; register dependency Rb
     *      jlt         Rb,1,@loop
     */
    cs.emit("", "seq_cl_sm", QL_SS2S("S" << smAddr), "# transfer DSM bits to Q1: " + descr);
    cs.emit("", "seq_wait", "3");
    cs.emit("", "move_sm", REG_TMP0);
    cs.emit("", "nop");
    return mask;
}

/*
 * Functions returning a bit
 */


void Functions::op_linv_B(const FncArgs &a) {
    // transfer single breg to REG_TMP0
    UInt mask = emit_bin_cast(a.ops.bregs, 1);

    cs.emit(
        "",
        "and",
        QL_SS2S(REG_TMP0 << "," << mask << "," << REG_TMP1),
        "# mask for '" + a.describe + "'"
    );    // results in '0' for 'bit==0' and 'mask' for 'bit==1'
    cs.emit("", "nop");
    cs.emit(
        "",
        "jge",  // NB: we use "jge" instead of "jlt" to invert
        REG_TMP1 + ",1," + as_target(a.label_if_false),
        "# skip next part if inverted condition is false"
    );
}

void Functions::op_grp_bit_2op_BB(const FncArgs &a) {
    // transfer 2 bregs to REG_TMP0
    UInt mask = emit_bin_cast(a.ops.bregs, 2);

#if 0    // FIXME: handle operation properly, this is just copy/paste from op_linv
    cs.emit("", "and", QL_SS2S(REG_TMP0 << "," << mask << "," << REG_TMP1));    // results in '0' for 'bit==0' and 'mask' for 'bit==1'
    cs.emit("", "nop");
    cs.emit("", "jlt", QL_SS2S(REG_TMP1 << ",1,@" << a.label_if_false), "# " + a.describe);
#endif
    QL_ICE("FIXME: CC backend does not yet support &&,||,^^, expression is '" << a.describe);
}

void Functions::op_grp_rel1_tail(const FncArgs &a) {
    cs.emit("", "nop");    // register dependency
    cs.emit(
        "",
        a.operation,
        REG_TMP0 + ",1," + as_target(a.label_if_false),
        "# skip next part if condition is false"
    );
}

void Functions::op_grp_rel1_CC(const FncArgs &a) {
    cs.emit(
        "",
        "xor",
        as_reg(a.ops.cregs[0]) + "," + as_reg(a.ops.cregs[1]),
        "# " + a.describe
    );
    op_grp_rel1_tail(a);
}

void Functions::op_grp_rel1_Ci_iC(const FncArgs &a) {
    cs.emit(
        "",
        "xor",
        as_reg(a.ops.cregs[0]) + "," + as_int(a.ops.integer),
        "# " + a.describe
    );
    op_grp_rel1_tail(a);
}


void Functions::op_grp_rel2_CC(const FncArgs &a) {
    emit_mnem2args(a, as_reg(a.ops.cregs[0]), as_reg(a.ops.cregs[1]), as_target(a.label_if_false));
}

void Functions::op_grp_rel2_Ci_iC(const FncArgs &a) {
    // NB: for profile "iC" we 'reverse' operands to match Q1 instruction set; this is for free because the operands
    // are split based on their type
    emit_mnem2args(a, as_reg(a.ops.cregs[0]), as_int(a.ops.integer), as_target(a.label_if_false));
}


void Functions::op_gt_CC(const FncArgs &a) {
    // increment arg1 since we lack 'jgt'
    cs.emit(
        "",
        "add",
        "1," + as_reg(a.ops.cregs[1]) + "," + REG_TMP0
    );

    // register dependency
    cs.emit("", "nop");

    // conditional jump
    cs.emit(
        "",
        a.operation,
        as_reg(a.ops.cregs[0]) + "," + REG_TMP0 + as_target(a.label_if_false),
        "# skip next part if condition is false"
    );
}

void Functions::op_gt_Ci(const FncArgs &a) {
    // conditional jump, increment literal since we lack 'jgt'
    cs.emit(
        "",
        a.operation,
        as_reg(a.ops.cregs[0]) + "," + as_int(a.ops.integer, 1) + as_target(a.label_if_false),
        "# skip next part if condition is false"
    );
}

void Functions::op_gt_iC(const FncArgs &a) {
    // conditional jump, deccrement literal since we lack 'jle'
    cs.emit(
        "",
        a.operation,
        as_reg(a.ops.cregs[0]) + "," + as_int(a.ops.integer, -1) + as_target(a.label_if_false),
        "# skip next part if condition is false"
    );
}


/*
 * Functions returning an int
 */

void Functions::op_binv_C(const FncArgs &a) {
    cs.emit(
        "",
        "not",
        as_reg(a.ops.cregs[0]) + "," + as_reg(a.dest_reg),
        "# " + a.describe
    );
}

void Functions::op_grp_int_2op_CC(const FncArgs &a) {
    emit_mnem2args(a, as_reg(a.ops.cregs[0]), as_reg(a.ops.cregs[1]), as_reg(a.dest_reg));
}

void Functions::op_grp_int_2op_Ci_iC(const FncArgs &a) {
    // NB: for profile "iC" we 'reverse' operands to match Q1 instruction set; this is for free because the operands
    // are split based on their type
    emit_mnem2args(a, as_reg(a.ops.cregs[0]), as_int(a.ops.integer), as_reg(a.dest_reg));
}

void Functions::op_sub_iC(const FncArgs &a) {
    // NB: 'reverse' operands to match Q1 instruction set
    emit_mnem2args(a, as_reg(a.ops.cregs[0]), as_int(a.ops.integer), as_reg(a.dest_reg));

    // Negate result in 2's complement to correct for changed op order
    Str reg = as_reg(a.dest_reg);
    cs.emit("", "not", reg);                       // invert
    cs.emit("", "nop");
    cs.emit("", "add", "1,"+reg+","+reg);          // add 1
}

#if OPT_CC_USER_FUNCTIONS
void Functions::rnd_seed_C(const FncArgs &a) {
    // FIXME
}

void Functions::rnd_seed_i(const FncArgs &a) {
    // FIXME
}

void Functions::rnd_C(const FncArgs &a) {
    // FIXME
}

void Functions::rnd_i(const FncArgs &a) {
    // FIXME
}
#endif


/*
 * Function tables
 *
 * The set of functions available here should match that in the platform as set by
 * 'convert_old_to_new(const compat::PlatformRef &old)'.
 * Unfortunately, consistency must currently be maintained manually.
 * FIXME: we might check against ir->platform->functions
 *
 * We maintain separate tables for functions returning an
 * - int (in the context of a SetInstruction); the result is passed to the LHS register
 * - bit (in the context of a logical expression); the result controls a jump (FIXME: which is conceptually distinct from tthe function)
 * FIXME: note that we do not yet support SetInstructions for bits
 *
 * Also see Codegen::handle_set_instruction() and handle_expression()
 */

//name		   profile      func                    operation
#define CC_FUNCTION_LIST_INT \
X("operator~",      C,      op_binv_C,              "") \
\
/* int arithmetic, 2 operands: "+", "-", "&", "|", "^" */ \
X("operator+",      CC,     op_grp_int_2op_CC,      "add") \
X("operator+",      Ci,     op_grp_int_2op_Ci_iC,   "add") \
X("operator+",      iC,     op_grp_int_2op_Ci_iC,   "add") \
X("operator-",      CC,     op_grp_int_2op_CC,      "sub") \
X("operator-",      Ci,     op_grp_int_2op_Ci_iC,   "sub") \
X("operator-",      iC,     op_sub_iC,              "sub") \
X("operator&",      CC,     op_grp_int_2op_CC,      "and") \
X("operator&",      Ci,     op_grp_int_2op_Ci_iC,   "and") \
X("operator&",      iC,     op_grp_int_2op_Ci_iC,   "and") \
X("operator|",      CC,     op_grp_int_2op_CC,      "or") \
X("operator|",      Ci,     op_grp_int_2op_Ci_iC,   "or") \
X("operator|",      iC,     op_grp_int_2op_Ci_iC,   "or") \
X("operator^",      CC,     op_grp_int_2op_CC,      "xor") \
X("operator^",      Ci,     op_grp_int_2op_Ci_iC,   "xor") \
X("operator^",      iC,     op_grp_int_2op_Ci_iC,   "xor") \

#define CC_FUNCTION_LIST_INT_USER \
/* user functions */ \
X("rnd_seed",       C,      rnd_seed_C,             "") \
X("rnd_seed",       i,      rnd_seed_i,             "") \
X("rnd",            C,      rnd_C,                  "") \
X("rnd",            i,      rnd_i,                  "")

#define CC_FUNCTION_LIST_BIT \
\
/* bit arithmetic, 1 operand: "!" */ \
X("operator!",      B,      op_linv_B,              "") \
\
/* bit arithmetic, 2 operands: "&&", "||", "^^" */ \
X("operator&&",     BB,     op_grp_bit_2op_BB,      "") \
X("operator||",     BB,     op_grp_bit_2op_BB,      "") \
X("operator^^",     BB,     op_grp_bit_2op_BB,      "") \
\
/* relop, group 1: "==", "!=" */ \
X("operator==",     CC,     op_grp_rel1_CC,         "jge") \
X("operator==",     Ci,     op_grp_rel1_Ci_iC,      "jge") \
X("operator==",     iC,     op_grp_rel1_Ci_iC,      "jge") \
/* repeat, with operation inversed */ \
X("operator!=",     CC,     op_grp_rel1_CC,         "jlt") \
X("operator!=",     Ci,     op_grp_rel1_Ci_iC,      "jlt") \
X("operator!=",     iC,     op_grp_rel1_Ci_iC,      "jlt") \
\
/* relop, group 2: ">=", "<" */ \
X("operator>=",     CC,     op_grp_rel2_CC,         "jge") \
X("operator>=",     Ci,     op_grp_rel2_Ci_iC,      "jge") \
X("operator>=",     iC,     op_grp_rel2_Ci_iC,      "jlt") /* inverse operation */ \
/* repeat, with inverse operation */ \
X("operator<",      CC,     op_grp_rel2_CC,         "jlt") \
X("operator<",      Ci,     op_grp_rel2_Ci_iC,      "jlt") \
X("operator<",      iC,     op_grp_rel2_Ci_iC,      "jge") /* inverse operation */ \
\
/* relop, group 3: ">", "<=" */ \
X("operator>",      CC,     op_gt_CC,               "jge") \
X("operator>",      Ci,     op_gt_Ci,               "jge") \
X("operator>",      iC,     op_gt_iC,               "jlt") /* inverse operation */ \
/* repeat, with inverse operation */ \
X("operator<=",     CC,     op_gt_CC,               "jlt") \
X("operator<=",     Ci,     op_gt_Ci,               "jlt") \
X("operator<=",     iC,     op_gt_iC,               "jge") /* inverse operation */



void Functions::register_functions() {
    // Initialize func_map_int with the functions from CC_FUNCTION_LIST and optionally CC_FUNCTION_LIST_USER.
    // The key consists of the concatenation of name, "_" and the stringified profile, e.g "operator~_C"

    #define X(name, profile, func, operation) { name "_" #profile, { &Functions::func, operation } } ,
    func_map_int = {
        CC_FUNCTION_LIST_INT
        #if OPT_CC_USER_FUNCTIONS
         CC_FUNCTION_LIST_INT_USER
        #endif
    };
    func_map_bit = {
        CC_FUNCTION_LIST_BIT
    };
    #undef X
}


void Functions::emit_mnem2args(const FncArgs &a, const Str &arg0, const Str &arg1, const Str &target) {
    cs.emit(
        "",
        a.operation, // mnemonic
        arg0 +  "," + arg1 + "," + target,
        "# " + a.describe
    );
}

} // namespace detail
} // namespace vq1asm
} // namespace gen
} // namespace pass
} // namespace cc
} // namespace arch
} // namespace ql
