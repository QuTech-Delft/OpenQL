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
#define OPT_VCD_OUTPUT                  1   // output Value Change Dump file for GTKWave viewer
#define OPT_RUN_ONCE                    0   // 0=loop indefinitely (CC-light emulation)
#define OPT_CALCULATE_LATENCIES         0   // fixed compensation based on instrument latencies
#define OPT_OLD_SEQBAR_SEMANTICS        1   // support old semantics of seqbar instruction

#include "json.h"
#include "platform.h"
#if OPT_VCD_OUTPUT
 #include "vcd.h"
#endif

#include <string>
#include <cstddef>  // for size_t etc.
#ifdef _MSC_VER     // MS Visual C++ does not know about ssize_t
// FIXME JvS: this #ifdef should not be necessary. libqasm shouldn't be
// #define'ing ssize_t, but should just use its own type! (so should this,
// though, for the same reason. Though this typedef is by far the nicer way
// to do it, it might still conflict with another header working around the
// same problem)
#ifndef ssize_t
  #include <type_traits>
  typedef std::make_signed<size_t>::type ssize_t;
#endif
#endif


class codegen_cc
{
private: // types
    typedef struct {
        int instrIdx;           // the index into JSON "eqasm_backend_cc/instruments" that provides the signal
        int group;              // the group within the instrument that provides the signal
    } tSignalInfo;

    typedef struct {
        std::string signalValue;
        size_t durationInNs;
        ssize_t readoutCop;     // NB: we use ssize_t iso size_t so we can encode 'unused' (-1)
#if OPT_SUPPORT_STATIC_CODEWORDS
        int staticCodewordOverride;
#endif
    } tGroupInfo;

    typedef struct {
        json node;              // a copy of the node found
        std::string path;       // path of the node, for reporting purposes
    } tJsonNodeInfo;

public:
    codegen_cc() = default;
    ~codegen_cc() = default;

    // Generic
    void init(const ql::quantum_platform &platform);
    std::string getCode();
    std::string getMap();

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

    // Classical arithmetic instructions
    void add();
    // FIXME: etc

private:    // vars
    static const int MAX_SLOTS = 12;
    static const int MAX_GROUPS = 32;                           // enough for VSM

    bool verboseCode = true;                                    // output extra comments in generated code. FIXME: not yet configurable
    bool mapPreloaded = false;

    std::stringstream cccode;                                   // the code generated for the CC

    // codegen state
    std::vector<std::vector<tGroupInfo>> groupInfo;             // matrix[instrIdx][group]
    json codewordTable;                                         // codewords versus signals per instrument group
    json inputLutTable;                                         // input LUT usage per instrument group
    size_t lastEndCycle[MAX_SLOTS];

    // some JSON nodes we need access to
    const json *jsonInstrumentDefinitions;
    const json *jsonControlModes;
    const json *jsonInstruments;
    const json *jsonSignals;

    const ql::quantum_platform *platform;

#if OPT_VCD_OUTPUT
    size_t kernelStartTime;
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
    void latencyCompensation();
    void padToCycle(size_t lastEndCycle, size_t startCycle, int slot, const std::string &instrumentName);
    uint32_t assignCodeword(const std::string &instrumentName, int instrIdx, int group);

    // Functions processing JSON
    void load_backend_settings();
    const json &findInstrumentDefinition(const std::string &name);

    // find instrument/group providing instructionSignalType for qubit
    tSignalInfo findSignalInfoForQubit(const std::string &instructionSignalType, size_t qubit);

    tJsonNodeInfo findSignalDefinition(const json &instruction, const std::string &iname) const;
}; // class

#endif  // ndef ARCH_CC_CODEGEN_CC_H
