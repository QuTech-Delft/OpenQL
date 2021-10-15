/**
 * @file    arch/cc/pass/gen/vq1asm/detail/settings.h
 * @date    20201001
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   handle JSON settings for the CC backend
 * @note
 */

#pragma once

#include "types.h"
#include "options.h"

#include "ql/ir/compat/platform.h"  // FIXME
#include "ql/ir/ir.h"

namespace ql {
namespace arch {
namespace cc {
namespace pass {
namespace gen {
namespace vq1asm {
namespace detail {

class Settings {
public: // types
    struct SignalDef {
        Json signal;                // a copy of the signal node found
        Str path;                   // path of the node, for reporting purposes
    };

    struct InstrumentInfo {         // information from key 'instruments'
        RawPtr<const Json> instrument;
        Str instrumentName;         // key 'instruments[]/name'
        Int slot;                   // key 'instruments[]/controller/slot'
        Bool forceCondGatesOn;      // optional key 'instruments[]/force_cond_gates_on', can be used to always enable AWG if gate execution is controlled by VSM
    };

    struct InstrumentControl {      // information from key 'instruments/ref_control_mode'
        InstrumentInfo ii;
        Str refControlMode;
        Json controlMode;           // FIXME: pointer
        UInt controlModeGroupCnt;   // number of groups in key 'control_bits' of effective control mode
        UInt controlModeGroupSize;  // the size (#channels) of the effective control mode group
    };

    struct SignalInfo {
        InstrumentControl ic;
        UInt instrIdx;              // the index into JSON "eqasm_backend_cc/instruments" that provides the signal
        Int group;                  // the group of channels within the instrument that provides the signal
    };

    static const Int NO_STATIC_CODEWORD_OVERRIDE = -1;

public: // functions
    Settings() = default;
    ~Settings() = default;

    // support for Info::preprocess_platform
    void loadBackendSettings(const ir::compat::PlatformRef &platform);
    static Bool isReadout(const Json &instruction, const Str &iname);   // FIXME: should be true for "measure" (because it uses a UHF)
    static Bool isFlux(const Json &instruction, RawPtr<const Json> signals, const Str &iname);

    // support for backend
    void loadBackendSettings(const ir::PlatformRef &platform);
    Str getReadoutMode(const ir::InstructionType &instrType);
    Bool isReadout(const ir::InstructionType &instrType);   // FIXME: naming
    Bool isMeasRsltRealTime(const ir::InstructionType &instrType) { return isReadout(instrType) && getReadoutMode(instrType) == "feedback"; };
    Bool isFlux(const ir::InstructionType &instrType);

    static SignalDef findSignalDefinition(const Json &instruction, RawPtr<const Json> signals, const Str &iname);
    SignalDef findSignalDefinition(const Json &instruction, const Str &iname) const;
    InstrumentInfo getInstrumentInfo(UInt instrIdx) const;
    InstrumentControl getInstrumentControl(UInt instrIdx) const;
    static Int getResultBit(const InstrumentControl &ic, Int group) ;

    // find instrument/group providing instructionSignalType for qubit
    SignalInfo findSignalInfoForQubit(const Str &instructionSignalType, UInt qubit) const;

    static Int findStaticCodewordOverride(const Json &instruction, UInt operandIdx, const Str &iname);

    // 'getters'
    const Json &getInstrumentAtIdx(UInt instrIdx) const { return (*jsonInstruments)[instrIdx]; }
    UInt getInstrumentsSize() const { return jsonInstruments->size(); }

private:    // functions
    void doLoadBackendSettings(const Json &jsonBackendSettings);

private:    // vars
//    ir::compat::PlatformRef platform;
    RawPtr<const Json> jsonInstrumentDefinitions;
    RawPtr<const Json> jsonControlModes;
    RawPtr<const Json> jsonInstruments;
    RawPtr<const Json> jsonSignals;
}; // class

} // namespace detail
} // namespace vq1asm
} // namespace gen
} // namespace pass
} // namespace cc
} // namespace arch
} // namespace ql
