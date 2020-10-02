/**
 * @file    settings_cc.h
 * @date    20201001
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   handle JSON settings for the CC backend
 * @note
 */

#ifndef ARCH_CC_SETTINGS_CC_H
#define ARCH_CC_SETTINGS_CC_H

#include "platform.h"
#include "json.h"

using json = nlohmann::json;        // FIXME: should not be part of interface

class settings_cc
{
public: // types
    typedef struct {
        json signal;                // a copy of the signal node found
        std::string path;           // path of the node, for reporting purposes
    } tSignalDef;

    typedef struct {
        std::string instrumentName;
        int slot;
        std::string refControlMode;
        json controlMode;
        size_t nrControlBitsGroups;
    } tInstrumentInfo;

    typedef struct {
        int instrIdx;               // the index into JSON "eqasm_backend_cc/instruments" that provides the signal
        int group;                  // the group of channels within the instrument that provides the signal
    } tSignalInfo;


public: // functions
    void load_backend_settings(const ql::quantum_platform &platform);
    const json &findInstrumentDefinition(const std::string &name) const;
    tSignalDef findSignalDefinition(const json &instruction, const std::string &iname) const;
    tInstrumentInfo getInstrumentInfo(size_t instrIdx) const;

    // find instrument/group providing instructionSignalType for qubit
    tSignalInfo findSignalInfoForQubit(const std::string &instructionSignalType, size_t qubit) const;

    // 'getters'
    const json &getInstrumentAtIdx(size_t instrIdx) const { return (*jsonInstruments)[instrIdx]; }
    size_t getInstrumentsSize() const { return jsonInstruments->size(); }

private:    // vars
    const json *jsonInstrumentDefinitions;
    const json *jsonControlModes;
    const json *jsonInstruments;
    const json *jsonSignals;
}; // class

#endif // ndef ARCH_CC_SETTINGS_CC_H
