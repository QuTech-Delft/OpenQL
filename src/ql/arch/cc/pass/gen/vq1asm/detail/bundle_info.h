/** \file
 * Defines the BundleInfo class/structure.
 */

#pragma once

#include "types.h"
#include "options.h"
#include "settings.h"

namespace ql {
namespace arch {
namespace cc {
namespace pass {
namespace gen {
namespace vq1asm {
namespace detail {


/*
 * additional definitions for describing conditional gates
 * NB: copied from gate.h, reused for now, will need change if we want to support more complex expressions for conditional gates
 */
enum class ConditionType {
    // 0 operands:
    ALWAYS, NEVER,
    // 1 operand:
    UNARY, NOT,
    // 2 operands
    AND, NAND, OR, NOR, XOR, NXOR,
    // rnd_bit()
    RND_BIT
};


struct tInstructionCondition {
public:
    ConditionType cond_type;
    utils::Vec<utils::UInt> cond_operands;
    Str describe;   // the original condition, for logging purposes
};


// information for an instrument group (of channels), for a single instruction
// FIXME: rename tInstrInfo, move to class cc:IR, together with customGate(), bundleStart(), bundleFinish()?
class BundleInfo {
public: // funcs
    BundleInfo() = default;

public: // vars
    // output gates
    Str signalValue;
    UInt durationInCycles = 0;
#if OPT_SUPPORT_STATIC_CODEWORDS
    Int staticCodewordOverride = Settings::NO_STATIC_CODEWORD_OVERRIDE;
#endif

    // conditional gate info
    tInstructionCondition instructionCondition;

    // measurement
    Bool isMeasure = false;
    UInt measQubit = 0;

    // real-time measurement results: flag and operands
    Bool isMeasRsltRealTime = false;
    UInt bregTargetMeasRsltRealTime = 0;  // the breg where the result is to be stored

    // original instruction, for logging purposes
    Str describe;
};
} // namespace detail
} // namespace vq1asm
} // namespace gen
} // namespace pass
} // namespace cc
} // namespace arch
} // namespace ql
