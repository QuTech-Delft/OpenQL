/**
 * @file    settings_cc.h
 * @date    20201001
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   handle JSON settings for the CC backend
 * @note
 */

#pragma once

#include "options_cc.h"
#include "platform.h"
#include "utils/json.h"


namespace ql {

class settings_cc
{
public: // types
    typedef struct {
        utils::Json signal;         // a copy of the signal node found
        std::string path;           // path of the node, for reporting purposes
    } tSignalDef;

    typedef struct {
        const utils::Json *instrument;
        std::string instrumentName; // key 'instruments[]/name'
        int slot;                   // key 'instruments[]/controller/slot'
#if OPT_FEEDBACK
        bool forceCondGatesOn;      // optional key 'instruments[]/force_cond_gates_on', can be used to always enable AWG if gate execution is controlled by VSM
#endif
    } tInstrumentInfo;              // information from key 'instruments'

    typedef struct {
        tInstrumentInfo ii;
        std::string refControlMode;
        utils::Json controlMode;    // FIXME: pointer
        size_t controlModeGroupCnt; // number of groups in key 'control_bits' of effective control mode
        size_t controlModeGroupSize;// the size (#channels) of the effective control mode group
    } tInstrumentControl;           // information from key 'instruments/ref_control_mode'

    typedef struct {
        tInstrumentControl ic;
        int instrIdx;               // the index into JSON "eqasm_backend_cc/instruments" that provides the signal
        int group;                  // the group of channels within the instrument that provides the signal
    } tSignalInfo;

    static const int NO_STATIC_CODEWORD_OVERRIDE = -1;

public: // functions
    settings_cc() = default;
    ~settings_cc() = default;

    void loadBackendSettings(const quantum_platform &platform);
	bool isReadout(const std::string &iname);
	bool isPragma(const std::string &iname);
	const utils::Json *getPragma(const std::string &iname);
	int getReadoutWait();
	tSignalDef findSignalDefinition(const utils::Json &instruction, const std::string &iname) const;
    tInstrumentInfo getInstrumentInfo(size_t instrIdx) const;
    tInstrumentControl getInstrumentControl(size_t instrIdx) const;
	int getResultBit(const tInstrumentControl &ic, int group) const;

    // find instrument/group providing instructionSignalType for qubit
    tSignalInfo findSignalInfoForQubit(const std::string &instructionSignalType, size_t qubit) const;

    static int findStaticCodewordOverride(const utils::Json &instruction, size_t operandIdx, const std::string &iname);

    // 'getters'
    const utils::Json &getInstrumentAtIdx(size_t instrIdx) const { return (*jsonInstruments)[instrIdx]; }
    size_t getInstrumentsSize() const { return jsonInstruments->size(); }

private:    // vars
	const quantum_platform *platform;                           // remind platform
    const utils::Json *jsonInstrumentDefinitions;
    const utils::Json *jsonControlModes;
    const utils::Json *jsonInstruments;
    const utils::Json *jsonSignals;
}; // class

} // namespace ql
