#pragma once

/*
 * Define platform functions using the venerable X macro mechanism
 *
 * We only need to implement libqasm functions which are overridden by OpenQL, and - since libqasm supports
 * polymorphism - only for the parameter types overridden (i.e. bit and int).
 * Also see the comment in codegen.h::handle_set_instruction() on how platform functions come into existence.
 *
 * Return types (R_*):
 * - R_B: bit
 * - R_I: integer
 * Parameter types (P_*):
 * - P_B: bit literal
 * - P_I: integer literal
 *
 * Based on:
 * - deps/libqasm/src/cqasm/func-gen/funcgen.cpp
 * - <build>/deps/libqasm/src/cqasm/cqasm-v1-functions-gen.cpp
 * - src/ql/arch/cc/pass/gen/vq1asm/detail/functions.cc
 *
 * Note: to have better guarantees on consistency, we might also use this file in ir/old_to_new.cc to create the
 * platform functions, and extend it to be usable for ir/operator_info.cc
 */

//name		        ret-type    par0    par1    operation   func
#define PLATFORM_FUNCTION_LIST \
/* Basic scalar arithmetic operators. */ \
X2("operator+",     R_I,        P_I,    P_I,    a + b,      op_add) \
X2("operator-",     R_I,        P_I,    P_I,    a - b,      op_sub) \
X2("operator*",     R_I,        P_I,    P_I,    a * b,      op_mul) \
/* Relational operators. */              \
X2("operator==",    R_B,        P_I,    P_I,    a == b,     op_eq) \
X2("operator!=",    R_B,        P_I,    P_I,    a != b,     op_ne) \
X2("operator>=",    R_B,        P_I,    P_I,    a >= b,     op_ge) \
X2("operator>",     R_B,        P_I,    P_I,    a > b,      op_gt) \
X2("operator<=",    R_B,        P_I,    P_I,    a <= b,     op_le) \
X2("operator<",     R_B,        P_I,    P_I,    a < b,      op_lt) \
/* Bitwise operators. */                 \
X2("operator&",     R_I,        P_I,    P_I,    a & b,      op_band) \
X2("operator|",     R_I,        P_I,    P_I,    a | b,      op_bor) \
X2("operator^",     R_I,        P_I,    P_I,    a ^ b,      op_bxor) \
/* Logical operators. */                 \
X2("operator==",    R_B,        P_B,    P_B,    a == b,     op_eq) \
X2("operator!=",    R_B,        P_B,    P_B,    a != b,     op_ne) \
X2("operator&&",    R_B,        P_B,    P_B,    a && b,     op_land) \
X2("operator||",    R_B,        P_B,    P_B,    a || b,     op_lor) \
X2("operator^^",    R_B,        P_B,    P_B,    !a ^ !b,    op_lxor) \
/* Unary functions */\
X1("operator-",     R_I,        P_I,            -a,         op_neg) \
X1("operator~",     R_I,        P_I,            ~a,         op_binv) \
X1("operator!",     R_I,        P_B,            !a,         op_linv) \
X1("int",           R_I,        P_B,            (int)a,     op_int)
