/**
 * @file    arch/cc/settings_cc.cc
 * @date    20201001
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   handle JSON settings for the CC backend
 * @note
 */

#include "settings_cc.h"

namespace ql {
namespace arch {
namespace cc {

using namespace utils;

void Settings::loadBackendSettings(const quantum_platform &platform) {
    this->platform = &platform;

    // remind some main JSON areas
    QL_JSON_ASSERT(platform.hardware_settings, "eqasm_backend_cc", "hardware_settings");  // NB: json_get<const json &> unavailable
    const Json &jsonBackendSettings = platform.hardware_settings["eqasm_backend_cc"];

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

// NB: assumes prior test for isReadout()==true
Str Settings::getReadoutMode(const Str &iname) {
    const Json &instruction = platform->find_instruction(iname);
    Str instructionPath = "instructions/"+iname;
    QL_JSON_ASSERT(instruction, "cc", instructionPath);
    return json_get<Str>(instruction["cc"], "readout_mode", instructionPath);
}

// determine whether this is a readout instruction
Bool Settings::isReadout(const Str &iname) {
#if 1    // new semantics
    const Json &instruction = platform->find_instruction(iname);
    Str instructionPath = "instructions/" + iname;
    QL_JSON_ASSERT(instruction, "cc", instructionPath);
    return QL_JSON_EXISTS(instruction["cc"], "readout_mode");
#else
    /*
        NB: we only use the instruction_type "readout" and don't care about the rest
        because the terms "mw" and "flux" don't fully cover gate functionality. It
        would be nice if custom gates could mimic gate_type_t
    */
    // FIXME: it seems that key "instruction/type" is no longer used by the 'core' of OpenQL, so we need a better criterion
    // FIXME: must not trigger in "prepz", which has type "readout" in (some?) configuration files (with empty signal though)
    // FIXME: gate semantics should be handled at the OpenQL core
    return platform->find_instruction_type(iname) == "readout";
#endif
}


Bool Settings::isPragma(const Str &iname) {
    return getPragma(iname).has_value();
}


RawPtr<const Json> Settings::getPragma(const Str &iname) {
    const Json &instruction = platform->find_instruction(iname);
    Str instructionPath = "instructions/" + iname;
    QL_JSON_ASSERT(instruction, "cc", instructionPath);
    if (QL_JSON_EXISTS(instruction["cc"], "pragma")) {
        return &instruction["cc"]["pragma"];
    } else {
        return nullptr;
    }
}


// return wait for instrument latency + SM data distribution
UInt Settings::getReadoutWait() {
    return 20+3;    // FIXME: make configurable
}


// find JSON signal definition for instruction, either inline or via 'ref_signal'
Settings::SignalDef Settings::findSignalDefinition(const Json &instruction, const Str &iname) const {
    SignalDef ret;

    Str instructionPath = "instructions/" + iname;
    QL_JSON_ASSERT(instruction, "cc", instructionPath);
    if (QL_JSON_EXISTS(instruction["cc"], "ref_signal")) {                      // optional syntax: "ref_signal"
        Str refSignal = instruction["cc"]["ref_signal"].get<Str>();
        ret.signal = (*jsonSignals)[refSignal];                                 // poor man's JSON pointer
        if(ret.signal.empty()) {
            QL_JSON_FATAL(
                "instruction '" << iname
                << "': ref_signal '" << refSignal
                << "' does not resolve"
            );
        }
        ret.path = "signals/" + refSignal;
    } else {                                                                    // alternative syntax: "signal"
        ret.signal = json_get<Json>(instruction["cc"], "signal", instructionPath + "/cc");
        QL_DOUT("signal for '" << instruction << "': " << ret.signal);
        ret.path = instructionPath + "/cc/signal";
    }
    return ret;
}


// collect some configuration info for an instrument
Settings::InstrumentInfo Settings::getInstrumentInfo(UInt instrIdx) const {
    InstrumentInfo ret = {nullptr};

    Str instrumentPath = QL_SS2S("instruments[" << instrIdx << "]");    // for JSON error reporting
    if (instrIdx >= jsonInstruments->size()) {
        QL_JSON_FATAL("node not defined: " + instrumentPath);                   // probably an internal backend error
    }
    ret.instrument = &(*jsonInstruments)[instrIdx];

    ret.instrumentName = json_get<Str>(*ret.instrument, "name", instrumentPath);

    QL_JSON_ASSERT(*ret.instrument, "controller", ret.instrumentName);          // first check intermediate node
    // FIXME: check controller/"name" being "cc"?
    ret.slot = json_get<Int>((*ret.instrument)["controller"], "slot", ret.instrumentName+"/controller");
    // FIXME: also return controller/"io_module"?

#if OPT_FEEDBACK
    // optional key 'instruments[]/force_cond_gates_on', can be used to always enable AWG if gate execution is controlled by VSM
    if (QL_JSON_EXISTS(*ret.instrument, "force_cond_gates_on")) {
        ret.forceCondGatesOn = json_get<Bool>(*ret.instrument, "force_cond_gates_on", ret.instrumentName+"/force_cond_gates_on"); // key will exist, but type may be wrong
    }
#endif

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
    ret.controlModeGroupSize = channels / ret.controlModeGroupCnt;  // FIXME: handle 0 div, and rounding

    // verify that group size is allowed
    const Json controlGroupSizes = json_get<const Json>(instrumentDefinition, "control_group_sizes", refInstrumentDefinition);
    // FIXME: find channels

    // FIXME: unfinished: find channels


    return ret;
}


Int Settings::getResultBit(const InstrumentControl &ic, Int group) {
    // FIXME: test similar to settings_cc::getInstrumentControl, move
    // check existence of key 'result_bits'
    if (!QL_JSON_EXISTS(ic.controlMode, "result_bits")) {        // this instrument mode produces results (i.e. it is a measurement device)
        QL_JSON_FATAL("readout requested on instrument '" << ic.ii.instrumentName << "', but key '" << ic.refControlMode << "/result_bits is not present");
    }

    // check existence of key 'result_bits[group]'
    const Json &groupResultBits = ic.controlMode["result_bits"][group];
    UInt nrGroupResultBits = groupResultBits.size();
    if (nrGroupResultBits != 1) {                             // single bit (NB: per group)
        QL_JSON_FATAL("key '" << ic.refControlMode << "/result_bits[" << group << "] must have 1 bit instead of " << nrGroupResultBits);
    }
    return groupResultBits[0].get<Int>();   // bit on digital interface. NB: we assume the result is active high, which is correct for UHF-QC
}


// find instrument&group given instructionSignalType for qubit
// NB: this implies that we map signal *vectors* to groups, i.e. it is not possible to map individual channels
// Conceptually, this is were we map an abstract signal definition, eg: {"flux", q3} (which may also be
// interpreted as port "q3.flux") onto an instrument & group
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
                QL_JSON_FATAL(
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
        QL_JSON_FATAL("No instruments found providing signal type '" << instructionSignalType << "'");
    }
    if (!qubitFound) {
        QL_JSON_FATAL("No instruments found driving qubit " << qubit << " for signal type '" << instructionSignalType << "'");
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
                QL_JSON_FATAL("Array size of static_codeword_override for instruction '" << iname << "' insufficient");
            }
        } else if (operandIdx == 0) {     // NB: JSON '"static_codeword_override": [3]' gives **scalar** result
            staticCodewordOverride = override.get<Int>();
        } else {
            QL_JSON_FATAL("Key static_codeword_override for instruction '" << iname
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
        QL_JSON_FATAL(
            "No static codeword defined for instruction '" << iname
            << "' (we currently require it because automatic assignment is disabled)"
        );
    }
 #endif
    return staticCodewordOverride;
}
//#endif

} // namespace cc
} // namespace arch
} // namespace ql
