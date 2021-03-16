/**
 * @file    arch/cc/codegen_cc.h
 * @date    201810xx
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   code generator backend for the Central Controller
 * @note    here we don't check whether the sequence of calling code generator
 *          functions is correct
 */

#pragma once

#include "types_cc.h"
#include "options_cc.h"
#include "bundle_info.h"
#include "datapath_cc.h"
#include "settings_cc.h"
#include "vcd_cc.h"
#include "platform.h"

namespace ql {
namespace arch {
namespace cc {

class Codegen {
public: //  functions
    Codegen() = default;
    ~Codegen() = default;

    // Generic
    void init(const quantum_platform &platform);
    Str getProgram();                           // return the CC source code that was created
    Str getMap();                               // return a map of codeword assignments, useful for configuring AWGs

    // Compile support
    void programStart(const Str &progName);
    void programFinish(const Str &progName);
    void kernelStart();
    void kernelFinish(const Str &kernelName, UInt durationInCycles);
    void bundleStart(const Str &cmnt);
    void bundleFinish(UInt startCycle, UInt durationInCycles, Bool isLastBundle);

    // Quantum instructions
    void customGate(
        // FIXME consider passing a gate&, custom_gate& or (new type) GateOperands&
        const Str &iname,
        const Vec<UInt> &operands,                  // qubit operands (FKA qops)
        const Vec<UInt> &creg_operands,             // classic operands (FKA cops)
        const Vec<UInt> &breg_operands,             // bit operands e.g. assigned to by measure
        cond_type_t condition,
        const Vec<UInt> &cond_operands,             // 0, 1 or 2 bit operands of condition
        Real angle,
        UInt startCycle, UInt durationInCycles
    );
    void nopGate();

    // Classical operations on kernels
    void ifStart(UInt op0, const Str &opName, UInt op1);
    void elseStart(UInt op0, const Str &opName, UInt op1);
    void forStart(const Str &label, UInt iterations);
    void forEnd(const Str &label);
    void doWhileStart(const Str &label);
    void doWhileEnd(const Str &label, UInt op0, const std::string &opName, UInt op1);

    void comment(const Str &c);

private:    // types
    struct CodeGenInfo {
        Bool instrHasOutput;
        Digital digOut;                                         // the digital output value sent over the instrument interface
        UInt instrMaxDurationInCycles;                          // maximum duration over groups that are used, one instrument
#if OPT_FEEDBACK
        FeedbackMap feedbackMap;
        CondGateMap condGateMap;
#endif
#if OPT_PRAGMA
        RawPtr<const Json> pragma;
        Int pragmaSmBit;
#endif
        // info copied from tInstrumentInfo
        Str instrumentName;
        Int slot;
    };

    using CodeGenMap = Map<Int, CodeGenInfo>;                   // NB: key is instrument group

    struct CalcSignalValue {
        Str signalValueString;
        UInt operandIdx;
        Settings::SignalInfo si;
    }; // return type for calcSignalValue()


private:    // vars
    static const Int MAX_SLOTS = 12;                            // physical maximum of CC
    static const Int MAX_GROUPS = 32;                           // based on VSM, which currently has the largest number of groups

    const quantum_platform *platform;                           // remind platform
    Settings settings;                                          // handling of JSON settings
    Datapath dp;                                                // handling of CC datapath
    Vcd vcd;                                                    // handling of VCD file output

    Bool runOnce = false;                                       // option to run once instead of repeating indefinitely
    Bool verboseCode = true;                                    // option to output extra comments in generated code
    Bool mapPreloaded = false;                                  // flag whether we have a preloaded map

    // codegen state, program scope
    Json codewordTable;                                         // codewords versus signals per instrument group
    StrStrm codeSection;                                        // the code generated

    // codegen state, kernel scope FIXME: create class
    UInt lastEndCycle[MAX_INSTRS];                              // vector[instrIdx], maintain where we got per slot
#if OPT_PRAGMA
    Str pragmaForLabel;
#endif

    // codegen state, bundle scope
    Vec<Vec<BundleInfo>> bundleInfo;                            // matrix[instrIdx][group]


private:    // funcs
    // helpers to ease nice assembly formatting
    void emit(const Str &labelOrComment, const Str &instr="");
    void emit(const Str &label, const Str &instr, const Str &ops, const Str &comment="");
    void emit(Int slot, const Str &instr, const Str &ops, const Str &comment="");

    // code generation helpers
    void showCodeSoFar();
    void emitProgramStart(const Str &progName);
    void emitProgramFinish();
    void emitFeedback(const FeedbackMap &feedbackMap, UInt instrIdx, UInt startCycle, Int slot, const Str &instrumentName);
    void emitOutput(const CondGateMap &condGateMap, Digital digOut, UInt instrMaxDurationInCycles, UInt instrIdx, UInt startCycle, Int slot, const Str &instrumentName);
    void emitPragma(const Json &pragma, Int pragmaSmBit, UInt instrIdx, UInt startCycle, Int slot, const Str &instrumentName);
    void emitPadToCycle(UInt instrIdx, UInt startCycle, Int slot, const Str &instrumentName);

    // generic helpers
    CodeGenMap collectCodeGenInfo(UInt startCycle, UInt durationInCycles);
    CalcSignalValue calcSignalValue(const Settings::SignalDef &sd, UInt s, const Vec<UInt> &operands, const Str &iname);
#if !OPT_SUPPORT_STATIC_CODEWORDS
    Codeword assignCodeword(const Str &instrumentName, Int instrIdx, Group group);
#endif

}; // class

} // namespace cc
} // namespace arch
} // namespace ql
