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
#include "utils/vec.h"
#include "utils/json.h"

#include <string>
#include <cstddef>  // for size_t etc.

namespace ql {

namespace { using namespace utils; }

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
		// FIXME consider passing a gate&, custom_gate& or (new type) GateOperands&
		const std::string &iname,
		const Vec<UInt> &operands,					// qubit operands (FKA qops)
		const Vec<UInt> &creg_operands,				// classic operands (FKA cops)
		const Vec<UInt> &breg_operands, 			// bit operands e.g. assigned to by measure
		cond_type_t condition,
		const Vec<UInt> &cond_operands,				// 0, 1 or 2 bit operands of condition
		double angle,
		size_t startCycle, size_t durationInCycles
	);
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
    public:	// funcs
    	BundleInfo() = default;

    public:	// vars
    	// output gates
        std::string signalValue;
        unsigned int durationInCycles = 0;
#if OPT_SUPPORT_STATIC_CODEWORDS
        int staticCodewordOverride = settings_cc::NO_STATIC_CODEWORD_OVERRIDE;
#endif
#if OPT_FEEDBACK
        // readout feedback
        bool isMeasFeedback = false;
		Vec<UInt> operands;							// NB: also used by OPT_PRAGMA
		Vec<UInt> creg_operands;
		Vec<UInt> breg_operands;

        // conditional gates
        cond_type_t condition = cond_always;		// FIXME
		Vec<UInt> cond_operands;
#endif
#if OPT_PRAGMA
        // pragma 'gates'
		const Json *pragma = nullptr;
#endif
    }; // information for an instrument group (of channels), for a single instruction
    // FIXME: rename tInstrInfo, store gate as annotation, move to class cc:IR, together with customGate(), bundleStart(), bundleFinish()?


#if OPT_FEEDBACK
	typedef struct {
		int smBit;
		int bit;
		const BundleInfo *bi;									// used for annotation only
	} tFeedbackInfo;											// information for feedback on single instrument group
	using tFeedbackMap = std::map<int, tFeedbackInfo>;			// NB: key is instrument group
#endif

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
	Json codewordTable;                                  		// codewords versus signals per instrument group
	std::stringstream codeSection;                              // the code generated

	// codegen state, kernel scope FIXME: create class
    unsigned int lastEndCycle[MAX_INSTRS];                      // vector[instrIdx], maintain where we got per slot
#if OPT_PRAGMA
	std::string pragmaForLabel;
#endif

	// codegen state, bundle scope
	bool bundleHasFeedback;
    std::vector<std::vector<BundleInfo>> bundleInfo;           	// matrix[instrIdx][group]


private:    // funcs
    // helpers to ease nice assembly formatting
    void emit(const std::string &labelOrComment, const std::string &instr="");
    void emit(const std::string &label, const std::string &instr, const std::string &ops, const std::string &comment="");
	void emit(int sel, const std::string &instr, const std::string &ops, const std::string &comment="");

    // code generation helpers
	void showCodeSoFar() { QL_EOUT("Code so far:\n" << codeSection.str()); }     // provide context to help finding reason. FIXME: limit # lines
    void emitProgramStart();
	void emitOutput(const tCondGateMap &condGateMap, int32_t digOut, unsigned int instrMaxDurationInCycles, size_t instrIdx, size_t startCycle, int slot, const std::string &instrumentName);
	void emitPragma(const Json *pragma, int pragmaSmBit, size_t instrIdx, size_t startCycle, int slot, const std::string &instrumentName);
	void emitFeedback(const tFeedbackMap &feedbackMap, size_t instrIdx, size_t startCycle, int slot, const std::string &instrumentName);
    void padToCycle(size_t instrIdx, size_t startCycle, int slot, const std::string &instrumentName);

    // generic helpers
    tCalcSignalValue calcSignalValue(const settings_cc::tSignalDef &sd, size_t s, const Vec<UInt> &operands, const std::string &iname);
#if !OPT_SUPPORT_STATIC_CODEWORDS
    uint32_t assignCodeword(const std::string &instrumentName, int instrIdx, int group);
#endif
}; // class

} // namespace ql
