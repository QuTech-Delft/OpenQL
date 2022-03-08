/**
 * @file    arch/cc/pass/gen/vq1asm/detail/codegen.h
 * @date    201810xx
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   code generator backend for the Central Controller
 * @note    here we don't check whether the sequence of calling code generator
 *          functions is correct
 */

#pragma once

#include "ql/ir/ir.h"

#include "operands.h"
#include "types.h"
#include "options.h"
#include "bundle_info.h"
#include "datapath.h"
#include "settings.h"
#include "vcd.h"

namespace ql {
namespace arch {
namespace cc {
namespace pass {
namespace gen {
namespace vq1asm {
namespace detail {


// FIXME: split aff the actual code generation, and simplify support for architectures that are similar
class Codegen {
public: //  functions
    Codegen(const ir::Ref &ir, const OptionsRef &options);
    ~Codegen() = default;

    // Generic
    Str get_program();                           // return the CC source code that was created
    Str get_map();                               // return a map of codeword assignments, useful for configuring AWGs

    // Compile support

    /*
     * 'Program' level functions
     */
    void program_start(const Str &progName);
    void program_finish(const Str &progName);

    /*
     * 'Block' level functions
     * (fka 'Kernel', this name stays relevant as it is used by the API)
     */
    void block_start(const Str &block_name, Int depth);
    void block_finish(const Str &block_name, UInt durationInCycles, Int depth);

    /*
     * 'Bundle' level functions.
     * Although the new IR no longer organizes instructions in Bundles, we still need to process them as such, i.e.
     * evaluate all instructions issued in the same cycle together.
     *
     * Our strategy is to first process all CustomInstruction's in a bundle, storing the relevant information in
     * BundleInfo. Then, when all work for a bundle has been collected, we generate code in bundle_finish
     */

    /**
     * Clear bundleInfo, which maintains the work that needs to be performed for bundle
     */
    void bundle_start(const Str &cmnt);

    /**
     * Generate code for bundle from information collected in bundleInfo (which may be empty if no custom gates are
     * present in bundle)
     */
    void bundle_finish(UInt startCycle, UInt durationInCycles, Bool isLastBundle);

    /**
     * Collect information from CustomInstruction (single/two/N qubit gate, including readout. FKA as gate). Translates
     * 'gate' representation to 'waveform' representation (BundleInfo) and maps qubits to instruments & group.
     *
     * Does not deal with the control mode and digital interface of the instrument, since we first need to collect all
     * work per instrument
     */
    void custom_instruction(const ir::CustomInstruction &custom);

    // Structured control flow
    void if_elif(const ir::ExpressionRef &condition, const Str &label, Int branch);
    void if_otherwise(const Str &label, Int branch);
    void if_end(const Str &label);
    void foreach_start(const ir::Reference &lhs, const ir::IntLiteral &frm, const Str &label);
    void foreach_end(const ir::Reference &lhs, const ir::IntLiteral &frm, const ir::IntLiteral &to, const Str &label);
    void repeat(const Str &label);
    void until(const ir::ExpressionRef &condition, const Str &label);
    void for_start(utils::Maybe<ir::SetInstruction> &initialize, const ir::ExpressionRef &condition, const Str &label);
    void for_end(utils::Maybe<ir::SetInstruction> &update, const Str &label);
    void do_break(const Str &label);
    void do_continue(const Str &label);

    void comment(const Str &c);

    // new IR expressions
    void handle_set_instruction(const ir::SetInstruction &set, const Str &descr);
    void handle_expression(const ir::ExpressionRef &expression, const Str &label_if_false, const Str &descr);   // FIXME: private?

private:    // types
    /**
     * Code generation info for single instrument.
     */
    struct CodeGenInfo {
        // output related
        Bool instrHasOutput;
        tDigital digOut;                                        // the digital output value sent over the instrument interface
        UInt instrMaxDurationInCycles;                          // maximum duration over groups that are used, one instrument

        // measurement related
        Vec<UInt> measQubits;                                   // the qubits measured

        // feedback related
        MeasResultRealTimeMap measResultRealTimeMap;
        CondGateMap condGateMap;

        // info copied from tInstrumentInfo
        Str instrumentName;
        Int slot;
    };

    /**
     * Code generation info for all instruments.
     */
    using CodeGenMap = Map<Int, CodeGenInfo>;                   // NB: key is instrument index


private:    // vars
    static const Int MAX_SLOTS = 12;                            // physical maximum of CC
    static const Int MAX_GROUPS = 32;                           // based on VSM, which currently has the largest number of groups

    ir::Ref ir;                                                 // remind ir (and thus platform too)
    OptionsRef options;                                         // remind options
    OperandContext operandContext;                              // context for Operand processing
    Settings settings;                                          // handling of JSON settings
    Datapath dp;                                                // handling of CC datapath
    Vcd vcd;                                                    // handling of VCD file output

    Bool mapPreloaded = false;                                  // flag whether we have a preloaded map

    // codegen state, global(program) scope
    StrStrm codeSection;                                        // the code generated
    Json codewordTable;                                         // codewords versus signals per instrument group
    Json measTable;                                             // measurement table, to assist downstream software in retrieving measurements
    Json shotsTable;                                            // nr of shots per instrument, companion to measTable

    // codegen state, block(kernel) scope
    UInt lastEndCycle[MAX_INSTRS];                              // vector[instrIdx], maintain where we got per slot
    Int depth = 0;                                              // depth of current block, used for indentation of comments

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
    void emitMeasRsltRealTime(const MeasResultRealTimeMap &measResultRealTimeMap, UInt instrIdx, UInt startCycle, Int slot, const Str &instrumentName);
    void emitOutput(const CondGateMap &condGateMap, tDigital digOut, UInt instrMaxDurationInCycles, UInt instrIdx, UInt startCycle, Int slot, const Str &instrumentName);
    void emitPadToCycle(UInt instrIdx, UInt startCycle, Int slot, const Str &instrumentName);

    // generic helpers

    /*
     * Build a map of CodeGenInfo with the information required for code generation, based on BundleInfo records for all
     * instrument groups
     */
    CodeGenMap collectCodeGenInfo(UInt startCycle, UInt durationInCycles);
#if !OPT_SUPPORT_STATIC_CODEWORDS
    tCodeword assignCodeword(const Str &instrumentName, Int instrIdx, Int group);
#endif

    // expression helpers
    Int creg2reg(const ir::Reference &ref);
    void do_handle_expression(const ir::ExpressionRef &expression, const ir::ExpressionRef &lhs, const Str &label_if_false="", const Str &descr="");
}; // class

} // namespace detail
} // namespace vq1asm
} // namespace gen
} // namespace pass
} // namespace cc
} // namespace arch
} // namespace ql
