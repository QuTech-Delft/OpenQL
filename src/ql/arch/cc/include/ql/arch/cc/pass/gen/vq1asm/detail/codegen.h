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

#include "ql/arch/cc/pass/gen/vq1asm/detail/operands.h"
#include "ql/arch/cc/pass/gen/vq1asm/detail/types.h"
#include "ql/arch/cc/pass/gen/vq1asm/detail/options.h"
#include "ql/arch/cc/pass/gen/vq1asm/detail/bundle_info.h"
#include "ql/arch/cc/pass/gen/vq1asm/detail/codesection.h"
#include "ql/arch/cc/pass/gen/vq1asm/detail/functions.h"
#include "ql/arch/cc/pass/gen/vq1asm/detail/datapath.h"
#include "ql/arch/cc/pass/gen/vq1asm/detail/settings.h"
#include "ql/arch/cc/pass/gen/vq1asm/detail/vcd.h"

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

    /**
     * Handle expressions.
     *
     * To understand how cQASM functions end up in the IR, please note that functions are handled during analyzing
     * cQASM, see 'AnalyzerHelper::analyze_function()'.
     *
     * A default set of functions that only handle constant arguments is provided by libqasm, see
     * 'register_into(resolver::FunctionTable &table)'. These functions add a constant node to the IR when called (and
     * fail if the arguments are not constant)
     *
     * Some of these are overridden by OpenQL to allow use of non-constant arguments. This is a 2 step process, where
     * 'convert_old_to_new(const compat::PlatformRef &old)' adds functions to ir->platform using 'add_function_type',
     * and 'ql::ir::cqasm:read()' then walks 'ir->platform->functions' and adds the functions using
     * 'register_function()'. These functions add a 'cqv::Function' node to the IR (even if the arguments are constant,
     * so overriding a function defeats libqasm's constant removal for that function).
     */

    /**
     * Perform the code generation for a SetInstruction.
     */
    void handle_set_instruction(const ir::SetInstruction &set, const Str &descr);

    /**
     * Perform the code generation for an expression. The expression should act as a condition for structured control,
     * parameter 'label_if_false' must contain the label to jump to if the expression evaluates as false.
     */
    void handle_expression(const ir::ExpressionRef &expression, const Str &label_if_false, const Str &descr);   // FIXME: private?


protected:
    // FIXME: split off emitting into separate class
//    friend class Functions;                                     // needs access to emit*()


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
    CodeSection cs;                                             // handling of CC code
    Datapath dp;                                                // handling of CC datapath
    Functions fncs;                                             // handling of functions within expressions
    Vcd vcd;                                                    // handling of VCD file output

    Bool mapPreloaded = false;                                  // flag whether we have a preloaded map

    // codegen state, global(program) scope
    Json codewordTable;                                         // codewords versus signals per instrument group
    Json measTable;                                             // measurement table, to assist downstream software in retrieving measurements
    Json shotsTable;                                            // nr of shots per instrument, companion to measTable

    // codegen state, block(kernel) scope
    UInt lastEndCycle[MAX_INSTRS];                              // vector[instrIdx], maintain where we got per slot
    Int depth = 0;                                              // depth of current block, used for indentation of comments

    // codegen state, bundle scope
    Vec<Vec<BundleInfo>> bundleInfo;                            // matrix[instrIdx][group]


private:    // funcs
    // code generation helpers
    void showCodeSoFar() { cs.showCodeSoFar(); };
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
}; // class

} // namespace detail
} // namespace vq1asm
} // namespace gen
} // namespace pass
} // namespace cc
} // namespace arch
} // namespace ql
