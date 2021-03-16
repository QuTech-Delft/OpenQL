/**
 * @file    arch/cc/settings_cc.h
 * @date    20201001
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   handle JSON settings for the CC backend
 * @note
 */

#pragma once

#include "types_cc.h"
#include "options_cc.h"
#include "platform.h"

namespace ql {
namespace arch {
namespace cc {

class Settings {
public: // types
    struct SignalDef {
        Json signal;        // a copy of the signal node found
        Str path;           // path of the node, for reporting purposes
    };

    struct InstrumentInfo {         // information from key 'instruments'
        RawPtr<const Json> instrument;
        Str instrumentName;         // key 'instruments[]/name'
        Int slot;                  // key 'instruments[]/controller/slot'
#if OPT_FEEDBACK
        Bool forceCondGatesOn;      // optional key 'instruments[]/force_cond_gates_on', can be used to always enable AWG if gate execution is controlled by VSM
#endif
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

    void loadBackendSettings(const quantum_platform &platform);
    Str getReadoutMode(const Str &iname);
    Bool isReadout(const Str &iname);
    Bool isPragma(const Str &iname);
    RawPtr<const Json> getPragma(const Str &iname);
    UInt getReadoutWait();
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

private:    // vars
    RawPtr<const quantum_platform> platform;
    RawPtr<const Json> jsonInstrumentDefinitions;
    RawPtr<const Json> jsonControlModes;
    RawPtr<const Json> jsonInstruments;
    RawPtr<const Json> jsonSignals;
}; // class

} // namespace cc
} // namespace arch
} // namespace ql
