/**
 * @file    codegen_cc.h
 * @date    201810xx
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   code generator backend for the Central Controller
 * @note    here we don't check whether the sequence of calling code generator
 *          functions is correct
 */

#ifndef QL_ARCH_CC_CODEGEN_CC_H
#define QL_ARCH_CC_CODEGEN_CC_H

// options
#define OPT_SUPPORT_STATIC_CODEWORDS    1
#define OPT_VCD_OUTPUT                  1   // output Value Change Dump file for GTKWave viewer
#define OPT_RUN_ONCE                    0   // 0=loop indefinitely (CC-light emulation)

#include "json.h"
#include "platform.h"
#if OPT_VCD_OUTPUT
 #include "vcd.h"
#endif

#include <string>
#include <cstddef>  // for size_t etc.
#ifdef _MSC_VER     // MS Visual C++ does not know about ssize_t
  #include <type_traits>
  typedef std::make_signed<size_t>::type ssize_t;
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
        size_t duration_ns;
        ssize_t readoutCop;     // NB: we use ssize_t iso size_t so we can encode 'unused' (-1)
#if OPT_SUPPORT_STATIC_CODEWORDS
        int staticCodewordOverride;
#endif
    } tGroupInfo;

    typedef struct {
        json node;
        std::string path;
    } tJsonNodeInfo;

private: // vars
    static const int MAX_SLOTS = 12;
    static const int MAX_GROUPS = 32;                           // enough for VSM

    bool verboseCode = true;                                    // output extra comments in generated code. FIXME: not yet configurable
    bool mapPreloaded = false;

    std::stringstream cccode;                                   // the code generated for the CC

    // codegen state
    std::vector<std::vector<tGroupInfo>> groupInfo;             // matrix[instrIdx][group]
    json codewordTable;                                         // codewords versus signals per instrument group
    json inputLutTable;                                         // input LUT usage per instrument group
    size_t lastStartCycle[MAX_SLOTS];

    // some JSON nodes we need access to. FIXME: use pointers/const json & for efficiency?
    json jsonBackendSettings;
    json jsonInstrumentDefinitions;
    json jsonControlModes;
    json jsonInstruments;
    json jsonSignals;

    const ql::quantum_platform *platform;

#if OPT_VCD_OUTPUT
    size_t kernelStartTime;
    Vcd vcd;
    int vcdVarKernel;
    std::vector<int> vcdVarQubit;
    std::vector<std::vector<int>> vcdVarSignal;
    std::vector<int> vcdVarCodeword;
#endif

public:
    codegen_cc() {}
    ~codegen_cc() {}

    // Generic
    void init(const ql::quantum_platform &platform);
    std::string getCode();
    std::string getMap();

    void program_start(std::string prog_name);
    void program_finish(std::string prog_name);
    void kernel_start();
    void kernel_finish(std::string kernelName, size_t duration_in_cycles);
    void bundle_start(std::string cmnt);
    void bundle_finish(size_t start_cycle, size_t duration_in_cycles, bool isLastBundle);
    void comment(std::string c);

    // Quantum instructions
    void custom_gate(std::string iname, std::vector<size_t> qops, std::vector<size_t> cops, double angle, size_t start_cycle, size_t duration_ns);
    void nop_gate();

    // Classical operations on kernels
    void if_start(size_t op0, std::string opName, size_t op1);
    void else_start(size_t op0, std::string opName, size_t op1);
    void for_start(std::string label, int iterations);
    void for_end(std::string label);
    void do_while_start(std::string label);
    void do_while_end(std::string label, size_t op0, std::string opName, size_t op1);

    // Classical arithmetic instructions
    void add();
    // FIXME: etc

private:
    // Some helpers to ease nice assembly formatting
    void emit(const char *labelOrComment, const char *instr="");

    // @param   label       must include trailing ":"
    // @param   comment     must include leading "#"
    void emit(const char *label, const char *instr, std::string qops, const char *comment="");

    // helpers
    void latencyCompensation();
    void padToCycle(size_t lastStartCycle, size_t start_cycle, int slot, std::string instrumentName);
    uint32_t assignCodeword(const std::string &instrumentName, int instrIdx, int group);

    // Functions processing JSON
    void load_backend_settings();
    const json &findInstrumentDefinition(const std::string &name);

    // find instrument/group providing instructionSignalType for qubit
    tSignalInfo findSignalInfoForQubit(std::string instructionSignalType, size_t qubit);

    tJsonNodeInfo findSignalDefinition(const json &instruction, const std::string &iname) const;
}; // class


#endif  // ndef QL_ARCH_CC_CODEGEN_CC_H
