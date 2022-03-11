#include "functions.h"
#include "codegen.h"

#include "ql/ir/describe.h"

namespace ql {
namespace arch {
namespace cc {
namespace pass {
namespace gen {
namespace vq1asm {
namespace detail {

using namespace utils;

// FIXME: copied from codegen.cc
static Str as_target(const Str &label) { return "@" + label; }

// new
static Str as_reg(UInt creg) {
    if(creg >= NUM_CREGS) {
        QL_INPUT_ERROR("register index " << creg << " exceeds maximum of " << NUM_CREGS-1);
    }
    return QL_SS2S("R"<<creg);
}

static Str as_int(Int val, Int add=0) {
    if(val+add < 0) {
        // FIXME: improve message, show expression
        QL_INPUT_ERROR("CC backend cannot handle negative integer literals: value=" << val << ", add=" << add);
    }
    if(val >= (1LL<<32) - 1 - add) {
        QL_INPUT_ERROR("CC backend requires integer literals to fit 32 bits: value=" << val << ", add=" << add);
    }
    return QL_SS2S(val+add);
}

// FIXME: add const to parameters
Functions::Functions(OperandContext &operandContext, Datapath &dp, Codegen &cg)
    : operandContext(operandContext)
    , dp(dp)
    , cg(cg) {
    register_functions();
}

void Functions::dispatch(const Str &name, const ir::ExpressionRef &lhs, const ir::ExpressionRef &expression) {
    // check parameter
    auto fn = expression->as_function_call();
    if(!fn) QL_ICE("Expected function call, got '" << ir::describe(expression) << "'");

    // collect arguments for operator functions
    OpArgs opArgs(expression);

    // split operands into different types, and determine profile
    Str profile;
    for(auto &op : fn->operands) {
        opArgs.ops.append(operandContext, op);

        // FIXME: maintain  profile in ops.append
        profile += get_operand_type(op);
    }

    // set dest_reg
    if(lhs.empty()) {
        opArgs.dest_reg = 0;
    } else {
        opArgs.dest_reg = creg2reg(*lhs->as_reference());
    }


    // FIXME:
    // check creg, breg range



    // create key from name and operand profile, using the same encoding as register_functions()
    Str key = fn->function_type->name + "_" + profile;

    // lookup key
    auto it = func_map.find(key);
    if (it == func_map.end()) {
        /*
         * NB: if we arrive here, there's an inconsistency between the functions registered in 'ql::ir::cqasm:read()'
         * (see comment at beginning of Codegen::do_handle_expression) and those available in func_map.
         */
        QL_ICE(
            "function '" << fn->function_type->name
            << "' with profile '" << profile
            << "' not supported by CC backend, but it should be"
        );
    }

    // finish arguments
    opArgs.operation = it->second.operation;

    // call the function
    (this->*it->second.func)(opArgs);
}



void Functions::op_binv_C(const OpArgs &a) {
    cg.emit(
        "",
        "not",
        as_reg(a.ops.cregs[0]) + "," + as_reg(a.dest_reg),
        "# " + ir::describe(a.expression)
    );
}

void Functions::op_linv_B(const OpArgs &a) {
    // transfer single breg to REG_TMP0
    UInt mask = emit_bin_cast(a.ops.bregs, 1);

    cg.emit(
        "",
        "and",
        QL_SS2S(REG_TMP0 << "," << mask << "," << REG_TMP1),
        "# mask for '" + ir::describe(a.expression) + "'"
    );    // results in '0' for 'bit==0' and 'mask' for 'bit==1'
    cg.emit("", "nop");
    cg.emit(
        "",
        "jge",  // NB: we use "jge" instead of "jlt" to invert
        QL_SS2S(REG_TMP1 << ",1," << as_target(a.label_if_false)),
        "# skip next part if inverted condition is false"
    );
}


void Functions::op_grp_int_2op_CC(const OpArgs &a) {
    emit_mnem2args(a, as_reg(a.ops.cregs[0]), as_reg(a.ops.cregs[1]), as_reg(a.dest_reg));
}

void Functions::op_grp_int_2op_Ci_iC(const OpArgs &a) {
    // NB: for profile "iC" we 'reverse' operands to match Q1 instruction set; this is for free because the operands
    // are split based on their type
    emit_mnem2args(a, as_reg(a.ops.cregs[0]), as_int(a.ops.integer), as_reg(a.dest_reg));
}

void Functions::op_sub_iC(const OpArgs &a) {
    // NB: 'reverse' operands to match Q1 instruction set
    emit_mnem2args(a, as_reg(a.ops.cregs[0]), as_int(a.ops.integer), as_reg(a.dest_reg));

    // Negate result in 2's complement to correct for changed op order
    Str reg = as_reg(a.dest_reg);
    cg.emit("", "not", reg);                       // invert
    cg.emit("", "nop");
    cg.emit("", "add", "1,"+reg+","+reg);          // add 1
}


void Functions::op_grp_bit_2op_BB(const OpArgs &a) {
    // transfer 2 bregs to REG_TMP0
    UInt mask = emit_bin_cast(a.ops.bregs, 2);

#if 0    // FIXME: handle operation properly, this is just copy/paste from op_linv
    cg.emit("", "and", QL_SS2S(REG_TMP0 << "," << mask << "," << REG_TMP1));    // results in '0' for 'bit==0' and 'mask' for 'bit==1'
    cg.emit("", "nop");
    cg.emit("", "jlt", QL_SS2S(REG_TMP1 << ",1,@" << a.label_if_false), "# " + ir::describe(a.expression));
#endif
    QL_ICE("CC backend does not yet support &&,||,^^, expression is '" << ir::describe(a.expression));
}


void Functions::op_grp_rel1_tail(const OpArgs &a) {
    cg.emit("", "nop");    // register dependency
    cg.emit(
        "",
        a.operation,
        Str(REG_TMP0)+",1,"+as_target(a.label_if_false),
        "# skip next part if condition is false"
    );
}

void Functions::op_grp_rel1_CC(const OpArgs &a) {
    cg.emit(
        "",
        "xor",
        as_reg(a.ops.cregs[0]) + "," + as_reg(a.ops.cregs[1]),
        "# skip next part if condition is false"
    );
    op_grp_rel1_tail(a);
}

void Functions::op_grp_rel1_Ci_iC(const OpArgs &a) {
    cg.emit(
        "",
        "xor",
        as_reg(a.ops.cregs[0]) + "," + as_int(a.ops.integer),
        "# skip next part if condition is false"
    );
    op_grp_rel1_tail(a);
}


void Functions::op_grp_rel2_CC(const OpArgs &a) {
    emit_mnem2args(a, as_reg(a.ops.cregs[0]), as_reg(a.ops.cregs[1]), as_target(a.label_if_false));
}

void Functions::op_grp_rel2_Ci_iC(const OpArgs &a) {
    // NB: for profile "iC" we 'reverse' operands to match Q1 instruction set; this is for free because the operands
    // are split based on their type
    emit_mnem2args(a, as_reg(a.ops.cregs[0]), as_int(a.ops.integer), as_target(a.label_if_false));
}


void Functions::op_gt_CC(const OpArgs &a) {
    // increment arg1 since we lack 'jgt'
    cg.emit(
        "",
        "add",
        "1," + as_reg(a.ops.cregs[1]) + "," + REG_TMP0
    );

    // register dependency
    cg.emit("", "nop");

    // conditional jump
    cg.emit(
        "",
        a.operation,
        as_reg(a.ops.cregs[0]) + "," + REG_TMP0 + as_target(a.label_if_false),
        "# skip next part if condition is false"
    );
}

void Functions::op_gt_Ci(const OpArgs &a) {
    // conditional jump, increment literal since we lack 'jgt'
    cg.emit(
        "",
        a.operation,
        as_reg(a.ops.cregs[0]) + "," + as_int(a.ops.integer, 1) + as_target(a.label_if_false),
        "# skip next part if condition is false"
    );
}

void Functions::op_gt_iC(const OpArgs &a) {
    // conditional jump, deccrement literal since we lack 'jle'
    cg.emit(
        "",
        a.operation,
        as_reg(a.ops.cregs[0]) + "," + as_int(a.ops.integer, -1) + as_target(a.label_if_false),
        "# skip next part if condition is false"
    );
}


#if OPT_CC_USER_FUNCTIONS
void Functions::rnd_seed_C(const OpArgs &a) {
}

void Functions::rnd_seed_i(const OpArgs &a) {
}

void Functions::rnd(const OpArgs &a) {
}
#endif


/*
 * Function table
 *
 * The set of available functions should match that in the platform as set by
 * 'convert_old_to_new(const compat::PlatformRef &old)'
 *
 * Also see Codegen::do_handle_expression()
 */

// FIXME: add return type
//name		   profile      func                    operation
#define CC_FUNCTION_LIST \
X("operator~",      C,      op_binv_C,             "") \
\
/* bit arithmetic, 1 operand: "!" */ \
X("operator!",      B,      op_linv_B,              "") \
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

#define CC_FUNCTION_LIST_USER \
/* user functions */ \
X("rnd_seed",       C,      rnd_seed_C,             "") \
X("rnd_seed",       i,      rnd_seed_i,             "") \
X("rnd",            -,      rnd,                    "")


void Functions::register_functions() {
//    #define X(name, profile, func, operation) { name, #profile, &Functions::func, operation } ,
//    struct {
//        Str name;
//        Str profile;
//        tOpFunc func;
//        Str operation;
//    } func_table[] = {
//        CC_FUNCTION_LIST
//        #if OPT_CC_USER_FUNCTIONS
//         CC_FUNCTION_LIST_USER
//        #endif
//    };
//    #undef X

    /*
     * Initialize func_map with the functions from CC_FUNCTION_LIST and optionally CC_FUNCTION_LIST_USER.
     * The key consists of the concatenation of name, "_" and the stringified profile, e.g "operator~_C"
     */


    #define X(name, profile, func, operation) { name "_" #profile, { &Functions::func, operation } } ,
    func_map = {
        CC_FUNCTION_LIST
        #if OPT_CC_USER_FUNCTIONS
         CC_FUNCTION_LIST_USER
        #endif
    };
    #undef X
}

Str Functions::get_operand_type(const ir::ExpressionRef &op) {
    if(op->as_int_literal()) {
        return "i";
    } else if(op->as_bit_literal()) {
        return "b";
    } else if(op->as_reference()) {
        if(operandContext.is_creg_reference(op)) {
            return "C";
        } else if(operandContext.is_breg_reference(op)) {
            return "B";
        } else if(operandContext.is_implicit_breg_reference(op)) {
            return "B"; // FIXME: WIP
        } else {
            QL_ICE("operand '" << ir::describe(op) << "' has unsupported type");
        }
    } else if(op->as_function_call()) {
        QL_INPUT_ERROR("cannot currently handle function call within function call '" << ir::describe(op) << "'");
    } else {
        QL_INPUT_ERROR("cannot currently handle parameter '" << ir::describe(op) << "'");
    }
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
    cg.emit("", "seq_cl_sm", QL_SS2S("S" << smAddr), "# transfer DSM bits to Q1: " + descr);
    cg.emit("", "seq_wait", "3");
    cg.emit("", "move_sm", REG_TMP0);
    cg.emit("", "nop");
    return mask;
}


#if 0
void Functions::emit_mnem2args(const OpArgs &a, Int arg0, Int arg1, const Str &target) {
    cg.emit(
        "",
        a.operation, // mnemonic
        QL_SS2S(
            expr2q1Arg(a.operands[arg0])
            << "," << expr2q1Arg(a.operands[arg1])
            << "," << target
        )
        , "# " + ir::describe(a.expression)
    );
}
#endif

void Functions::emit_mnem2args(const OpArgs &a, const Str &arg0, const Str &arg1, const Str &target) {
    cg.emit(
        "",
        a.operation, // mnemonic
        QL_SS2S(
            arg0
            << "," << arg1
            << "," << target
        )
        , "# " + ir::describe(a.expression)
    );
}


Int Functions::creg2reg(const ir::Reference &ref) {
    auto reg = operandContext.convert_creg_reference(ref);
    if(reg >= NUM_CREGS) {
        QL_INPUT_ERROR("register index " << reg << " exceeds maximum");
    }
    return reg;
};

#if 0
Int Functions::dest_reg(const ir::ExpressionRef &lhs) {
    return creg2reg(*lhs->as_reference());
};

// FIXME: resolve operands based on profile
Str Functions::expr2q1Arg(const ir::ExpressionRef &op) {
    if(op->as_reference()) {
        return QL_SS2S("R" << creg2reg(*op->as_reference()));
    } else if(auto ilit = op->as_int_literal()) {
        check_int_literal(*ilit);
        return QL_SS2S(ilit->value);
    } else {
        QL_ICE("Expected integer operand");
    }
};
#endif

} // namespace detail
} // namespace vq1asm
} // namespace gen
} // namespace pass
} // namespace cc
} // namespace arch
} // namespace ql
