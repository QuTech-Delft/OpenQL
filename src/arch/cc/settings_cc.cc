/**
 * @file    settings_cc.cc
 * @date    20201001
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   handle JSON settings for the CC backend
 * @note
 */

#include "settings_cc.h"


void settings_cc::load_backend_settings(const ql::quantum_platform &platform)
{
    // remind some main JSON areas
    JSON_ASSERT(platform.hardware_settings, "eqasm_backend_cc", "hardware_settings");  // NB: json_get<const json &> unavailable
    const json &jsonBackendSettings = platform.hardware_settings["eqasm_backend_cc"];

    JSON_ASSERT(jsonBackendSettings, "instrument_definitions", "eqasm_backend_cc");
    jsonInstrumentDefinitions = &jsonBackendSettings["instrument_definitions"];

    JSON_ASSERT(jsonBackendSettings, "control_modes", "eqasm_backend_cc");
    jsonControlModes = &jsonBackendSettings["control_modes"];

    JSON_ASSERT(jsonBackendSettings, "instruments", "eqasm_backend_cc");
    jsonInstruments = &jsonBackendSettings["instruments"];

    JSON_ASSERT(jsonBackendSettings, "signals", "eqasm_backend_cc");
    jsonSignals = &jsonBackendSettings["signals"];

#if 0   // FIXME: print some info, which also helps detecting errors early on
    // read instrument definitions
    // FIXME: the following requires json>v3.1.0:  for(auto& id : jsonInstrumentDefinitions->items()) {
    for(size_t i=0; i<jsonInstrumentDefinitions->size(); i++) {
        std::string idName = jsonInstrumentDefinitions[i];        // NB: uses type conversion to get node value
        DOUT("found instrument definition: '" << idName <<"'");
    }

    // read control modes
    for(size_t i=0; i<jsonControlModes->size(); i++)
    {
        const json &name = (*jsonControlModes)[i]["name"];
        DOUT("found control mode '" << name <<"'");
    }

    // read instruments
    const json &ccSetupType = jsonCcSetup["type"];

    // CC specific
    const json &ccSetupSlots = jsonCcSetup["slots"];                      // FIXME: check against jsonInstrumentDefinitions
    for(size_t slot=0; slot<ccSetupSlots.size(); slot++) {
        const json &instrument = ccSetupSlots[slot]["instrument"];
        std::string instrumentName = instrument["name"];
        std::string signalType = instrument["signal_type"];

        DOUT("found instrument: name='" << instrumentName << "', signal type='" << signalType << "'");
    }
#endif
}


#if 1  // FIXME: only used by OPT_CALCULATE_LATENCIES
const json &settings_cc::findInstrumentDefinition(const std::string &name)
{
    // FIXME: use json_get
    if JSON_EXISTS(*jsonInstrumentDefinitions, name) {
        return (*jsonInstrumentDefinitions)[name];
    } else {
        FATAL("Could not find key 'name'=" << name << "in JSON section 'instrument_definitions'");
    }
}
#endif


// find JSON signal definition for instruction, either inline or via 'ref_signal'
settings_cc::tSignalDef settings_cc::findSignalDefinition(const json &instruction, const std::string &iname)
{
    tSignalDef ret;

    std::string instructionPath = "instructions/"+iname;
    JSON_ASSERT(instruction, "cc", instructionPath);
    if(JSON_EXISTS(instruction["cc"], "ref_signal")) {                          // optional syntax: "ref_signal"
        std::string refSignal = instruction["cc"]["ref_signal"];
        ret.signal = (*jsonSignals)[refSignal];                            // poor man's JSON pointer
        if(ret.signal.size() == 0) {
            FATAL("Error in JSON definition of instruction '" << iname <<
                  "': ref_signal '" << refSignal << "' does not resolve");
        }
        ret.path = "signals/"+refSignal;
    } else {                                                                    // alternative syntax: "signal"
        ret.signal = json_get<json>(instruction["cc"], "signal", instructionPath+"/cc");
        DOUT("signal for '" << instruction << "': " << ret.signal);
        ret.path = instructionPath+"/cc/signal";
    }
    return ret;
}


// collect some configuration info for an instrument
settings_cc::tInstrumentInfo settings_cc::getInstrumentInfo(size_t instrIdx) {
    tInstrumentInfo ret;

    const json &instrument = (*jsonInstruments)[instrIdx];              // NB: always exists (if we iterate jsonInstruments)

    ret.instrumentName = json_get<std::string>(instrument, "name", SS2S("instruments["<<instrIdx<<"]"));
    JSON_ASSERT(instrument, "controller", ret.instrumentName);           // first check intermediate node
    ret.slot = json_get<int>(instrument["controller"], "slot", ret.instrumentName+"/controller");    // FIXME: assuming controller being cc

// FIXME: split off to getInstrumentControl
    // get control mode for instrument
    ret.refControlMode = json_get<std::string>(instrument, "ref_control_mode", ret.instrumentName);
    ret.controlMode = json_get<const json>(*jsonControlModes, ret.refControlMode, "control_modes");   // the control mode definition for our instrument
    ret.nrControlBitsGroups = ret.controlMode["control_bits"].size();        // how many groups of control bits does the control mode specify

    return ret;
}


// find instrument&group providing instructionSignalType for qubit
settings_cc::tSignalInfo settings_cc::findSignalInfoForQubit(const std::string &instructionSignalType, size_t qubit)
{
    tSignalInfo ret = {-1, -1};
    bool signalTypeFound = false;
    bool qubitFound = false;

    // iterate over instruments
    for(size_t instrIdx=0; instrIdx<jsonInstruments->size() && !qubitFound; instrIdx++) {
        const json &instrument = (*jsonInstruments)[instrIdx];                  // NB: always exists
        std::string instrumentPath = SS2S("instruments["<<instrIdx<<"]");       // for JSON error reporting
        std::string instrumentSignalType = json_get<std::string>(instrument, "signal_type", instrumentPath);
        if(instrumentSignalType == instructionSignalType) {
            signalTypeFound = true;
            std::string instrumentName = json_get<std::string>(instrument, "name", instrumentPath);
            const json qubits = json_get<const json>(instrument, "qubits", instrumentPath);   // NB: json_get<const json&> unavailable

            // FIXME: verify group size: qubits vs. control mode
            // FIXME: verify signal dimensions

            // anyone connected to qubit?
            for(size_t group=0; group<qubits.size() && !qubitFound; group++) {
                for(size_t idx=0; idx<qubits[group].size() && !qubitFound; idx++) {
                    if(qubits[group][idx] == qubit) {
                        qubitFound = true;                                      // also: stop searching

                        DOUT("qubit " << qubit
                             << " signal type '" << instructionSignalType
                             << "' driven by instrument '" << instrumentName
                             << "' group " << group
                             );

                        ret.instrIdx = instrIdx;
                        ret.group = group;
                    }
                }
            }
        }
    }
    if(!signalTypeFound) {
        FATAL("No instruments found providing signal type '" << instructionSignalType << "'");
    }
    if(!qubitFound) {
        FATAL("No instruments found driving qubit " << qubit << " for signal type '" << instructionSignalType << "'");
    }

    return ret;
}
