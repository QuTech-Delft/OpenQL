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
 * NB: copied from gate.h, reused for now, will change if we want to support more complex expressions for conditional gates
 */
enum class ConditionType {
    // 0 operands:
    ALWAYS, NEVER,
    // 1 operand:
    UNARY, NOT,
    // 2 operands
    AND, NAND, OR, NOR, XOR, NXOR
};


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

    // operands
    // FIXME: use new class Operands
    Vec<UInt> operands;
    Vec<UInt> creg_operands;
    Vec<UInt> breg_operands;

    // conditional gate operands
    ConditionType condition = ConditionType::ALWAYS;        // FIXME
    Vec<UInt> cond_operands;

    // readout feedback
    Bool isMeasFeedback = false;
}; // information for an instrument group (of channels), for a single instruction
// FIXME: rename tInstrInfo, store gate as annotation, move to class cc:IR, together with customGate(), bundleStart(), bundleFinish()?

} // namespace detail
} // namespace vq1asm
} // namespace gen
} // namespace pass
} // namespace cc
} // namespace arch
} // namespace ql
