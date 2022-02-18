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

    // information from key 'instruments'
    struct InstrumentInfo {
        RawPtr<const Json> instrument;
        Str instrumentName;         // key 'instruments[]/name'
        Int slot;                   // key 'instruments[]/controller/slot'
        Bool forceCondGatesOn;      // optional key 'instruments[]/force_cond_gates_on', can be used to always enable AWG if gate execution is controlled by VSM
    };

    // information from key 'instruments/ref_control_mode'
    struct InstrumentControl {
        Str refControlMode;         // the name of the control mode
        Json controlMode;           // the data of the control mode FIXME: use pointer
        UInt controlModeGroupCnt;   // number of groups in key 'control_bits' of effective control mode
        UInt controlModeGroupSize;  // the size (#channels) of the effective control mode group
        InstrumentInfo ii;
    };

    struct SignalInfo {
        UInt instrIdx;              // the index into JSON "eqasm_backend_cc/instruments" that provides the signal
        Int group;                  // the group of channels within the instrument that provides the signal
        InstrumentControl ic;
    };

    // return type for calcSignalValue()
    struct CalcSignalValue {
        Str signalValueString;
        Bool isMeasure;
        UInt operandIdx;            // NB: in the new IR, 'operand' is called 'qubit' in most places. FIXME: required for findStaticCodewordOverride()
        SignalInfo si;
    };

    static const Int NO_STATIC_CODEWORD_OVERRIDE = -1;

public: // functions
    Settings() = default;
    ~Settings() = default;

    /************************************************************************\
    | support for Info::preprocess_platform(), when we only have raw JSON
    | data available
    \************************************************************************/

    /*
     * Load backend settings from raw JSON data.
     */
    void loadBackendSettings(const utils::Json &data);

    /*
     * Determine whether the instruction record refers to a 'measure instruction', i.e. whether it produces any signal
     * with "type" matching "measure" AND isMeasRsltRealTime() is false
     * Note that both isMeasure() and isFlux() may be true on the same instruction.
     * Used as guidance for resource constrained scheduling.
     */
    Bool isMeasure(const Json &instruction, const Str &iname);

    /*
     * Determine whether the instruction record refers to a 'flux instruction', i.e. whether it produces any signal with
     * "type" matching "flux".
     * Note that both isMeasure() and isFlux() may be true on the same instruction.
     * Used as guidance for resource constrained scheduling.
     */
    Bool isFlux(const Json &instruction, const Str &iname);

    /************************************************************************\
    | support for Info::postprocess_platform(), when we only have an old
    | (ir::compat::PlatformRef) platform available
    \************************************************************************/

    /*
     * Load backend settings from old-style platform.
     */
    void loadBackendSettings(const ir::compat::PlatformRef &platform);

    // FIXME: this adds semantics to "signal_type", whereas the names are otherwise fully up to the user: optionally get from JSON
    static Str getInstrumentSignalTypeMeasure() { return "measure"; }
    static Str getInstrumentSignalTypeFlux() { return "flux"; }

    /************************************************************************\
    | support for Backend::Backend() (Codegen::init)
    \************************************************************************/

    /*
     * Load backend settings from new-style platform.
     */
    void loadBackendSettings(const ir::PlatformRef &platform);

    /**
     * Does this instruction process real time measurement results:
     * - false for an instruction that initiates the measurement, e.g. "measure"
     * - true for an instruction that acquires the bits resulting from the measurement, e.g. "_dist_dsm"
     */
    Bool isMeasRsltRealTime(const Json &instruction, const Str &iname);
    Bool isMeasRsltRealTime(const ir::InstructionType &instrType);

    /**
     * Find JSON signal definition for instruction, either inline or via 'ref_signal'
     */
    SignalDef findSignalDefinition(const Json &instruction, const Str &iname) const;

    /*
     * Compute signalValueString, and some meta information, for sd[s] (i.e. one of the signals in the JSON definition
     * of an instruction)
     * NB: helper for codegen::custom_instruction, which is called with try/catch to add error context
     */
    CalcSignalValue calcSignalValue(const Settings::SignalDef &sd, UInt s, const Vec<UInt> &qubits, const Str &iname);

    /*
     * Collect some configuration info for an instrument.
     */
    InstrumentInfo getInstrumentInfo(UInt instrIdx) const;

    InstrumentControl getInstrumentControl(UInt instrIdx) const;
    static Int getResultBit(const InstrumentControl &ic, Int group) ;

    /*
     * Find instrument&group given instructionSignalType for qubit.
     *
     * NB: we map signal *vectors* to groups, i.e. it is not possible to map individual channels.
     *
     * Conceptually, this is where we map an abstract signal definition, eg: {"flux", q3} (which may also be interpreted
     * as port "q3.flux") onto an instrument & group
     */
    SignalInfo findSignalInfoForQubit(const Str &instructionSignalType, UInt qubit) const;

    static Int findStaticCodewordOverride(const Json &instruction, UInt operandIdx, const Str &iname);

    // 'getters'
    const Json &getInstrumentAtIdx(UInt instrIdx) const { return (*jsonInstruments)[instrIdx]; }
    UInt getInstrumentsSize() const { return jsonInstruments->size(); }

private:    // functions
    void doLoadBackendSettings(const Json &jsonBackendSettings);

private:    // vars
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
