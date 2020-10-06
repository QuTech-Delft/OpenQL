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
#define OPT_SUPPORT_STATIC_CODEWORDS    1   // support (currently: require) static codewords, instead of allocating them on demand
#define OPT_STATIC_CODEWORDS_ARRAYS     1   // JSON field static_codeword_override is an array with one element per qubit parameter
#define OPT_VECTOR_MODE                 0   // 1=generate single code word for all output groups together (requires codewords allocated by backend)
#define OPT_RUN_ONCE                    0   // 0=loop indefinitely (CC-light emulation), 1=run once (preferred, but breaks compatibility)
#define OPT_CALCULATE_LATENCIES         0   // fixed compensation based on instrument latencies. Not actively maintained
#define OPT_OLD_SEQBAR_SEMANTICS        0   // support old semantics of seqbar instruction. Will be deprecated
#define OPT_FEEDBACK                    0   // feedback support. Coming feature

#include "settings_cc.h"
#include "vcd_cc.h"
#include "platform.h"

#include <string>


class codegen_cc
{
private: // types
    typedef struct {
        std::string signalValue;
        unsigned int durationInNs;
#if OPT_FEEDBACK
        int readoutCop;                 // classic operand for readout. NB: we encode 'unused' as -1
#endif
#if OPT_SUPPORT_STATIC_CODEWORDS
        int staticCodewordOverride;
#endif
    } tBundleInfo;                      // information for an instrument group (of channels), for a single instruction
// FIXME: rename tInstrInfo, store gate as annotation, move to class cc:IR?

public:
    codegen_cc() = default;
    ~codegen_cc() = default;

    // Generic
    void init(const ql::quantum_platform &platform);
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
            const std::vector<size_t> &qops,
            const std::vector<size_t> &cops,
            double angle, size_t startCycle, size_t durationInNs);  // FIXME: here we have duration in ns, elsewhere in cycles
    void nopGate();

    void comment(const std::string &c);

    // Classical operations on kernels
    void ifStart(size_t op0, const std::string &opName, size_t op1);
    void elseStart(size_t op0, const std::string &opName, size_t op1);
    void forStart(const std::string &label, int iterations);
    void forEnd(const std::string &label);
    void doWhileStart(const std::string &label);
    void doWhileEnd(const std::string &label, size_t op0, const std::string &opName, size_t op1);

private:    // vars
    static const int MAX_SLOTS = 12;                            // physical maximum of CC
    static const int MAX_GROUPS = 32;                           // based on VSM, which currently has the largest number of groups

    const ql::quantum_platform *platform;                       // remind platform
    settings_cc settings;                                       // handling of JSON settings
    vcd_cc vcd;                                                 // handling of VCD file output

    bool verboseCode = true;                                    // option to output extra comments in generated code. FIXME: not yet configurable
    bool mapPreloaded = false;

    std::stringstream codeSection;                              // the code generated
#if OPT_FEEDBACK
    std::stringstream datapathSection;                          // the data path configuration generated
#endif

    // codegen state
    unsigned int lastEndCycle[MAX_SLOTS];
    std::vector<std::vector<tBundleInfo>> bundleInfo;           // matrix[instrIdx][group]
    json codewordTable;                                         // codewords versus signals per instrument group
#if OPT_FEEDBACK
    json inputLutTable;                                         // input LUT usage per instrument group
#endif


private:    // funcs
    // Some helpers to ease nice assembly formatting
    void emit(const char *labelOrComment, const char *instr="");
    void emit(const char *label, const char *instr, const std::string &qops, const char *comment="");

    // helpers
    void emitProgramStart();
    void padToCycle(size_t lastEndCycle, size_t startCycle, int slot, const std::string &instrumentName);
    uint32_t assignCodeword(const std::string &instrumentName, int instrIdx, int group);
}; // class

#endif  // ndef ARCH_CC_CODEGEN_CC_H
