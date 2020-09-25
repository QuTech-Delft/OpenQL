/**
 * @file    codegen_cc.h
 * @date    201810xx
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   code generator backend for the Central Controller
 * @note    here we don't check whether the sequence of calling code generator
 *          functions is correct
 */

#ifndef ARCH_CC_CODEGEN_CC_H
#define ARCH_CC_CODEGEN_CC_H

// options
#define OPT_SUPPORT_STATIC_CODEWORDS    1
#define OPT_STATIC_CODEWORDS_ARRAYS     1   // JSON field static_codeword_override is an array with one element per qubit parameter
#define OPT_VCD_OUTPUT                  1   // output Value Change Dump file for GTKWave viewer
#define OPT_RUN_ONCE                    0   // 0=loop indefinitely (CC-light emulation)
#define OPT_CALCULATE_LATENCIES         0   // fixed compensation based on instrument latencies
#define OPT_OLD_SEQBAR_SEMANTICS        0   // support old semantics of seqbar instruction. Will be deprecated
#define OPT_FEEDBACK                    0   // feedback support. Coming feature

#include "json.h"
#include "platform.h"
#if OPT_VCD_OUTPUT
 #include "vcd.h"
#endif

#include <string>

typedef struct {
    json node;                  // a copy of the node found
    std::string path;           // path of the node, for reporting purposes
} tJsonNodeInfo;


class codegen_cc
{
private: // types
    typedef struct {
        int instrIdx;               // the index into JSON "eqasm_backend_cc/instruments" that provides the signal
        int group;                  // the group of channels within the instrument that provides the signal
    } tSignalInfo;

    typedef struct {
        std::string signalValue;
        unsigned int durationInNs;
        int readoutCop;             // classic operand for readout. NB: we encode 'unused' as -1
#if OPT_SUPPORT_STATIC_CODEWORDS
        int staticCodewordOverride;
#endif
    } tGroupInfo;                   // information for an instrument group (of channels), for a single instruction

public:
    codegen_cc() = default;
    ~codegen_cc() = default;

    // Generic
    void init(const ql::quantum_platform &platform);
    std::string getCode();                              // return the CC source code that was created
    std::string getMap();                               // return a map of codeword assignments, useful for configuring AWGs

    void program_start(const std::string &progName);
    void program_finish(const std::string &progName);
    void kernel_start();
    void kernel_finish(const std::string &kernelName, size_t durationInCycles);
    void bundle_start(const std::string &cmnt);
    void bundle_finish(size_t startCycle, size_t durationInCycles, bool isLastBundle);
    void comment(const std::string &c);

    // Quantum instructions
    void custom_gate(
            const std::string &iname,
            const std::vector<size_t> &qops,
            const std::vector<size_t> &cops,
            double angle, size_t startCycle, size_t durationInNs);
    void nop_gate();

    // Classical operations on kernels
    void if_start(size_t op0, const std::string &opName, size_t op1);
    void else_start(size_t op0, const std::string &opName, size_t op1);
    void for_start(const std::string &label, int iterations);
    void for_end(const std::string &label);
    void do_while_start(const std::string &label);
    void do_while_end(const std::string &label, size_t op0, const std::string &opName, size_t op1);

#if 0   // FIXME: unused and incomplete
    // Classical arithmetic instructions
    void add();
    // FIXME: etc
#endif

private:    // vars
    static const int MAX_SLOTS = 12;                            // physical maximum of CC
    static const int MAX_GROUPS = 32;                           // based on VSM, which currently has the largest number of groups

    bool verboseCode = true;                                    // output extra comments in generated code. FIXME: not yet configurable
    bool mapPreloaded = false;

    std::stringstream cccode;                                   // the code generated for the CC

    // codegen state
    std::vector<std::vector<tGroupInfo>> groupInfo;             // matrix[instrIdx][group]
    json codewordTable;                                         // codewords versus signals per instrument group
#if OPT_FEEDBACK
    json inputLutTable;                                         // input LUT usage per instrument group
#endif
    unsigned int lastEndCycle[MAX_SLOTS];

    // some JSON nodes we need access to
    const json *jsonInstrumentDefinitions;
    const json *jsonControlModes;
    const json *jsonInstruments;
    const json *jsonSignals;

    const ql::quantum_platform *platform;

#if OPT_VCD_OUTPUT
    unsigned int kernelStartTime;
    Vcd vcd;
    int vcdVarKernel;
    std::vector<int> vcdVarQubit;
    std::vector<std::vector<int>> vcdVarSignal;
    std::vector<int> vcdVarCodeword;
#endif

private:    // funcs
    // Some helpers to ease nice assembly formatting
    void emit(const char *labelOrComment, const char *instr="");

    // @param   label       must include trailing ":"
    // @param   comment     must include leading "#"
    void emit(const char *label, const char *instr, const std::string &qops, const char *comment="");

    // helpers
    void emitProgramStart();
    void padToCycle(size_t lastEndCycle, size_t startCycle, int slot, const std::string &instrumentName);
    uint32_t assignCodeword(const std::string &instrumentName, int instrIdx, int group);

    // Functions processing JSON
    void load_backend_settings();
    const json &findInstrumentDefinition(const std::string &name);
    tJsonNodeInfo findSignalDefinition(const json &instruction, const std::string &iname);

    // find instrument/group providing instructionSignalType for qubit
    tSignalInfo findSignalInfoForQubit(const std::string &instructionSignalType, size_t qubit);
}; // class

#endif  // ndef ARCH_CC_CODEGEN_CC_H
