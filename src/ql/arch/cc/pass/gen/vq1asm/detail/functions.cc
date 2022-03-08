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


// helpers
static void check_int_literal(const ir::IntLiteral &ilit, Int bottomRoom=0, Int headRoom=0) {
    if(ilit.value-bottomRoom < 0) {
        QL_INPUT_ERROR("CC backend cannot handle negative integer literals: value=" << ilit.value << ", bottomRoom=" << bottomRoom);
    }
    if(ilit.value >= (1LL<<32) - 1 - headRoom) {
        QL_INPUT_ERROR("CC backend requires integer literals limited to 32 bits: value=" << ilit.value << ", headRoom=" << headRoom);
    }
}


void Functions::dispatch(const Str &name, const ir::ExpressionRef &expression) {
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




void Functions::op_binv_x(const OpArgs &a) {
    emit(
        "",
        "not",
        QL_SS2S(
            expr2q1Arg(a.operands[0])
            << ",R" << dest_reg()
        )
        , "# " + ir::describe(a.expression)
    );
}

void Functions::op_linv_b(const OpArgs &a) {
    UInt mask = emit_bin_cast(a.operands, 1);

    emit(
        "",
        "and",
        QL_SS2S(REG_TMP0 << "," << mask << "," << REG_TMP1),
        "# mask for '" + ir::describe(a.expression) + "'"
    );    // results in '0' for 'bit==0' and 'mask' for 'bit==1'
    emit("", "nop");
    emit("", "jge", QL_SS2S(REG_TMP1 << ",1,@" << a.label_if_false), "# skip next part if inverted condition is false");  // NB: we use "jge" instead of "jlt" to invert
}


void Functions::op_grp_int_2op_CC(const OpArgs &a) {
    emit_mnem2args(a, QL_SS2S(a.ops.cregs[0]), QL_SS2S(a.ops.cregs[1]), QL_SS2S("R"<<a.dest_reg));
}

void Functions::op_grp_int_2op_Ci(const OpArgs &a) {
    emit_mnem2args(a, QL_SS2S(a.ops.cregs[0]), QL_SS2S("R"<<a.ops.integer), QL_SS2S("R"<<a.dest_reg));
}

void Functions::op_grp_int_2op_iC(const OpArgs &a) {
    // reverse operands to match Q1 instruction set
    // FIXME: reversal is for free, because we split types
    emit_mnem2args(a, QL_SS2S(a.ops.cregs[0]), QL_SS2S("R"<<a.ops.integer), QL_SS2S("R"<<a.dest_reg));
}


void Functions::op_sub_iC(const OpArgs &a) {
    emit_mnem2args(a, 1, 0, QL_SS2S("R"<<dest_reg()));   // reverse operands to match Q1 instruction set
    // Negate result in 2's complement to correct for changed op order
    Str reg = QL_SS2S("R"<<dest_reg());
    emit("", "not", reg);                       // invert
    emit("", "nop");
    emit("", "add", "1,"+reg+","+reg);          // add 1
}

void Functions::op_grp_bit_2op(const OpArgs &a) {
    UInt mask = emit_bin_cast(a.operands, 2);
    // FIXME: handle operation properly
    emit("", "and", QL_SS2S(REG_TMP0 << "," << mask << "," << REG_TMP1));    // results in '0' for 'bit==0' and 'mask' for 'bit==1'
    emit("", "nop");
    emit("", "jlt", QL_SS2S(REG_TMP1 << ",1,@" << a.label_if_false), "# " + ir::describe(a.expression));
    QL_ICE("CC backend does not yet support " << fn->function_type->name);
}

void Functions::op_grp_rel1_rx(const OpArgs &a) {
    emit_mnem2args("xor", 0, 1);
    emit("", "nop");    // register dependency
    emit("", a.operation, Str(REG_TMP0)+",1,@"+a.label_if_false, "# skip next part if condition is false");
}

void Functions::op_grp_rel1_lr(const OpArgs &a) {
    emit_mnem2args("xor", 1, 0);   // reverse operands to match Q1 instruction set
    // FIXME: common with previous function
    emit("", "nop");    // register dependency
    emit("", a.operation, Str(REG_TMP0)+",1,@"+a.label_if_false, "# skip next part if condition is false");
}

void Functions::op_grp_rel2_rx(const OpArgs &a) {
    emit_mnem2args(a, 0, 1, as_target(a.label_if_false)); break;
}

void Functions::op_grp_rel2_lr(const OpArgs &a) {
    emit_mnem2args(a, 1, 0, as_target(a.label_if_false)); break;   // reverse operands (and instruction) to match Q1 instruction set
}

void Functions::op_gt_rl(const OpArgs &a) {
    check_int_literal(*a.operands[1]->as_int_literal(), 0, 1);
    emit(
        "",
        a.operation,
        QL_SS2S(
            expr2q1Arg(a.operands[0]) << ","
            << a.operands[1]->as_int_literal()->value + 1    // increment literal since we lack 'jgt'
            << ",@"+a.label_if_false
        ),
        "# skip next part if condition is false"
    );
}

void Functions::op_gt_rr(const OpArgs &a) {
    emit(
        "",
        "add",
        QL_SS2S(
            "1,"
            << expr2q1Arg(a.operands[1])
            << "," << REG_TMP0
        )
    );                      // increment arg1
    emit("", "nop");        // register dependency
    emit(
        "",
        a.operation,
        QL_SS2S(
            expr2q1Arg(a.operands[0])
            << "," << REG_TMP0
            << ",@"+a.label_if_false
        ),
        "# skip next part if condition is false"
    );
}

void Functions::op_gt_lr(const OpArgs &a) {
    check_int_literal(*a.operands[0]->as_int_literal(), 1, 0);
    emit(
        "",
        a.operation,
        QL_SS2S(
            expr2q1Arg(a.operands[1])     // reverse operands
            << a.operands[0]->as_int_literal()->value - 1    // DECrement literal since we lack 'jle'
            << ",@"+a.label_if_false
        ),
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
//name		                func stem           profile operation
#define CC_FUNCTION_LIST \
X("operator~",              op_binv,            x,      "") \
\
/* bit arithmetic, 1 operand: "!" */ \
X("operator!",              op_linv,            b,      "") \
\
/* int arithmetic, 2 operands: "+", "-", "&", "|", "^" */ \
X("operator+",              op_grp_int_2op,     CC,     "add") \
X("operator+",              op_grp_int_2op,     Ci,     "add") \
X("operator+",              op_grp_int_2op,     iC,     "add") \
X("operator-",              op_grp_int_2op,     CC,     "sub") \
X("operator-",              op_grp_int_2op,     Ci,     "sub") \
X("operator-",              op_sub,             iC,     "sub") \
X("operator&",              op_grp_int_2op,     CC,     "and") \
X("operator&",              op_grp_int_2op,     Ci,     "and") \
X("operator&",              op_grp_int_2op,     iC,     "and") \
X("operator|",              op_grp_int_2op,     CC,     "or") \
X("operator|",              op_grp_int_2op,     Ci,     "or") \
X("operator|",              op_grp_int_2op,     iC,     "or") \
X("operator^",              op_grp_int_2op,     CC,     "xor") \
X("operator^",              op_grp_int_2op,     Ci,     "xor") \
X("operator^",              op_grp_int_2op,     iC,     "xor") \
\
/* bit arithmetic, 2 operands: "&&", "||", "^^" */ \
X("operator&&",             op_grp_bit_2op,     bb,     "") \
X("operator||",             op_grp_bit_2op,     bb,     "") \
X("operator^^",             op_grp_bit_2op,     bb,     "") \
\
/* relop, group 1: "==", "!=" */ \
X("operator==",             op_grp_rel1,        rx,     "jge") \
X("operator==",             op_grp_rel1,        lr,     "jge") \
/* repeat, with operation inversed */ \
X("operator!=",             op_grp_rel1,        rx,     "jlt") \
X("operator!=",             op_grp_rel1,        lr,     "jlt") \
\
/* relop, group 2: ">=", "<" */ \
X("operator>=",             op_grp_rel2,        rx,     "jge") \
X("operator>=",             op_grp_rel2,        lr,     "jlt") \
/* repeat, with inverse operation */ \
X("operator<",              op_grp_rel2,        rx,     "jlt") \
X("operator<",              op_grp_rel2,        lr,     "jge") \
\
/* relop, group 3: ">", "<=" */ \
X("operator>",              op_gt,              rl,     "jge") \
X("operator>",              op_gt,              rr,     "jge") \
X("operator>",              op_gt,              lr,     "jlt") /* inverse operation */ \
/* repeat, with inverse operation */ \
X("operator<=",             op_gt,              rl,     "jlt") \
X("operator<=",             op_gt,              rr,     "jlt") \
X("operator<=",             op_gt,              lr,     "jge") \
\
/* user functions */ \
X("rnd_seed",               rnd_seed,           -,       "") \
X("rnd",                    rnd,                -,       "")

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

} // namespace detail
} // namespace vq1asm
} // namespace gen
} // namespace pass
} // namespace cc
} // namespace arch
} // namespace ql
