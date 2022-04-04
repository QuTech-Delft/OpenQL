/** \file
 * Constant propagation pass.
 */

#include "ql/pass/opt/const_prop/const_prop.h"

#include "ql/utils/tree.h"
//#include "ql/ir/ops.h"
#include "ql/pmgr/pass_types/base.h"

namespace ql {
namespace pass {
namespace opt {
namespace const_prop {

/**
 * Constructs an constant propagator.
 */
ConstantPropagationPass::ConstantPropagationPass(
    const utils::Ptr<const pmgr::Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::Transformation(pass_factory, instance_name, type_name) {
}

/**
 * Returns a user-friendly type name for this pass.
 */
utils::Str ConstantPropagationPass::get_friendly_type() const {
    return "Constant propagator";
}

/**
 * Runs the constant propagator.
 */
utils::Int ConstantPropagationPass::run(
    const ir::Ref &ir,
    const pmgr::pass_types::Context &context
) const {
    // register functions

    // perform constant propagation
    if (!ir->program.empty()) {
        for (const auto &block : ir->program->blocks) {
            run_on_block(ir, block);
        }
    }
    return 0;
}

/**
 * Dumps docs for constant propagator.
 */
void ConstantPropagationPass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix, R"(
    This pass replaces constant expressions by their result.
    )");
}

/**
 * Runs the constant propagator on the given block.
 */
void ConstantPropagationPass::run_on_block(
    const ir::Ref &ir,
    const ir::BlockBaseRef &block
) {
    for (const auto &statement : block->statements) {
        auto insn = statement.as<ir::Instruction>();
        if (!insn.empty()) {
//            ir::generalize_instruction(insn);
        }
        if (auto if_else = statement->as_if_else()) {
            for (const auto &branch : if_else->branches) {
                run_on_block(ir, branch->body);
            }
            if (!if_else->otherwise.empty()) {
                run_on_block(ir, if_else->otherwise);
            }
        } else if (auto loop = statement->as_loop()) {
            run_on_block(ir, loop->body);
        }
    }
}


// operator functions
// Based on:
// - deps/libqasm/src/cqasm/func-gen/funcgen.cpp
// - <build>/deps/libqasm/src/cqasm/cqasm-v1-functions-gen.cpp
// - src/ql/arch/cc/pass/gen/vq1asm/detail/functions.cc
// libqasm's cQASM resolver
// FIXME: explain which functions need to be implemented (those overridden by OpenQL), and which types (bit and int)
// also see the comment in codegen.h::handle_set_instruction()

// Return types (R_*):
// - R_B: bit
// - R_I: integer
// Parameter types (P_*):
// - P_B: bit literal
// - P_I: integer literal

//name		        ret-type    par0    par1    operation   func
#define FUNCTION_LIST \
/* Basic scalar arithmetic operators. */ \
X("operator+",      R_I,        P_I,    P_I,    a + b,    op_add) \
X("operator-",      R_I,        P_I,    P_I,    a - b,    op_sub) \
X("operator*",      R_I,        P_I,    P_I,    a * b,    op_mul) \
/* Relational operators. */              \
X("operator==",     R_B,        P_I,    P_I,    a == b,   op_eq) \
X("operator!=",     R_B,        P_I,    P_I,    a != b,   op_ne) \
X("operator>=",     R_B,        P_I,    P_I,    a >= b,   op_ge) \
X("operator>",      R_B,        P_I,    P_I,    a > b,    op_gt) \
X("operator<=",     R_B,        P_I,    P_I,    a <= b,   op_le) \
X("operator<",      R_B,        P_I,    P_I,    a < b,    op_lt) \
/* Bitwise operators. */                 \
X("operator&",      R_I,        P_I,    P_I,    a & b,    op_band) \
X("operator|",      R_I,        P_I,    P_I,    a | b,    op_bor) \
X("operator^",      R_I,        P_I,    P_I,    a ^ b,    op_bxor) \
/* Logical operators. */                 \
X("operator==",     R_B,        P_B,    P_B,    a == b,   op_eq) \
X("operator!=",     R_B,        P_B,    P_B,    a != b,   op_ne) \
X("operator&&",     R_B,        P_B,    P_B,    a && b,   op_land) \
X("operator||",     R_B,        P_B,    P_B,    a || b,   op_lor) \
X("operator^^",     R_B,        P_B,    P_B,    !a ^ !b,  op_lxor)


// FIXME: unary: - ! ~ (int)

#define xstr(s) str(s)
#define str(s) #s

// create the functions
#define P_I as_int_literal()
#define P_B as_bit_literal()
//#define P_R as_real_literal()
//#define P_C as_complex_literal()
#define R_I(x) utils::make<ir::IntLiteral>(x)   // FIXME: use make_int_lit ?
#define R_B(x) utils::make<ir::BitLiteral>(x)
//#define R_R(x) utils::make<ir::RealLiteral>(x)
//#define R_C(x) utils::make<ir::ComplexLiteral>(x)
//#define FUNC_NAME func ## _ ## par0 ## par1

#define X(name, ret_type, par0, par1, operation, func)  \
static FncRet func ## _ ## par0 ## par1(FncArgs &v) {   \
    auto a = v[0]->par0->value;                         \
    auto b = v[1]->par1->value;                         \
    return ret_type(operation);                         \
}

FUNCTION_LIST

#undef X
//#undef R_C
//#undef R_R
#undef R_B
#undef R_I
//#undef P_C
//#undef P_R
#undef P_B
#undef P_I


void ConstantPropagationPass::register_functions() {
    // Initialize func_map_int with the functions from FUNCTION_LIST
    // The key consists of the concatenation of name, "_" and the stringified profile, e.g "operator~_i"

    #define P_I i
    #define P_R r
    #define P_C c
    #define X(name, ret_type, par0, par1, operation, func) { name "_" xstr(par0) xstr(par1), &(func ## _ ## par0 ## par1) },
    func_map = {
        FUNCTION_LIST
    };
    #undef X
    #undef P_C
    #undef P_R
    #undef P_I
}
#undef xstr
#undef str




} // namespace const_prop
} // namespace opt
} // namespace pass
} // namespace ql
