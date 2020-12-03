/**
 * @file    codegen_cc.h
 * @date    201810xx
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   code generator backend for the Central Controller
 * @note    here we don't check whether the sequence of calling code generator
 *          functions is correct
 */

#pragma once

#include "options_cc.h"
#include "datapath_cc.h"
#include "settings_cc.h"
#include "vcd_cc.h"
#include "platform.h"

#include <string>
#include <cstddef>  // for size_t etc.
#include "utils/vec.h"
#include "utils/json.h"

namespace ql {

class codegen_cc
{
public: //  functions
    codegen_cc() = default;
    ~codegen_cc() = default;

    // Generic
    void init(const quantum_platform &platform);
    std::string getProgram();                           // return the CC source code that was created
    std::string getMap();                               // return a map of codeword assignments, useful for configuring AWGs

    // Compile support
    void programStart(const std::string &progName);
    void programFinish(const std::string &progName);
    void kernelStart();
    void kernelFinish(const std::string &kernelName, size_t durationInCycles);
    void bundleStart(const std::string &cmnt);
    void bundleFinish(size_t startCycle, size_t durationInCycles, bool isLastBundle);

    // Quantum instructions
    void customGate(
            const std::string &iname,
            const utils::Vec<utils::UInt> &qops,
            const utils::Vec<utils::UInt> &cops,
            double angle, size_t startCycle, size_t durationInCycles);
    void nopGate();

    // Classical operations on kernels
    void ifStart(size_t op0, const std::string &opName, size_t op1);
    void elseStart(size_t op0, const std::string &opName, size_t op1);
    void forStart(const std::string &label, int iterations);
    void forEnd(const std::string &label);
    void doWhileStart(const std::string &label);
    void doWhileEnd(const std::string &label, size_t op0, const std::string &opName, size_t op1);

    void comment(const std::string &c);

private:    // types
    class BundleInfo {
    public:
    	BundleInfo();

    public:	// vars
#if OPT_FEEDBACK
        // readout
        int readoutCop;                 // classic operand for readout. NB: we encode an implicit cop as -1
        int readoutQubit;

        // conditional gates
        int condition;
        // FIXME: add cops
#endif
    	// output gates
        std::string signalValue;
        unsigned int durationInCycles;
#if OPT_SUPPORT_STATIC_CODEWORDS
        int staticCodewordOverride;
#endif
#if OPT_PRAGMA
        // pragma 'gates'
		const utils::Json *pragma;
		utils::Vec<utils::UInt> pragmaCops;
		utils::Vec<utils::UInt> pragmaQops;		// FIXME: naming, integrate with readout(Cop,Qubit}
#endif
    }; // information for an instrument group (of channels), for a single instruction
    // FIXME: rename tInstrInfo, store gate as annotation, move to class cc:IR, together with custmGate(), bundleStart(), bundleFinish()?


    typedef struct {
        std::string signalValueString;
        unsigned int operandIdx;
        settings_cc::tSignalInfo si;
    } tCalcSignalValue;	// return type for calcSignalValue()


private:    // vars
    static const int MAX_SLOTS = 12;                            // physical maximum of CC
    static const int MAX_INSTRS = MAX_SLOTS;                    // maximum number of instruments in config file
    static const int MAX_GROUPS = 32;                           // based on VSM, which currently has the largest number of groups

    const quantum_platform *platform;                           // remind platform
    settings_cc settings;                                       // handling of JSON settings
    datapath_cc dp;												// handling of CC datapath
    vcd_cc vcd;                                                 // handling of VCD file output

    bool verboseCode = true;                                    // option to output extra comments in generated code. FIXME: not yet configurable
    bool mapPreloaded = false;

	// codegen state, program scope
	utils::Json codewordTable;                                  // codewords versus signals per instrument group
	std::stringstream codeSection;                              // the code generated

	// codegen state, kernel scope FIXME: create class
    unsigned int lastEndCycle[MAX_INSTRS];                      // vector[instrIdx], maintain where we got per slot
#if OPT_PRAGMA
	std::string pragmaForLabel;
#endif

	// codegen state, bundle scope
    std::vector<std::vector<BundleInfo>> bundleInfo;           	// matrix[instrIdx][group]

private:    // funcs
    // Some helpers to ease nice assembly formatting
    void emit(const std::string &labelOrComment, const std::string &instr="");
    void emit(const std::string &label, const std::string &instr, const std::string &ops, const std::string &comment="");
	void emit(int sel, const std::string &instr, const std::string &ops, const std::string &comment="");

    // helpers
    void emitProgramStart();
    void padToCycle(size_t instrIdx, size_t startCycle, int slot, const std::string &instrumentName);
    tCalcSignalValue calcSignalValue(const settings_cc::tSignalDef &sd, size_t s, const utils::Vec<utils::UInt> &qops, const std::string &iname);
#if !OPT_SUPPORT_STATIC_CODEWORDS
    uint32_t assignCodeword(const std::string &instrumentName, int instrIdx, int group);
#endif
}; // class

} // namespace ql
