// FIXME: WIP on splitting off function handling from codegen.cc

#include "functions.h"
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
static Str as_reg(Int creg) {
    if(creg >= NUM_CREGS) {
        QL_INPUT_ERROR("register index " << creg << " exceeds maximum of " << NUM_CREGS);
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
        profile += get_profile(op);
    }

    // set dest_reg
    opArgs.dest_reg = creg2reg(*lhs->as_reference());


    // FIXME:
    // check creg, breg range



    // match name and operand profile against table of available functions


    // call the function


}



void Functions::op_binv__C(const OpArgs &a) {
    emit(
        "",
        "not",
        as_reg(a.ops.cregs[0]) + "," + as_reg(a.dest_reg),
        "# " + ir::describe(a.expression)
    );
}

void Functions::op_linv__B(const OpArgs &a) {
    UInt mask = emit_bin_cast(a.ops.bregs, 1);

    emit(
        "",
        "and",
        QL_SS2S(REG_TMP0 << "," << mask << "," << REG_TMP1),
        "# mask for '" + ir::describe(a.expression) + "'"
    );    // results in '0' for 'bit==0' and 'mask' for 'bit==1'
    emit("", "nop");
    emit(
        "",
        "jge",  // NB: we use "jge" instead of "jlt" to invert
        QL_SS2S(REG_TMP1 << ",1," << as_target(a.label_if_false)),
        "# skip next part if inverted condition is false"
    );
}


void Functions::op_grp_int_2op__CC(const OpArgs &a) {
    emit_mnem2args(a, as_reg(a.ops.cregs[0]), as_reg(a.ops.cregs[1]), as_reg(a.dest_reg));
}

void Functions::op_grp_int_2op__Ci_iC(const OpArgs &a) {
    // NB: for profile "iC" we 'reverse' operands to match Q1 instruction set; this is for free because the operands
    // are split based on their type
    emit_mnem2args(a, as_reg(a.ops.cregs[0]), as_int(a.ops.integer), as_reg(a.dest_reg));
}

void Functions::op_sub__iC(const OpArgs &a) {
    // NB: 'reverse' operands to match Q1 instruction set
    emit_mnem2args(a, as_reg(a.ops.cregs[0]), as_int(a.ops.integer), as_reg(a.dest_reg));

    // Negate result in 2's complement to correct for changed op order
    Str reg = as_reg(a.dest_reg);
    emit("", "not", reg);                       // invert
    emit("", "nop");
    emit("", "add", "1,"+reg+","+reg);          // add 1
}


void Functions::op_grp_bit_2op__BB(const OpArgs &a) {
    UInt mask = emit_bin_cast(a.ops.bregs, 2);
#if 0    // FIXME: handle operation properly, this is just copy/paste from op_linv
    emit("", "and", QL_SS2S(REG_TMP0 << "," << mask << "," << REG_TMP1));    // results in '0' for 'bit==0' and 'mask' for 'bit==1'
    emit("", "nop");
    emit("", "jlt", QL_SS2S(REG_TMP1 << ",1,@" << a.label_if_false), "# " + ir::describe(a.expression));
#endif
    QL_ICE("CC backend does not yet support &&,||,^^, expression is '" << ir::describe(a.expression));
}


void Functions::op_grp_rel1_tail(const OpArgs &a) {
    emit("", "nop");    // register dependency
    emit(
        "",
        a.operation,
        Str(REG_TMP0)+",1,"+as_target(a.label_if_false),
        "# skip next part if condition is false"
    );
}

void Functions::op_grp_rel1__CC(const OpArgs &a) {
    emit(
        "",
        "xor",
        as_reg(a.ops.cregs[0]) + "," + as_reg(a.ops.cregs[1]),
        "# skip next part if condition is false"
    );
    op_grp_rel1_tail(a);
}

void Functions::op_grp_rel1__Ci_iC(const OpArgs &a) {
    emit(
        "",
        "xor",
        as_reg(a.ops.cregs[0]) + "," + as_int(a.ops.integer),
        "# skip next part if condition is false"
    );
    op_grp_rel1_tail(a);
}


void Functions::op_grp_rel2__CC(const OpArgs &a) {
    emit_mnem2args(a, as_reg(a.ops.cregs[0]), as_reg(a.ops.cregs[1]), as_target(a.label_if_false));
}

void Functions::op_grp_rel2__Ci_iC(const OpArgs &a) {
    // NB: for profile "iC" we 'reverse' operands to match Q1 instruction set; this is for free because the operands
    // are split based on their type
    emit_mnem2args(a, as_reg(a.ops.cregs[0]), as_int(a.ops.integer), as_target(a.label_if_false));
}


void Functions::op_gt__CC(const OpArgs &a) {
    // increment arg1 since we lack 'jgt'
    emit(
        "",
        "add",
        "1," + as_reg(a.ops.cregs[1]) + "," + REG_TMP0
    );

    // register dependency
    emit("", "nop");

    // conditional jump
    emit(
        "",
        a.operation,
        as_reg(a.ops.cregs[0]) + "," + REG_TMP0 + as_target(a.label_if_false),
        "# skip next part if condition is false"
    );
}

void Functions::op_gt__Ci(const OpArgs &a) {
    // conditional jump, increment literal since we lack 'jgt'
    emit(
        "",
        a.operation,
        as_reg(a.ops.cregs[0]) + "," + as_int(a.ops.integer, 1) + as_target(a.label_if_false),
        "# skip next part if condition is false"
    );
}

void Functions::op_gt__iC(const OpArgs &a) {
    // conditional jump, deccrement literal since we lack 'jle'
    emit(
        "",
        a.operation,
        as_reg(a.ops.cregs[0]) + "," + as_int(a.ops.integer, -1) + as_target(a.label_if_false),
        "# skip next part if condition is false"
    );
}



/*
 * Function table
 *
 * The set of available names should match that in platform as set by 'convert_old_to_new(const compat::PlatformRef &old)'
 * FIXME: copy more of the comment of Codegen::do_handle_expression
 */

// FIXME: add return type
//name		   profile  func                    operation
#define CC_FUNCTION_LIST \
X("operator~",      C,      op_binv__C,             "") \
\
/* bit arithmetic, 1 operand: "!" */ \
X("operator!",      B,      op_linv__B,             "") \
\
/* int arithmetic, 2 operands: "+", "-", "&", "|", "^" */ \
X("operator+",      CC,     op_grp_int_2op__CC,     "add") \
X("operator+",      Ci,     op_grp_int_2op__Ci_iC,  "add") \
X("operator+",      iC,     op_grp_int_2op__Ci_iC,  "add") \
X("operator-",      CC,     op_grp_int_2op__CC,     "sub") \
X("operator-",      Ci,     op_grp_int_2op__Ci_iC,  "sub") \
X("operator-",      iC,     op_sub__iC,             "sub") \
X("operator&",      CC,     op_grp_int_2op__CC,     "and") \
X("operator&",      Ci,     op_grp_int_2op__Ci_iC,  "and") \
X("operator&",      iC,     op_grp_int_2op__Ci_iC,  "and") \
X("operator|",      CC,     op_grp_int_2op__CC,     "or") \
X("operator|",      Ci,     op_grp_int_2op__Ci_iC,  "or") \
X("operator|",      iC,     op_grp_int_2op__Ci_iC,  "or") \
X("operator^",      CC,     op_grp_int_2op__CC,     "xor") \
X("operator^",      Ci,     op_grp_int_2op__Ci_iC,  "xor") \
X("operator^",      iC,     op_grp_int_2op__Ci_iC,  "xor") \
\
/* bit arithmetic, 2 operands: "&&", "||", "^^" */ \
X("operator&&",     BB,     op_grp_bit_2op__BB,     "") \
X("operator||",     BB,     op_grp_bit_2op__BB,     "") \
X("operator^^",     BB,     op_grp_bit_2op__BB,     "") \
\
/* relop, group 1: "==", "!=" */ \
X("operator==",     CC,     op_grp_rel1__CC,        "jge") \
X("operator==",     Ci,     op_grp_rel1__Ci_iC,     "jge") \
X("operator==",     iC,     op_grp_rel1__Ci_iC,     "jge") \
/* repeat, with operation inversed */ \
X("operator!=",     CC,     op_grp_rel1__CC,        "jlt") \
X("operator!=",     Ci,     op_grp_rel1__Ci_iC,     "jlt") \
X("operator!=",     iC,     op_grp_rel1__Ci_iC,     "jlt") \
\
/* relop, group 2: ">=", "<" */ \
X("operator>=",     CC,     op_grp_rel2__CC,        "jge") \
X("operator>=",     Ci,     op_grp_rel2__Ci_iC,     "jge") \
X("operator>=",     iC,     op_grp_rel2__Ci_iC,     "jlt") /* inverse operation */ \
/* repeat, with inverse operation */ \
X("operator<",      CC,     op_grp_rel2__CC,        "jlt") \
X("operator<",      Ci,     op_grp_rel2__Ci_iC,     "jlt") \
X("operator<",      iC,     op_grp_rel2__Ci_iC,     "jge") /* inverse operation */ \
\
/* relop, group 3: ">", "<=" */ \
X("operator>",      CC,     op_gt__CC,              "jge") \
X("operator>",      Ci,     op_gt__Ci,              "jge") \
X("operator>",      iC,     op_gt__iC,              "jlt") /* inverse operation */ \
/* repeat, with inverse operation */ \
X("operator<=",     CC,     op_gt__CC,              "jlt") \
X("operator<=",     Ci,     op_gt__Ci,              "jlt") \
X("operator<=",     iC,     op_gt__iC,              "jge") /* inverse operation */ \
\
/* user functions */ \
X("rnd_seed",       -,      rnd_seed,               "") \
X("rnd",            -,      rnd,                    "")

#define X(name, func, profile, operation) name,
const char *names[] = {
CC_FUNCTION_LIST
};
#undef X


Str Functions::get_profile(const ir::ExpressionRef &op) {
    if(op->as_int_literal()) {
        return "i";
    } else if(op->as_bit_literal()) {
        return "b";
    } else if(op->as_reference()) {
        if(operandContext.is_creg_reference(op)) {
            return "C";
        } else if(operandContext.is_breg_reference(op)) {
            return "B";
        } else {
            QL_ICE("Operand '" << ir::describe(op) << "' has unsupported type");
        }
    } else if(op->as_function_call()) {
        QL_INPUT_ERROR("cannot currently handle function call within function call '" << ir::describe(op) << "'");
    } else {
        QL_INPUT_ERROR("cannot currently handle parameter '" << ir::describe(op) << "'");
    }
}

#if 0
// FIXME: uses dp.
UInt Functions::emit_bin_cast(Any<ir::Expression> operands, Int expOpCnt) {
    if(operands.size() != expOpCnt) {
        QL_ICE("Expected " << expOpCnt << " bit operands, got " << operands.size());
    }

    // Compute DSM address and mask for operands.
    UInt smAddr = 0;
    UInt mask = 0;      // mask for used SM bits in 32 bit word transferred using move_sm
    Str descr;
    for (Int i=0; i<operands.size(); i++) {
        auto &op = operands[i];

        // Convert breg reference to the register index
        Int breg;
        if(op->as_reference()) {
            breg = operandContext.convert_breg_reference(op);
            if(breg >= NUM_BREGS) {
                QL_INPUT_ERROR("bit register index " << breg << " exceeds maximum");
            }
        } else {
            QL_ICE("Expected bit operand, got '" << ir::describe(op) << "'");
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
}
#else
UInt Functions::emit_bin_cast(utils::Vec<utils::UInt> bregs, Int expOpCnt) {
    if(bregs.size() != expOpCnt) {
        QL_ICE("Expected " << expOpCnt << " breg operands, got " << bregs.size());
    }

    // Compute DSM address and mask for operands.
    UInt smAddr = 0;
    UInt mask = 0;      // mask for used SM bits in 32 bit word transferred using move_sm
    Str descr;
    for (Int i=0; i<bregs.size(); i++) {
        Int breg = bregs[i];
        if(breg >= NUM_BREGS) {
            QL_INPUT_ERROR("bit register index " << breg << " exceeds maximum of " << NUM_BREGS);   // FIXME: cleanup "breg" vs. "bit register index"
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
}
#endif



#if 0
void Functions::emit_mnem2args(const OpArgs &a, Int arg0, Int arg1, const Str &target) {
    emit(
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
    emit(
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
