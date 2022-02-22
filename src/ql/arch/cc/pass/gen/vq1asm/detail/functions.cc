// FIXME: WIP on splitting off function handling from codegen.cc

#include "functions.h"

namespace ql {
namespace arch {
namespace cc {
namespace pass {
namespace gen {
namespace vq1asm {
namespace detail {

using namespace utils;

/**
 * Function naming follows libqasm's func_gen::Function::unique_name
 */

typedef struct {
    ir::Any<ir::Expression> operands;
} OpArgs;

// bitwise inversion
void op_binv_x(const OpArgs &a) {
    emit(
        "",
        "not",
        QL_SS2S(
            expr2q1Arg(a.operands[0])
            << ",R" << dest_reg()
        )
        , "# " + ir::describe(expression)
    );
}

// logical inversion
void op_linv_b(ir::Any<ir::Expression> operands) {
    UInt mask = emit_bin_cast(operands, 1);

    emit(
        "",
        "and",
        QL_SS2S(REG_TMP0 << "," << mask << "," << REG_TMP1),
        "# mask for '" + ir::describe(expression) + "'"
    );    // results in '0' for 'bit==0' and 'mask' for 'bit==1'
    emit("", "nop");
    emit("", "jge", QL_SS2S(REG_TMP1 << ",1,@" << label_if_false), "# skip next part if inverted condition is false");  // NB: we use "jge" instead of "jlt" to invert
}

// int arithmetic, 2 operands: "+", "-", "&", "|", "^"
void op_grp_int_2op_rx() {
    emit_mnem2args(operation, 0, 1, QL_SS2S("R"<<dest_reg()));
}

void op_grp_int_2op_lr() {
    emit_mnem2args(operation, 1, 0, QL_SS2S("R"<<dest_reg()));   // reverse operands to match Q1 instruction set
}

void op_sub_lr() {
    emit_mnem2args(operation, 1, 0, QL_SS2S("R"<<dest_reg()));   // reverse operands to match Q1 instruction set
    // Negate result in 2's complement to correct for changed op order
    Str reg = QL_SS2S("R"<<dest_reg());
    emit("", "not", reg);                       // invert
    emit("", "nop");
    emit("", "add", "1,"+reg+","+reg);          // add 1
}


// bit arithmetic, 2 operands: "&&", "||", "^^"
void op_grp_bit_2op() {
    UInt mask = emit_bin_cast(fn->operands, 2);
    // FIXME: handle operation properly
    emit("", "and", QL_SS2S(REG_TMP0 << "," << mask << "," << REG_TMP1));    // results in '0' for 'bit==0' and 'mask' for 'bit==1'
    emit("", "nop");
    emit("", "jlt", QL_SS2S(REG_TMP1 << ",1,@" << label_if_false), "# " + ir::describe(expression));
    QL_ICE("CC backend does not yet support " << fn->function_type->name);
}

// relop, group 1: "==", "!="
void op_grp_rel1_rx() {
    emit_mnem2args("xor", 0, 1);
    emit("", "nop");    // register dependency
    emit("", operation, Str(REG_TMP0)+",1,@"+label_if_false, "# skip next part if condition is false");
}

void op_grp_rel1_lr() {
    emit_mnem2args("xor", 1, 0); break;   // reverse operands to match Q1 instruction set
    // FIXME: common with previous function
    emit("", "nop");    // register dependency
    emit("", operation, Str(REG_TMP0)+",1,@"+label_if_false, "# skip next part if condition is false");
}

// relop, group 2: ">=", "<"
void op_grp_rel2_rx() {
    emit_mnem2args(operation, 0, 1, as_target(label_if_false)); break;
}

void op_grp_rel2_lr() {
    emit_mnem2args(operation, 1, 0, as_target(label_if_false)); break;   // reverse operands (and instruction) to match Q1 instruction set
}

// relop, group 3: ">", "<="
void op_gt_rl() {
    check_int_literal(*fn->operands[1]->as_int_literal(), 0, 1);
    emit(
        "",
        operation,
        QL_SS2S(
            expr2q1Arg(fn->operands[0]) << ","
            << fn->operands[1]->as_int_literal()->value + 1    // increment literal since we lack 'jgt'
            << ",@"+label_if_false
        ),
        "# skip next part if condition is false"
    );
}

void op_gt_rr() {
    emit(
        "",
        "add",
        QL_SS2S(
            "1,"
            << expr2q1Arg(fn->operands[1])
            << "," << REG_TMP0
        )
    );                      // increment arg1
    emit("", "nop");        // register dependency
    emit(
        "",
        operation,
        QL_SS2S(
            expr2q1Arg(fn->operands[0])
            << "," << REG_TMP0
            << ",@"+label_if_false
        ),
        "# skip next part if condition is false"
    );
}

void op_gt_lr() {
    check_int_literal(*fn->operands[0]->as_int_literal(), 1, 0);
    emit(
        "",
        operation,
        QL_SS2S(
            expr2q1Arg(fn->operands[1])     // reverse operands
            << fn->operands[0]->as_int_literal()->value - 1    // DECrement literal since we lack 'jle'
            << ",@"+label_if_false
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

//name		    ret         func                profile operation
#define CC_FUNCTION_LIST \
X("operator~",              op_binv,            x,      "") \
\
/* bit arithmetic, 1 operand: "!" */ \
X("operator!",              op_linv,            b,      "") \
\
/* int arithmetic, 2 operands: "+", "-", "&", "|", "^" */ \
X("operator+",              op_grp_int_2op,     rx,     "add") \
X("operator+",              op_grp_int_2op,     lr,     "add") \
X("operator-",              op_grp_int_2op,     rx,     "sub") \
X("operator-",              op_grp_int_2op,     lr,     "sub") \
X("operator&",              op_grp_int_2op,     rx,     "and") \
X("operator&",              op_grp_int_2op,     lr,     "and") \
X("operator|",              op_grp_int_2op,     rx,     "or") \
X("operator|",              op_grp_int_2op,     lr,     "or") \
X("operator^",              op_grp_int_2op,     rx,     "xor") \
X("operator^",              op_grp_int_2op,     lr,     "xor") \
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
/* repeat, with operation inversed */ \
X("operator<",              op_grp_rel2,        rx,     "jlt") \
X("operator<",              op_grp_rel2,        lr,     "jge") \
\
/* relop, group 3: ">", "<=" */ \
X("operator>",              op_gt,              rl,     "jge") \
X("operator>",              op_gt,              rr,     "jge") \
X("operator>",              op_gt,              lr,     "jlt" /* inverse operation */) \
/* repeat, with operation inversed */ \
X("operator<="              op_gt,              rl,     "jlt") \
X("operator<="              op_gt,              rr,     "jlt") \
X("operator<="              op_gt,              lr,     "jge") \
\
/* user functions */ \
X("rnd_seed",               rnd_seed,           ,       "") \
X("rnd",                    rnd                 ,       "" )



Functions::Profile Functions::get_profile(Any<ir::Expression> operands) {
    CHECK_COMPAT(
        operands.size() == 2,
        "expected 2 operands"
    );
    if(operands[0]->as_int_literal() && operands[1]->as_reference()) {
        return LR;
    } else if(operands[0]->as_reference() && operands[1]->as_int_literal()) {
        return RL;
    } else if(operands[0]->as_reference() && operands[1]->as_reference()) {
        return RR;
    } else if(operands[0]->as_int_literal() && operands[1]->as_int_literal()) {
        QL_INPUT_ERROR("cannot currently handle functions on two literal parameters"); // FIXME: maybe handle in separate pass
    } else if(operands[0]->as_function_call()) {
        QL_INPUT_ERROR("cannot currently handle function call within function call '" << ir::describe(operands[0]) << "'");
    } else if(operands[1]->as_function_call()) {
        QL_INPUT_ERROR("cannot currently handle function call within function call '" << ir::describe(operands[1]) << "'");
    } else {
        QL_INPUT_ERROR("cannot currently handle parameter combination '" << ir::describe(operands[0]) << "' , '" << ir::describe(operands[1]) << "'");
    }
};


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


} // namespace detail
} // namespace vq1asm
} // namespace gen
} // namespace pass
} // namespace cc
} // namespace arch
} // namespace ql
