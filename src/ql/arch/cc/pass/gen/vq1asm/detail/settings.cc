/**
 * @file    arch/cc/pass/gen/vq1asm/detail/settings.cc
 * @date    20201001
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   handle JSON settings for the CC backend
 * @note
 */

#include "ql/arch/cc/pass/gen/vq1asm/detail/settings.h"

namespace ql {
namespace arch {
namespace cc {
namespace pass {
namespace gen {
namespace vq1asm {
namespace detail {

using namespace utils;


static bool isMeasureSignal(const Json &signal, const Str &iname, bool realTime=false) {
    QL_JSON_ASSERT(signal, "type", iname);
    QL_JSON_ASSERT(signal, "value", iname);

    // check type
    if(signal["type"] != Settings::getInstrumentSignalTypeMeasure()) return false;

    // value must be non-empty on true measurements (and empty on instructions where hasMeasRsltSignalRealTime()==true)
    // value must be empty on instructions retrieving measurements in real-time
    if(signal["value"].empty() == realTime) return true;
    return false;
}

// return true for instructions retrieving measurement *results* in real-time (e.g. '_dist_dsm')
static bool isMeasRsltSignalRealTime(const Json &signal, const Str &iname) {
    return isMeasureSignal(signal, iname, true);
}

/************************************************************************\
| support for Info::preprocess_platform()
\************************************************************************/

void Settings::loadBackendSettings(const utils::Json &data) {
    QL_JSON_ASSERT(data, "hardware_settings", "/");
    const utils::Json &hardware_settings = data["hardware_settings"];
    QL_JSON_ASSERT(hardware_settings, "eqasm_backend_cc", "hardware_settings");
    const utils::Json &jsonBackendSettings = hardware_settings["eqasm_backend_cc"];
    doLoadBackendSettings(jsonBackendSettings);
}

Bool Settings::isMeasure(const Json &instruction, const Str &iname) {
    // key "cc" is optional, since we may be looking at a 'gate decomposition' instruction
    if (!QL_JSON_EXISTS(instruction, "cc")) return false;

    // return true if any "signal/type" matches
    SignalDef sd = findSignalDefinition(instruction, iname);
    for (UInt s = 0; s < sd.signal.size(); s++) {
        const Json &signal = sd.signal[s];
        if(isMeasureSignal(signal, iname)) return true;
    }
    return false;
}

Bool Settings::isFlux(const Json &instruction, const Str &iname) {
    // key "cc" is optional, since we may be looking at a 'gate decomposition' instruction
    if (!QL_JSON_EXISTS(instruction, "cc")) return false;

    // return true if any "signal/type" matches
    SignalDef sd = findSignalDefinition(instruction, iname);
    for (UInt s = 0; s < sd.signal.size(); s++) {
        const Json &signal = sd.signal[s];
        if (!QL_JSON_EXISTS(signal, "type")) {
            QL_WOUT("no type detected for '" << iname << "', signal=" << signal);
        } else {
            QL_DOUT("type detected for '" << iname << "': " << signal["type"]);
            if(signal["type"] == getInstrumentSignalTypeFlux()) return true;
        }
    }
    return false;
}

/************************************************************************\
| support for Info::postprocess_platform()
\************************************************************************/

void Settings::loadBackendSettings(const ir::compat::PlatformRef &platform) {
    QL_JSON_ASSERT(platform->hardware_settings, "eqasm_backend_cc", "hardware_settings");  // NB: json_get<const json &> unavailable
    const Json &jsonBackendSettings = platform->hardware_settings["eqasm_backend_cc"];
    doLoadBackendSettings(jsonBackendSettings);
}

/************************************************************************\
| support for Backend::Backend() (Codegen::init)
\************************************************************************/

void Settings::loadBackendSettings(const ir::PlatformRef &platform) {
    const Json &hardwareSettings = platform->data.data["hardware_settings"];
    QL_JSON_ASSERT(hardwareSettings, "eqasm_backend_cc", "hardware_settings");  // NB: json_get<const json &> unavailable
    const Json &jsonBackendSettings = hardwareSettings["eqasm_backend_cc"];
    doLoadBackendSettings(jsonBackendSettings);
}


Bool Settings::isMeasRsltRealTime(const Json &instruction, const Str &iname) {
    // key "cc" is optional, since we may be looking at a 'gate decomposition' instruction
    if (!QL_JSON_EXISTS(instruction, "cc")) return false;

    // return true if any "signal/type" matches (note that a qualifying instruction will only have a single signal in practice)
    SignalDef sd = findSignalDefinition(instruction, iname);
    for (UInt s = 0; s < sd.signal.size(); s++) {
        const Json &signal = sd.signal[s];
        if(isMeasRsltSignalRealTime(signal, iname)) return true;
    }
    return false;
};

Bool Settings::isMeasRsltRealTime(const ir::InstructionType &instrType) {
    return isMeasRsltRealTime(instrType.data.data, instrType.name);
}


// FIXME: add Settings::SignalDef Settings::findSignalDefinition(const ir::InstructionType &instrType) const
// FIXME: deprecate "ref_signal"?
Settings::SignalDef Settings::findSignalDefinition(const Json &instruction, const Str &iname) const {
    SignalDef ret;

    Str instructionPath = "instructions/" + iname;
    if(!QL_JSON_EXISTS(instruction, "cc")) {
        ret.signal = {};
        ret.path = instructionPath;
    } else {
        // FIXME: deprecate ref_signal? Not useful once we have fully switched to new semantics for signal contents. Wait until new configuration has percolated to the lab
        if (QL_JSON_EXISTS(instruction["cc"], "ref_signal")) {                      // optional syntax: "ref_signal"
            Str refSignal = instruction["cc"]["ref_signal"].get<Str>();
            ret.signal = (*jsonSignals)[refSignal];                                 // poor man's JSON pointer
            if(ret.signal.empty()) {
                QL_JSON_ERROR(
                    "instruction '" << iname
                    << "': ref_signal '" << refSignal
                    << "' does not resolve"
                );
            }
            ret.path = "signals/" + refSignal;
        } else {                                                                    // alternative syntax: "signal"
            ret.signal = json_get<Json>(instruction["cc"], "signal", instructionPath + "/cc");
            QL_DOUT("signal for '" << instruction << "': '" << ret.signal << "'");
            ret.path = instructionPath + "/cc/signal";
        }
    }
    return ret;
}


Settings::CalcSignalValue Settings::calcSignalValue(
    const Settings::SignalDef &sd,
    UInt s,
    const Vec<UInt> &qubits,
    const Str &iname
) {
    CalcSignalValue ret;
    Str signalSPath = QL_SS2S(sd.path<<"["<<s<<"]");                   // for JSON error reporting

    /************************************************************************\
    | decode sd.signal[s], and map operand index to qubit
    \************************************************************************/

    // get the operand (i.e. qubit) index & qubit to work on
    // NB: the key name "operand_idx" is a historical artifact: formerly all operands were qubits
    // FIXME: replace array sd containing operand_idx with array (dimension: # operands) of arrays (dimension: # signals for operands, mostly 1, more for special cases like flux during measurement and phase corrections during CZ)
    UInt operandIdx = json_get<UInt>(sd.signal[s], "operand_idx", signalSPath);
    if (operandIdx >= qubits.size()) {
        QL_JSON_ERROR(
            "instruction '" << iname
            << "': JSON file defines operand_idx " << operandIdx
            << ", but only " << qubits.size()
            << " qubit operands were provided (correct JSON, or provide enough operands)"
        );
    }
    UInt qubit = qubits[operandIdx];

    // get signal value
    // FIXME: note that the actual contents of the signalValue only become important when we'll do automatic codeword assignment and provide codewordTable to downstream software to assign waveforms to the codewords
    const Json instructionSignalValue = json_get<const Json>(sd.signal[s], "value", signalSPath);   // NB: json_get<const Json&> unavailable
    // FIXME: also allow key "value" to be absent
    if (instructionSignalValue.empty()) {    // allow empty signal
        ret.signalValueString = "";
    } else {
        Str sv = QL_SS2S(instructionSignalValue);   // serialize/stream instructionSignalValue into std::string
        sv = replace_all(sv, "\"", "");   // get rid of quotes
#if 0   // FIXME: no longer useful? Maybe to disambiguate different signal_ref
        // expand macros
        sv = replace_all(sv, "{gateName}", iname);
        sv = replace_all(sv, "{instrumentName}", FIXMEret.si.ic.ii.instrumentName);
        sv = replace_all(sv, "{instrumentGroup}", to_string(ret.si.group));
        sv = replace_all(sv, "{qubit}", to_string(qubit));
#endif
        ret.signalValueString = sv;
    }

    // is this a measurement?
    ret.isMeasure = isMeasureSignal(sd.signal[s], iname);

    // get instruction signal type (e.g. "mw", "flux", etc).
    // NB: instructionSignalType is different from "instruction/type" provided by find_instruction_type, although some
    // identical strings are used). NB: that key is no longer used by the 'core' of OpenQL
    Str instructionSignalType = json_get<Str>(sd.signal[s], "type", signalSPath);

    /************************************************************************\
    | map signal type for qubit to instrument & group
    \************************************************************************/

    // find signalInfo, i.e. perform the mapping of abstract signals to instruments
    ret.si = findSignalInfoForQubit(instructionSignalType, qubit);

#if OPT_SUPPORT_STATIC_CODEWORDS
    ret.operandIdx = operandIdx;
#endif
    return ret;
}


Settings::InstrumentInfo Settings::getInstrumentInfo(UInt instrIdx) const {
    InstrumentInfo ret = {nullptr};

    Str instrumentPath = QL_SS2S("instruments[" << instrIdx << "]");    // for JSON error reporting
    if (instrIdx >= jsonInstruments->size()) {
        QL_JSON_ERROR("node not defined: " + instrumentPath);                   // probably an internal backend error
    }
    ret.instrument = &(*jsonInstruments)[instrIdx];

    ret.instrumentName = json_get<Str>(*ret.instrument, "name", instrumentPath);

    QL_JSON_ASSERT(*ret.instrument, "controller", ret.instrumentName);          // first check intermediate node
    // FIXME: check controller/"name" being "cc"?
    ret.slot = json_get<Int>((*ret.instrument)["controller"], "slot", ret.instrumentName+"/controller");
    // FIXME: also return controller/"io_module"?

    // optional key 'instruments[]/force_cond_gates_on', can be used to always enable AWG if gate execution is controlled by VSM
    if (QL_JSON_EXISTS(*ret.instrument, "force_cond_gates_on")) {
        ret.forceCondGatesOn = json_get<Bool>(*ret.instrument, "force_cond_gates_on", ret.instrumentName+"/force_cond_gates_on"); // key will exist, but type may be wrong
    }

    return ret;
}


Settings::InstrumentControl Settings::getInstrumentControl(UInt instrIdx) const {
    InstrumentControl ret;

    ret.ii = getInstrumentInfo(instrIdx);

    // get control mode reference for for instrument
    ret.refControlMode = json_get<Str>(*ret.ii.instrument, "ref_control_mode", ret.ii.instrumentName);

    // get control mode definition for our instrument
    ret.controlMode = json_get<Json>(*jsonControlModes, ret.refControlMode, "control_modes");

    // how many groups of control bits does the control mode specify (NB: 0 on missing key)
    ret.controlModeGroupCnt = ret.controlMode["control_bits"].size();



    // get instrument definition reference for for instrument
    Str refInstrumentDefinition = json_get<Str>(*ret.ii.instrument, "ref_instrument_definition", ret.ii.instrumentName);
    // get instrument definition for our instrument
    const Json instrumentDefinition = json_get<const Json>(*jsonInstrumentDefinitions, refInstrumentDefinition, "instrument_definitions");

    // get number of channels of instrument
    UInt channels = json_get<UInt>(instrumentDefinition, "channels", refInstrumentDefinition);
    // calculate groups size (#channels) of control mode
    ret.controlModeGroupSize = channels / ret.controlModeGroupCnt;  // FIXME: handle 0 div, and rounding. FIXME: no longer really used

    // verify that group size is allowed
    const Json controlGroupSizes = json_get<const Json>(instrumentDefinition, "control_group_sizes", refInstrumentDefinition);
    // FIXME: find channels

    // FIXME: unfinished: find channels


    return ret;
}


// FIXME: assumes that group configuration for readout input matches that of output
Int Settings::getResultBit(const InstrumentControl &ic, Int group) {
    // FIXME: test similar to settings_cc::getInstrumentControl, move
    // check existence of key 'result_bits'
    if (!QL_JSON_EXISTS(ic.controlMode, "result_bits")) {        // this instrument mode produces results (i.e. it is a measurement device)
        QL_JSON_ERROR("readout requested on instrument '" << ic.ii.instrumentName << "', but key '" << ic.refControlMode << "/result_bits is not present");
    }

    // check existence of key 'result_bits[group]'
    const Json &groupResultBits = ic.controlMode["result_bits"][group];
    UInt nrGroupResultBits = groupResultBits.size();
    if (nrGroupResultBits != 1) {                             // single bit (NB: per group)
        QL_JSON_ERROR("key '" << ic.refControlMode << "/result_bits[" << group << "] must have 1 bit instead of " << nrGroupResultBits);
    }
    return groupResultBits[0].get<Int>();   // bit on digital interface. NB: we assume the result is active high, which is correct for UHF-QC
}


Settings::SignalInfo Settings::findSignalInfoForQubit(const Str &instructionSignalType, UInt qubit) const {
    SignalInfo ret;
    Bool signalTypeFound = false;
    Bool qubitFound = false;

    // iterate over instruments
    for (UInt instrIdx = 0; instrIdx < jsonInstruments->size() && !qubitFound; instrIdx++) {
        InstrumentControl ic = getInstrumentControl(instrIdx);
        Str instrumentSignalType = json_get<Str>(*ic.ii.instrument, "signal_type", ic.ii.instrumentName);
        if (instrumentSignalType == instructionSignalType) {
            signalTypeFound = true;
            const Json qubits = json_get<const Json>(*ic.ii.instrument, "qubits", ic.ii.instrumentName);   // NB: json_get<const json&> unavailable

            // verify group size: qubits vs. control mode
            UInt qubitGroupCnt = qubits.size();                                  // NB: JSON key qubits is a 'matrix' of [groups*qubits]
            if (qubitGroupCnt != ic.controlModeGroupCnt) {
                QL_JSON_ERROR(
                    "instrument " << ic.ii.instrumentName
                    << ": number of qubit groups " << qubitGroupCnt
                    << " does not match number of control_bits groups " << ic.controlModeGroupCnt
                    << " of selected control mode '" << ic.refControlMode << "'"
                );
            }

            // anyone connected to qubit?
            for (UInt group = 0; group < qubitGroupCnt && !qubitFound; group++) {
                for (UInt idx = 0; idx < qubits[group].size() && !qubitFound; idx++) {
                    if (qubits[group][idx].get<UInt>() == qubit) {
                        qubitFound = true;                                      // also: stop searching

                        QL_DOUT(
                            "qubit " << qubit
                            << " signal type '" << instructionSignalType
                            << "' driven by instrument '" << ic.ii.instrumentName
                            << "' group " << group
                        );

                        ret.ic = ic;
                        ret.instrIdx = instrIdx;
                        ret.group = group;
                    }
                }
            }
        }
    }
    if (!signalTypeFound) {
        QL_JSON_ERROR("No instruments found providing signal type '" << instructionSignalType << "'");
    }
    if (!qubitFound) {
        QL_JSON_ERROR("No instruments found driving qubit " << qubit << " for signal type '" << instructionSignalType << "'");
    }

    return ret;
}

/************************************************************************\
| Static functions processing JSON
\************************************************************************/

//#if OPT_SUPPORT_STATIC_CODEWORDS
Int Settings::findStaticCodewordOverride(const Json &instruction, UInt operandIdx, const Str &iname)
{
    // look for optional codeword override
    Int staticCodewordOverride = NO_STATIC_CODEWORD_OVERRIDE;               // -1 means unused
    if (QL_JSON_EXISTS(instruction["cc"], "static_codeword_override")) {    // optional keyword
 #if OPT_STATIC_CODEWORDS_ARRAYS
        const Json &override = instruction["cc"]["static_codeword_override"];
        if (override.is_array()) {
            if (operandIdx < override.size()) {
                staticCodewordOverride = override[operandIdx];
            } else {
                QL_JSON_ERROR("Array size of static_codeword_override for instruction '" << iname << "' insufficient");
            }
        } else if (operandIdx == 0) {     // NB: JSON '"static_codeword_override": [3]' gives **scalar** result
            staticCodewordOverride = override.get<Int>();
        } else {
            QL_JSON_ERROR("Key static_codeword_override for instruction '" << iname
                          << "' should be an array (found '" << override << "' in '" << instruction << "')");
        }
 #else
        staticCodewordOverride = instruction["cc"]["static_codeword_override"];
 #endif
        QL_DOUT(
            "Found static_codeword_override=" << staticCodewordOverride
            << " for instruction '" << iname
            << "', operand index " << operandIdx
        );
    }
 #if 1 // FIXME: require override
    if (staticCodewordOverride < 0) {
        QL_JSON_ERROR(
            "No static codeword defined for instruction '" << iname
            << "' (we currently require it because automatic assignment is disabled)"
        );
    }
 #endif
    return staticCodewordOverride;
}
//#endif

/************************************************************************\
| Private
\************************************************************************/

void Settings::doLoadBackendSettings(const Json &jsonBackendSettings) {
    // remind some main JSON areas
    QL_JSON_ASSERT(jsonBackendSettings, "instrument_definitions", "eqasm_backend_cc");
    jsonInstrumentDefinitions = &jsonBackendSettings["instrument_definitions"];

    QL_JSON_ASSERT(jsonBackendSettings, "control_modes", "eqasm_backend_cc");
    jsonControlModes = &jsonBackendSettings["control_modes"];

    QL_JSON_ASSERT(jsonBackendSettings, "instruments", "eqasm_backend_cc");
    jsonInstruments = &jsonBackendSettings["instruments"];

    QL_JSON_ASSERT(jsonBackendSettings, "signals", "eqasm_backend_cc");
    jsonSignals = &jsonBackendSettings["signals"];

#if 0   // FIXME: print some info, which also helps detecting errors early on
    // read instrument definitions
    // FIXME: the following requires json>v3.1.0: (NB: we now moved to 3.9!) for(auto& id : jsonInstrumentDefinitions->items()) {
    for (UInt i = 0; i < jsonInstrumentDefinitions->size(); i++) {
        Str idName = (*jsonInstrumentDefinitions)[i].get<Str>();
        QL_DOUT("found instrument definition: '" << idName <<"'");
    }

    // read control modes
    for (UInt i = 0; i < jsonControlModes->size(); i++) {
        const Json &name = (*jsonControlModes)[i]["name"];
        QL_DOUT("found control mode '" << name <<"'");
    }

    // read instruments
    const Json &ccSetupType = jsonCcSetup["type"];

    // CC specific
    const Json &ccSetupSlots = jsonCcSetup["slots"];                      // FIXME: check against jsonInstrumentDefinitions
    for (UInt slot = 0; slot < ccSetupSlots.size(); slot++) {
        const Json &instrument = ccSetupSlots[slot]["instrument"];
        Str instrumentName = instrument["name"].get<Str>();
        Str signalType = instrument["signal_type"].get<Str>();

        QL_DOUT("found instrument: name='" << instrumentName << "', signal type='" << signalType << "'");
    }
#endif
}

} // namespace detail
} // namespace vq1asm
} // namespace gen
} // namespace pass
} // namespace cc
} // namespace arch
} // namespace ql
