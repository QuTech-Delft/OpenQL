/** \file
 * Defines the BundleInfo class/structure.
 */

#pragma once

#include "types_cc.h"
#include "options_cc.h"
#include "settings_cc.h"

namespace ql {
namespace arch {
namespace cc {

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
#if OPT_FEEDBACK
    // readout feedback
    Bool isMeasFeedback = false;
    Vec<UInt> operands;                         // NB: also used by OPT_PRAGMA
    Vec<UInt> creg_operands;
    Vec<UInt> breg_operands;

    // conditional gates
    cond_type_t condition = cond_always;        // FIXME
    Vec<UInt> cond_operands;
#endif
#if OPT_PRAGMA
    // pragma 'gates'
    RawPtr<const Json> pragma;
#endif
}; // information for an instrument group (of channels), for a single instruction
// FIXME: rename tInstrInfo, store gate as annotation, move to class cc:IR, together with customGate(), bundleStart(), bundleFinish()?

} // namespace cc
} // namespace arch
} // namespace ql
