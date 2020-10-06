/**
 * @file    settings_cc.cc
 * @date    20201001
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   handle JSON settings for the CC backend
 * @note
 */

#include "settings_cc.h"


void settings_cc::loadBackendSettings(const ql::quantum_platform &platform)
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
const json &settings_cc::findInstrumentDefinition(const std::string &name) const
{
    // FIXME: use json_get
    if JSON_EXISTS(*jsonInstrumentDefinitions, name) {
        return (*jsonInstrumentDefinitions)[name];
    } else {
        JSON_FATAL("Could not find key 'name'=" << name << "in section 'instrument_definitions'");
    }
}
#endif


// find JSON signal definition for instruction, either inline or via 'ref_signal'
settings_cc::tSignalDef settings_cc::findSignalDefinition(const json &instruction, const std::string &iname) const
{
    tSignalDef ret;

    std::string instructionPath = "instructions/"+iname;
    JSON_ASSERT(instruction, "cc", instructionPath);
    if(JSON_EXISTS(instruction["cc"], "ref_signal")) {                          // optional syntax: "ref_signal"
        std::string refSignal = instruction["cc"]["ref_signal"];
        ret.signal = (*jsonSignals)[refSignal];                                 // poor man's JSON pointer
        if(ret.signal.size() == 0) {
            JSON_FATAL("instruction '" << iname <<
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
settings_cc::tInstrumentInfo settings_cc::getInstrumentInfo(size_t instrIdx) const
{
    tInstrumentInfo ret;

    std::string instrumentPath = SS2S("instruments["<<instrIdx<<"]");           // for JSON error reporting
    if(instrIdx >= jsonInstruments->size()) {
        JSON_FATAL("node not defined: " + instrumentPath);                      // probably an internal backend error
    }
    ret.instrument = &(*jsonInstruments)[instrIdx];

    ret.instrumentName = json_get<std::string>(*ret.instrument, "name", instrumentPath);

    JSON_ASSERT(*ret.instrument, "controller", ret.instrumentName);              // first check intermediate node
    // FIXME: check controller/"name" being "cc"?
    int slot = json_get<int>((*ret.instrument)["controller"], "slot", ret.instrumentName+"/controller");
    // FIXME: also return controller/"io_module"?

    return ret;
}


settings_cc::tInstrumentControl settings_cc::getInstrumentControl(size_t instrIdx) const
{
    tInstrumentControl ret;

    ret.ii = getInstrumentInfo(instrIdx);

    // get control mode for instrument
    ret.refControlMode = json_get<std::string>(*ret.ii.instrument, "ref_control_mode", ret.ii.instrumentName);
    ret.controlMode = json_get<const json>(*jsonControlModes, ret.refControlMode, "control_modes");   // the control mode definition for our instrument
    ret.controlModeGroupCnt = ret.controlMode["control_bits"].size();   // how many groups of control bits does the control mode specify

    return ret;
}


// find instrument&group given instructionSignalType for qubit
// NB: this implies that we map signal *vectors* to groups, i.e. it is not possible to map individual channels
settings_cc::tSignalInfo settings_cc::findSignalInfoForQubit(const std::string &instructionSignalType, size_t qubit) const
{
    tSignalInfo ret;
    bool signalTypeFound = false;
    bool qubitFound = false;

    // iterate over instruments
    for(size_t instrIdx=0; instrIdx<jsonInstruments->size() && !qubitFound; instrIdx++) {
        tInstrumentControl ic = getInstrumentControl(instrIdx);
        std::string instrumentSignalType = json_get<std::string>(*ic.ii.instrument, "signal_type", ic.ii.instrumentName);
        if(instrumentSignalType == instructionSignalType) {
            signalTypeFound = true;
            const json qubits = json_get<const json>(*ic.ii.instrument, "qubits", ic.ii.instrumentName);   // NB: json_get<const json&> unavailable

            // verify group size: qubits vs. control mode
            int qubitGroupCnt = qubits.size();                                  // NB: JSON key qubits is a 'matrix' of [groups*qubits]
            if(qubitGroupCnt != ic.controlModeGroupCnt) {
                JSON_FATAL("instrument " << ic.ii.instrumentName <<
                           ": number of qubit groups " << qubitGroupCnt <<
                           " does not match number of control_bits groups " << ic.controlModeGroupCnt <<
                           " of selected control mode '" << ic.refControlMode << "'");
            }

            // anyone connected to qubit?
            for(size_t group=0; group<qubitGroupCnt && !qubitFound; group++) {
                for(size_t idx=0; idx<qubits[group].size() && !qubitFound; idx++) {
                    if(qubits[group][idx] == qubit) {
                        qubitFound = true;                                      // also: stop searching

                        DOUT("qubit " << qubit
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
    if(!signalTypeFound) {
        JSON_FATAL("No instruments found providing signal type '" << instructionSignalType << "'");
    }
    if(!qubitFound) {
        JSON_FATAL("No instruments found driving qubit " << qubit << " for signal type '" << instructionSignalType << "'");
    }

    return ret;
}

/************************************************************************\
| Static functions processing JSON
\************************************************************************/

//#if OPT_SUPPORT_STATIC_CODEWORDS
int settings_cc::findStaticCodewordOverride(const json &instruction, size_t operandIdx, const std::string &iname)
{
    // look for optional codeword override
    int staticCodewordOverride = -1;    // -1 means unused
    if(JSON_EXISTS(instruction["cc"], "static_codeword_override")) {    // optional keyword
 #if OPT_STATIC_CODEWORDS_ARRAYS
        const json &override = instruction["cc"]["static_codeword_override"];
        if(override.is_array()) {
            if(override.size() > operandIdx) {
                staticCodewordOverride = override[operandIdx];
            } else {
                JSON_FATAL("Array size of static_codeword_override for instruction '" << iname << "' insufficient");
            }
        } else if (operandIdx==0) {     // NB: JSON '"static_codeword_override": [3]' gives **scalar** result
            staticCodewordOverride = override;
        } else {
            JSON_FATAL("Key static_codeword_override for instruction '" << iname
                  << "' should be an array (found '" << override << "' in '" << instruction << "')");
        }
 #else
        staticCodewordOverride = instruction["cc"]["static_codeword_override"];
 #endif
        DOUT("Found static_codeword_override=" << staticCodewordOverride <<
             " for instruction '" << iname << "'");
    }
 #if 1 // FIXME: require override
    if(staticCodewordOverride < 0) {
        JSON_FATAL("No static codeword defined for instruction '" << iname <<
            "' (we currently require it because automatic assignment is disabled)");
    }
 #endif
    return staticCodewordOverride;
}
//#endif


