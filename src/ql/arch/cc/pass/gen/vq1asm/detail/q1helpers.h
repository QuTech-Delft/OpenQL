#pragma once

#include "ql/ir/ir.h"

#include "types.h"
#include "operands.h"
#include "datapath.h"

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

static inline Str as_target(const Str &label) { return "@" + label; }

// new
static inline Str as_reg(UInt creg) {
    if(creg >= NUM_CREGS) {
        QL_INPUT_ERROR("register index " << creg << " exceeds maximum of " << NUM_CREGS-1);
    }
    return QL_SS2S("R"<<creg);
}

static inline Str as_int(Int val, Int add=0) {
    if(val+add < 0) {
        // FIXME: improve message, show expression
        QL_INPUT_ERROR("CC backend cannot handle negative integer literals: value=" << val << ", add=" << add);
    }
    if(val >= (1LL<<32) - 1 - add) {
        QL_INPUT_ERROR("CC backend requires integer literals to fit 32 bits: value=" << val << ", add=" << add);
    }
    return QL_SS2S(val+add);
}

// FIXME: add as_bit() with checks?



} // namespace detail
} // namespace vq1asm
} // namespace gen
} // namespace pass
} // namespace cc
} // namespace arch
} // namespace ql
