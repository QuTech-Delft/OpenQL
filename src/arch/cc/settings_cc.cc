/**
 * @file    settings_cc.cc
 * @date    20201001
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   handle JSON settings for the CC backend
 * @note
 */

#include "settings_cc.h"

namespace ql {

void settings_cc::loadBackendSettings(const quantum_platform &platform)
{
    // remind some main JSON areas
    QL_JSON_ASSERT(platform.hardware_settings, "eqasm_backend_cc", "hardware_settings");  // NB: json_get<const json &> unavailable
    const utils::Json &jsonBackendSettings = platform.hardware_settings["eqasm_backend_cc"];

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
    // FIXME: the following requires Json>v3.1.0:  for(auto& id : jsonInstrumentDefinitions->items()) {
    for(size_t i=0; i<jsonInstrumentDefinitions->size(); i++) {
        std::string idName = jsonInstrumentDefinitions[i];        // NB: uses type conversion to get node value
        QL_DOUT("found instrument definition: '" << idName <<"'");
    }

    // read control modes
    for(size_t i=0; i<jsonControlModes->size(); i++)
    {
        const Json &name = (*jsonControlModes)[i]["name"];
        QL_DOUT("found control mode '" << name <<"'");
    }

    // read instruments
    const Json &ccSetupType = jsonCcSetup["type"];

    // CC specific
    const Json &ccSetupSlots = jsonCcSetup["slots"];                      // FIXME: check against jsonInstrumentDefinitions
    for(size_t slot=0; slot<ccSetupSlots.size(); slot++) {
        const Json &instrument = ccSetupSlots[slot]["instrument"];
        std::string instrumentName = instrument["name"];
        std::string signalType = instrument["signal_type"];

        QL_DOUT("found instrument: name='" << instrumentName << "', signal type='" << signalType << "'");
    }
#endif
}


// find JSON signal definition for instruction, either inline or via 'ref_signal'
settings_cc::tSignalDef settings_cc::findSignalDefinition(const utils::Json &instruction, const std::string &iname) const
{
    tSignalDef ret;

    std::string instructionPath = "instructions/"+iname;
    QL_JSON_ASSERT(instruction, "cc", instructionPath);
    if (QL_JSON_EXISTS(instruction["cc"], "ref_signal")) {                          // optional syntax: "ref_signal"
        std::string refSignal = instruction["cc"]["ref_signal"];
        ret.signal = (*jsonSignals)[refSignal];                                 // poor man's JSON pointer
        if (ret.signal.size() == 0) {
            QL_JSON_FATAL("instruction '" << iname <<
                                          "': ref_signal '" << refSignal << "' does not resolve");
        }
        ret.path = "signals/"+refSignal;
    } else {                                                                    // alternative syntax: "signal"
        ret.signal = utils::json_get<utils::Json>(instruction["cc"], "signal", instructionPath + "/cc");
        QL_DOUT("signal for '" << instruction << "': " << ret.signal);
        ret.path = instructionPath+"/cc/signal";
    }
    return ret;
}


// collect some configuration info for an instrument
settings_cc::tInstrumentInfo settings_cc::getInstrumentInfo(size_t instrIdx) const
{
    tInstrumentInfo ret;

    std::string instrumentPath = QL_SS2S("instruments[" << instrIdx << "]");           // for JSON error reporting
    if(instrIdx >= jsonInstruments->size()) {
        QL_JSON_FATAL("node not defined: " + instrumentPath);                      // probably an internal backend error
    }
    ret.instrument = &(*jsonInstruments)[instrIdx];

    ret.instrumentName = utils::json_get<std::string>(*ret.instrument, "name", instrumentPath);

    QL_JSON_ASSERT(*ret.instrument, "controller", ret.instrumentName);              // first check intermediate node
    // FIXME: check controller/"name" being "cc"?
    ret.slot = utils::json_get<int>((*ret.instrument)["controller"], "slot", ret.instrumentName+"/controller");
    // FIXME: also return controller/"io_module"?

#if OPT_FEEDBACK
    // optional key 'instruments[]/force_cond_gates_on'
    if(QL_JSON_EXISTS(*ret.instrument, "force_cond_gates_on")) {
        ret.forceCondGatesOn = json_get<bool>((*ret.instrument), "force_cond_gates_on", ret.instrumentName+"/force_cond_gates_on"); // key will exist, but type may be wrong
    }
#endif

    return ret;
}


settings_cc::tInstrumentControl settings_cc::getInstrumentControl(size_t instrIdx) const
{
    tInstrumentControl ret;

    ret.ii = getInstrumentInfo(instrIdx);

    // get control mode reference for for instrument
    ret.refControlMode = utils::json_get<std::string>(*ret.ii.instrument, "ref_control_mode", ret.ii.instrumentName);

    // get control mode definition for our instrument
    ret.controlMode = utils::json_get<const utils::Json>(*jsonControlModes, ret.refControlMode, "control_modes");

    // how many groups of control bits does the control mode specify (NB: 0 on missing key)
    ret.controlModeGroupCnt = ret.controlMode["control_bits"].size();

#if OPT_CROSSCHECK_INSTRUMENT_DEF   // FIXME: WIP
    // get instrument definition reference for for instrument
    std::string refInstrumentDefinition = json_get<std::string>(*ret.ii.instrument, "ref_instrument_definition", ret.ii.instrumentName);

    // get instrument definition for our instrument
    const Json instrumentDefinition = json_get<const Json>(*jsonInstrumentDefinitions, refInstrumentDefinition, "instrument_definitions");

    // get number of channels of instrument
    int channels = json_get<int>(instrumentDefinition, "channels", refInstrumentDefinition);

    // calculate groups size (#channels) of control mode
    ret.controlModeGroupSize = channels / ret.controlModeGroupCnt;  // FIXME: handle 0 div, and rounding

    // verify that group size is allowed
    const Json controlGroupSizes = json_get<const Json>(instrumentDefinition, "control_group_sizes", refInstrumentDefinition);
    // FIXME: find channels

#endif

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
        std::string instrumentSignalType = utils::json_get<std::string>(*ic.ii.instrument, "signal_type", ic.ii.instrumentName);
        if(instrumentSignalType == instructionSignalType) {
            signalTypeFound = true;
            const utils::Json qubits = utils::json_get<const utils::Json>(*ic.ii.instrument, "qubits", ic.ii.instrumentName);   // NB: json_get<const json&> unavailable

            // verify group size: qubits vs. control mode
            int qubitGroupCnt = qubits.size();                                  // NB: JSON key qubits is a 'matrix' of [groups*qubits]
            if(qubitGroupCnt != ic.controlModeGroupCnt) {
                QL_JSON_FATAL("instrument " << ic.ii.instrumentName <<
                                            ": number of qubit groups " << qubitGroupCnt <<
                                            " does not match number of control_bits groups " << ic.controlModeGroupCnt <<
                                            " of selected control mode '" << ic.refControlMode << "'");
            }

            // anyone connected to qubit?
            for(size_t group=0; group<qubitGroupCnt && !qubitFound; group++) {
                for(size_t idx=0; idx<qubits[group].size() && !qubitFound; idx++) {
                    if(qubits[group][idx] == qubit) {
                        qubitFound = true;                                      // also: stop searching

                        QL_DOUT("qubit " << qubit
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
        QL_JSON_FATAL("No instruments found providing signal type '" << instructionSignalType << "'");
    }
    if(!qubitFound) {
        QL_JSON_FATAL("No instruments found driving qubit " << qubit << " for signal type '" << instructionSignalType << "'");
    }

    return ret;
}

/************************************************************************\
| Static functions processing JSON
\************************************************************************/

//#if OPT_SUPPORT_STATIC_CODEWORDS
int settings_cc::findStaticCodewordOverride(const utils::Json &instruction, size_t operandIdx, const std::string &iname)
{
    // look for optional codeword override
    int staticCodewordOverride = -1;    // -1 means unused
    if(QL_JSON_EXISTS(instruction["cc"], "static_codeword_override")) {    // optional keyword
 #if OPT_STATIC_CODEWORDS_ARRAYS
        const utils::Json &override = instruction["cc"]["static_codeword_override"];
        if(override.is_array()) {
            if(override.size() > operandIdx) {
                staticCodewordOverride = override[operandIdx];
            } else {
                QL_JSON_FATAL("Array size of static_codeword_override for instruction '" << iname << "' insufficient");
            }
        } else if (operandIdx==0) {     // NB: JSON '"static_codeword_override": [3]' gives **scalar** result
            staticCodewordOverride = override;
        } else {
            QL_JSON_FATAL("Key static_codeword_override for instruction '" << iname
                                                                           << "' should be an array (found '" << override << "' in '" << instruction << "')");
        }
 #else
        staticCodewordOverride = instruction["cc"]["static_codeword_override"];
 #endif
        QL_DOUT("Found static_codeword_override=" << staticCodewordOverride <<
                                                  " for instruction '" << iname <<
                                                  "', operand index " << operandIdx);
    }
 #if 1 // FIXME: require override
    if(staticCodewordOverride < 0) {
        QL_JSON_FATAL("No static codeword defined for instruction '" << iname <<
                                                                     "' (we currently require it because automatic assignment is disabled)");
    }
 #endif
    return staticCodewordOverride;
}
//#endif


} // namespace ql


