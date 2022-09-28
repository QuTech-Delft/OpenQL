/**
 * @file    arch/cc/pass/gen/vq1asm/detail/codegen.cc
 * @date    201810xx
 * @author  Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief   code generator backend for the Central Controller
 * @note    here we don't check whether the sequence of calling code generator
 *          functions is correct
 */

#include "ql/arch/cc/pass/gen/vq1asm/detail/codegen.h"
#include "ql/arch/cc/pass/gen/vq1asm/detail/codesection.h"

#include "ql/version.h"
#include "ql/com/options.h"
#include "ql/ir/describe.h"

#include <iosfwd>

namespace ql {
namespace arch {
namespace cc {
namespace pass {
namespace gen {
namespace vq1asm {
namespace detail {

using namespace utils;

// helpers for label generation.
static Str to_start(const Str &base) { return base + "_start"; };
static Str to_end(const Str &base) { return base + "_end"; };
static Str to_ifbranch(const Str &base, Int branch) { return QL_SS2S(base << "_" << branch); }
static Str as_label(const Str &label) { return label + ":"; }

/**
 * Decode the expression for a conditional instruction into the old format as used for the API. Eventually this will have
 * to be changed, but as long as the CC can handle expressions with 2 variables only this covers all we need.
 */
// FIXME: move to datapath
static tInstructionCondition decode_condition(const OperandContext &operandContext, const ir::ExpressionRef &condition) {
    ConditionType cond_type;
    utils::Vec<utils::UInt> cond_operands;

    try {
        if (auto blit = condition->as_bit_literal()) {
            if (blit->value) {
                cond_type = ConditionType::ALWAYS;
            } else {
                cond_type = ConditionType::NEVER;
            }
        } else if (auto cond = condition->as_reference()) {
            // FIXME: add is_breg_reference()
            cond_operands.push_back(operandContext.convert_breg_reference(*cond));
            cond_type = ConditionType::UNARY;
        } else if (auto fn = condition->as_function_call()) {
            if (
                fn->function_type->name == "operator!" ||
                fn->function_type->name == "operator~"
            ) {
                CHECK_COMPAT(fn->operands.size() == 1, "expected one operand");
                if (auto op0 = fn->operands[0]->as_reference()) {
                    cond_operands.push_back(operandContext.convert_breg_reference(*op0));
                    cond_type = ConditionType::NOT;
                } else if (auto fn2 = fn->operands[0]->as_function_call()) {
                    CHECK_COMPAT(fn2->operands.size() == 2, "expected 2 operands");
                    cond_operands.push_back(operandContext.convert_breg_reference(fn2->operands[0]));
                    cond_operands.push_back(operandContext.convert_breg_reference(fn2->operands[1]));
                    if (
                        fn2->function_type->name == "operator&" ||
                        fn2->function_type->name == "operator&&"
                    ) {
                        cond_type = ConditionType::NAND;
                    } else if (
                        fn2->function_type->name == "operator|" ||
                        fn2->function_type->name == "operator||"
                    ) {
                        cond_type = ConditionType::NOR;
                    } else if (
                        fn2->function_type->name == "operator^" ||
                        fn2->function_type->name == "operator^^" ||
                        fn2->function_type->name == "operator!="
                    ) {
                        cond_type = ConditionType::NXOR;
                    } else if (
                        fn2->function_type->name == "operator=="
                    ) {
                        cond_type = ConditionType::XOR;
                    } else {
                        QL_ICE("unsupported gate condition");
                    }
                } else {
                    QL_ICE("unsupported gate condition");
                }
#if OPT_CC_USER_FUNCTIONS
            // FIXME: note that is only here to allow playing around with function calls as condition. Real support
            //  requires a redesign
            } else if (
                fn->function_type->name == "rnd" ||
                fn->function_type->name == "rnd_seed"
            ) {
                cond_type = ConditionType::ALWAYS;
                QL_WOUT("FIXME: instruction condition function not yet handled: " + fn->function_type->name);
#endif
            } else {
                CHECK_COMPAT(fn->operands.size() == 2, "expected 2 operands");
                cond_operands.push_back(operandContext.convert_breg_reference(fn->operands[0]));
                cond_operands.push_back(operandContext.convert_breg_reference(fn->operands[1]));
                if (
                    fn->function_type->name == "operator&" ||
                    fn->function_type->name == "operator&&"
                ) {
                    cond_type = ConditionType::AND;
                } else if (
                    fn->function_type->name == "operator|" ||
                    fn->function_type->name == "operator||"
                ) {
                    cond_type = ConditionType::OR;
                } else if (
                    fn->function_type->name == "operator^" ||
                    fn->function_type->name == "operator^^" ||
                    fn->function_type->name == "operator!="
                ) {
                    cond_type = ConditionType::XOR;
                } else if (
                    fn->function_type->name == "operator=="
                ) {
                    cond_type = ConditionType::NXOR;
                } else {
                    QL_ICE("unsupported condition function");
                }
            }
        } else {
            QL_ICE("unsupported condition expression");
        }
    } catch (utils::Exception &e) {
        e.add_context("in gate condition '" + ir::describe(condition) + "'", true);
        throw;
    }
    return {cond_type, cond_operands, ir::describe(condition)};
}


/*
 * Return type for calcGroupDigOut()
 */
typedef struct {
    tDigital groupDigOut;   // codeword/mask fragment for this group
    Str comment;            // comment for instruction stream
} CalcGroupDigOut;

/*
 * Calculate the digital output for a single instrument group.
 * Static helper function for bundle_finish()
 */
static CalcGroupDigOut calcGroupDigOut(
        UInt instrIdx,
        UInt group,
        UInt nrGroups,
        const Settings::InstrumentControl &ic,
        tCodeword staticCodewordOverride
) {
    CalcGroupDigOut ret{0, ""};

    // determine control mode group FIXME: more explanation
    Int controlModeGroup = -1;
    if (ic.controlModeGroupCnt == 0) {
        QL_JSON_ERROR("'control_bits' not defined or empty in 'control_modes/" << ic.refControlMode <<"'");
#if OPT_VECTOR_MODE
    } else if (ic.controlModeGroupCnt == 1) {                   // vector mode: group addresses channel within vector
        controlModeGroup = 0;
#endif
    } else if (group < ic.controlModeGroupCnt) {                // normal mode: group selects control group
        controlModeGroup = group;
    } else {
        // NB: this actually an error in program logic
        QL_JSON_ERROR(
            "instrument '" << ic.ii.instrumentName
            << "' uses " << nrGroups
            << " groups, but control mode '" << ic.refControlMode
            << "' only defines " << ic.controlModeGroupCnt
            << " groups in 'control_bits'"
        );
    }

    // get number of control bits for group
    const Json &groupControlBits = ic.controlMode["control_bits"][controlModeGroup];    // NB: tests above guarantee existence
    QL_DOUT(
        "instrumentName=" << ic.ii.instrumentName
        << ", slot=" << ic.ii.slot
        << ", control mode group=" << controlModeGroup
        << ", group control bits: " << groupControlBits
    );
    UInt nrGroupControlBits = groupControlBits.size();

    // calculate digital output for group
    if (nrGroupControlBits == 1) {       // single bit, implying this is a mask (not code word)
        ret.groupDigOut |= 1ul << groupControlBits[0].get<Int>();     // NB: we assume the mask is active high, which is correct for VSM and UHF-QC
        // FIXME: check controlModeGroup vs group
    } else if (nrGroupControlBits > 1) {                 // > 1 bit, implying code word
#if OPT_VECTOR_MODE
        //  allow single code word for vector of groups. FIXME: requires looking at all sd.signal before assigning code word
        if (group != controlModeGroup) {
            // FIXME: unfinished work on vector mode
        }
#endif

        // find or assign code word
        tCodeword codeword = 0;
        Bool codewordOverriden = false;
#if OPT_SUPPORT_STATIC_CODEWORDS
        codeword = staticCodewordOverride;
        codewordOverriden = true;
#else
        codeword = assignCodeword(ic.ii.instrumentName, instrIdx, group);
#endif

        // convert codeword to digOut
        for (size_t idx=0; idx<nrGroupControlBits; idx++) {
            Int codeWordBit = nrGroupControlBits - 1 - idx;    // NB: groupControlBits defines MSB..LSB
            if (codeword & (1ul << codeWordBit)) {
                ret.groupDigOut |= 1ul << groupControlBits[idx].get<Int>();
            }
        }

        ret.comment = QL_SS2S(
            "  # slot=" << ic.ii.slot
            << ", instrument='" << ic.ii.instrumentName << "'"
            << ", group=" << group
            << ": codeword=" << codeword
            << std::string(codewordOverriden ? " (static override)" : "")
            << ": groupDigOut=0x" << std::hex << std::setfill('0') << std::setw(8) << ret.groupDigOut
        );
    } else {    // nrGroupControlBits < 1
        QL_JSON_ERROR(
            "key 'control_bits' empty for group " << controlModeGroup
            << " on instrument '" << ic.ii.instrumentName << "'"
        );
    }

    // add trigger to digOut
    UInt nrTriggerBits = ic.controlMode["trigger_bits"].size();
    if (nrTriggerBits == 0) {                                   // no trigger
        // do nothing
    } else if (nrTriggerBits == 1) {                            // single trigger for all groups (NB: will possibly assigned multiple times)
        ret.groupDigOut |= 1ul << ic.controlMode["trigger_bits"][0].get<Int>();
#if 1    // FIXME: hotfix for QWG, implement properly
    } else if(nrTriggerBits == 2) {
        ret.groupDigOut |= 1ul << ic.controlMode["trigger_bits"][0].get<Int>();
        ret.groupDigOut |= 1ul << ic.controlMode["trigger_bits"][1].get<Int>();
#endif
#if 1   // FIXME: trigger per group
    } else if(nrTriggerBits == nrGroups) {                      // trigger per group
        ret.groupDigOut |= 1ul << ic.controlMode["trigger_bits"][group].get<Int>();
#endif
    } else {
        QL_JSON_ERROR(
            "instrument '" << ic.ii.instrumentName
            << "' uses " << nrGroups
            << " groups, but control mode '" << ic.refControlMode
            << "' defines " << nrTriggerBits
            << " trigger bits in 'trigger_bits' (must be 1 or #groups)"
        );
    }

    return ret;
}

/************************************************************************\
| Class Codegen
\************************************************************************/

Codegen::Codegen(const ir::Ref &ir, const OptionsRef &options)
    : ir(ir)
    , options(options)
    , operandContext(ir)
    , cs(operandContext)
    , fncs(operandContext, dp, cs)
{
    // NB: a new Backend is instantiated per call to compile, and
    // as a result also a Codegen, so we don't need to cleanup

    settings.loadBackendSettings(ir->platform);

    // optionally preload codewordTable
    Str map_input_file = options->map_input_file;
    if (!map_input_file.empty()) {
        QL_DOUT("loading map_input_file='" << map_input_file << "'");
        Json map = load_json(map_input_file);
        codewordTable = map["codewords"]["data"];      // FIXME: use json_get
        mapPreloaded = true;
    }

    // show instruments that can produce real-time measurement results
    for (UInt instrIdx = 0; instrIdx < settings.getInstrumentsSize(); instrIdx++) {
        const Settings::InstrumentControl ic = settings.getInstrumentControl(instrIdx);
        if (QL_JSON_EXISTS(ic.controlMode, "result_bits")) {  // this instrument mode produces results (i.e. it is a measurement device)
            QL_IOUT("instrument '" << ic.ii.instrumentName << "' (index " << instrIdx << ") can produce real-time measurement results");
        }
    }
}

/************************************************************************\
| Generic
\************************************************************************/

Str Codegen::get_program() {
    return cs.getCodeSection() + dp.getDatapathSection();
}

Str Codegen::get_map() {
    Json map;

    map["openql"]["version"] = OPENQL_VERSION_STRING;
    map["openql"]["backend"] = "cc";
    map["openql"]["backend-version"] = CC_BACKEND_VERSION_STRING;

    map["codewords"]["version"] = 1;
    map["codewords"]["data"] = codewordTable;

    map["measurements"]["version"] = 2;
    map["measurements"]["data"] = measTable;
    map["measurements"]["nr-shots"] = shotsTable;

    return QL_SS2S(std::setw(4) << map << std::endl);
}


void Codegen::program_start(const Str &progName) {
    emitProgramStart(progName);

    dp.programStart();

    // Determine number of qubits.
    utils::UInt num_qubits;
    if (ir->platform->qubits->shape.size() == 1) {
        num_qubits = ir->platform->qubits->shape[0];
    } else {
        QL_USER_ERROR("main qubit register has wrong dimensionality");
    };

    // Get cycle time from old Platform (NB: in new Platform, all durations are in quantum cycles, not ns).
    auto &json = ir->platform->data.data;
    QL_JSON_ASSERT(json, "hardware_settings", "hardware_settings");
    auto hardware_settings = json["hardware_settings"];
    QL_JSON_ASSERT(hardware_settings, "cycle_time", "hardware_settings/cycle_time");
    UInt cycle_time = hardware_settings["cycle_time"];

    vcd.programStart(num_qubits, cycle_time, MAX_GROUPS, settings);
}


void Codegen::program_finish(const Str &progName) {
    emitProgramFinish();

    dp.programFinish();

    vcd.programFinish(options->output_prefix + ".vcd");
}


void Codegen::block_start(const Str &block_name, Int depth) {
    this->depth = depth;
    if(depth == 0) {
        comment("");    // white space before top level block
    }
    comment("### Block: '" + block_name + "'");
    zero(lastEndCycle); // NB: new IR starts counting at zero
}

void Codegen::block_finish(const Str &block_name, UInt durationInCycles, Int depth) {
    comment("### Block end: '" + block_name + "'");
    vcd.kernelFinish(block_name, durationInCycles);

    // unindent, unless at top (in which case nothing follows)
    this->depth = depth>0 ? depth-1 : 0;
}


void Codegen::bundle_start(const Str &cmnt) {
    // create ragged 'matrix' of BundleInfo with proper vector size per instrument
    bundleInfo.clear();
    BundleInfo empty;
    for (UInt instrIdx = 0; instrIdx < settings.getInstrumentsSize(); instrIdx++) {
        const Settings::InstrumentControl ic = settings.getInstrumentControl(instrIdx);
        bundleInfo.emplace_back(
            ic.controlModeGroupCnt,     // one BundleInfo per group in the control mode selected for instrument
            empty                       // empty BundleInfo
        );
    }

    // generate source code comments
    comment(cmnt);
    dp.comment(cmnt, options->verbose);      // FIXME: comment is not fully appropriate, but at least allows matching with .CODE section
}


Codegen::CodeGenMap Codegen::collectCodeGenInfo(
    UInt startCycle,
    UInt durationInCycles
) {
    CodeGenMap codeGenMap;

    // iterate over instruments
    for (UInt instrIdx = 0; instrIdx < settings.getInstrumentsSize(); instrIdx++) {
        // get control info from instrument settings
        const Settings::InstrumentControl ic = settings.getInstrumentControl(instrIdx);
        if (ic.ii.slot >= MAX_SLOTS) {
            QL_JSON_ERROR(
                "illegal slot " << ic.ii.slot
                << " on instrument '" << ic.ii.instrumentName
            );
        }

        /************************************************************************\
        | collect code generation info for an instrument, based on BundleInfo of
        | all groups of that instrument
        \************************************************************************/

        // FIXME: the term 'group' is used in a diffused way: 1) index of signal vectors, 2) controlModeGroup

        CodeGenInfo codeGenInfo = {false};

        // remind information needed for code generation
        codeGenInfo.instrumentName = ic.ii.instrumentName;
        codeGenInfo.slot = ic.ii.slot;

        // now collect code generation info from all groups of instrument
        UInt nrGroups = bundleInfo[instrIdx].size();
        for (UInt group = 0; group < nrGroups; group++) {
            const BundleInfo &bi = bundleInfo[instrIdx][group];           // shorthand

            // handle output
            if (!bi.signalValue.empty()) {                         // signal defined, i.e.: we need to output something
                // compute maximum duration over all groups
                if (bi.durationInCycles > codeGenInfo.instrMaxDurationInCycles) {
                    codeGenInfo.instrMaxDurationInCycles = bi.durationInCycles;
                }

                CalcGroupDigOut gdo = calcGroupDigOut(instrIdx, group, nrGroups, ic, bi.staticCodewordOverride);
                codeGenInfo.digOut |= gdo.groupDigOut;
                comment(gdo.comment);

                // save codeword mapping
                // FIXME: store JSON signal iso string, store codeword as key
                codewordTable[ic.ii.instrumentName][group] = {bi.staticCodewordOverride, bi.signalValue};   // NB: structure created on demand

                // conditional gates
                // store condition and groupDigOut in condMap, if all groups are unconditional we use old scheme, otherwise
                // datapath is configured to generate proper digital output
                if (bi.instructionCondition.cond_type == ConditionType::ALWAYS || ic.ii.forceCondGatesOn) {
                    // nothing to do, just use digOut
                } else {    // other conditions, including cond_never
                    // remind mapping for setting PL
                    codeGenInfo.condGateMap.emplace(group, CondGateInfo{bi.instructionCondition, gdo.groupDigOut});
                }

                vcd.bundleFinishGroup(startCycle, bi.durationInCycles, gdo.groupDigOut, bi.signalValue, instrIdx, group);

                codeGenInfo.instrHasOutput = true;
            } // if(signal defined)

            // handle measurements (for which we'll generate output allowing downstream software to retrieve them).
            // Note that we do not look at the code path leading to a measurement, results are not very useful if
            // any measurements are within conditional code paths
            if (bi.isMeasure) {
                codeGenInfo.measQubits.push_back(bi.measQubit);
                // NB: the association of qubit to correlator number is up to the downstream software (both the DIO bit
                // and the index of the qubit in the instrument definition provide no explicit clue)
            }

            // handle real-time measurement results (i.e. when necessary, create measResultRealTimeMap entry
            // NB: we allow for instruments that only perform the input side of readout, without signal generation by the
            // same instrument.
            if (bi.isMeasRsltRealTime) {
                UInt resultBit = Settings::getResultBit(ic, group);

#if 0    // FIXME: partly redundant, but partly useful
                // get our qubit
                const Json qubits = json_get<const Json>(*ic.ii.instrument, "qubits", ic.ii.instrumentName);   // NB: json_get<const Json&> unavailable
                UInt qubitGroupCnt = qubits.size();                                  // NB: JSON key qubits is a 'matrix' of [groups*qubits]
                if (group >= qubitGroupCnt) {    // FIXME: also tested in settings_cc::findSignalInfoForQubit
                    QL_JSON_ERROR("group " << group << " not defined in '" << ic.ii.instrumentName << "/qubits'");
                }
                const Json qubitsOfGroup = qubits[group];
                if (qubitsOfGroup.size() != 1) {    // FIXME: not tested elsewhere
                    QL_JSON_ERROR("group " << group << " of '" << ic.ii.instrumentName << "/qubits' should define 1 qubit, not " << qubitsOfGroup.size());
                }
                Int qubit = qubitsOfGroup[0].get<Int>();
                if (bi.readoutQubit != qubit) {              // this instrument group handles requested qubit. FIXME: inherently true
                    QL_ICE("inconsistency FIXME");
                };
#endif
                // allocate SM bit for classic operand
                UInt smBit = dp.allocateSmBit(bi.breg_operand, instrIdx);

                // remind mapping of bit -> smBit for setting MUX
                codeGenInfo.measResultRealTimeMap.emplace(group, MeasResultRealTimeInfo{smBit, resultBit, bi.describe});

                // FIXME: also generate VCD
            }

        } // for(group)
        codeGenMap.set(instrIdx) = codeGenInfo;
     } // for(instrIdx)
     return codeGenMap;
}


// bundle_finish: see 'strategy' above
void Codegen::bundle_finish(
    UInt startCycle,
    UInt durationInCycles,
    Bool isLastBundle
) {
    // collect info for all instruments
    CodeGenMap codeGenMap = collectCodeGenInfo(startCycle, durationInCycles);

    // compute stuff requiring overview over all instruments:
    // FIXME: add:
    // - DSM used, for seq_inv_sm

    // determine whether bundle has any real-time measurement results
    Bool bundleHasMeasRsltRealTime = false;
    for (const auto &codeGenInfo : codeGenMap) {
        if (!codeGenInfo.second.measResultRealTimeMap.empty()) {
            bundleHasMeasRsltRealTime = true;
            // FIXME: calc min and max SM address used
            //  unsigned int smAddr = datapath_cc::getMuxSmAddr(feedbackMap);
        }
    }

    // turn code generation info collected above into actual code
    Json measTableEntry;    // entry for measTable
    for (UInt instrIdx = 0; instrIdx < settings.getInstrumentsSize(); instrIdx++) {
        CodeGenInfo codeGenInfo = codeGenMap.at(instrIdx);

        if (isLastBundle && instrIdx == 0) {
            comment(" # last bundle of kernel, will pad outputs to match durations");
        }

        // generate code for instrument output
        if (codeGenInfo.instrHasOutput) {
            emitOutput(
                codeGenInfo.condGateMap,
                codeGenInfo.digOut,
                codeGenInfo.instrMaxDurationInCycles,
                instrIdx,
                startCycle,
                codeGenInfo.slot,
                codeGenInfo.instrumentName
            );
        } else {    // !instrHasOutput
            // nothing to do, we delay emitting till a slot is used or kernel finishes (i.e. isLastBundle just below)
        }

        // handle measurement
        if(!codeGenInfo.measQubits.empty()) {
            // save measurements
            auto qubits = codeGenInfo.measQubits;
            std::sort(qubits.begin(), qubits.end());    // not strictly required, but improves readability
            measTableEntry[codeGenInfo.instrumentName] = qubits;

            // update shots per instrument
            if(!QL_JSON_EXISTS(shotsTable, codeGenInfo.instrumentName)) {
                shotsTable[codeGenInfo.instrumentName] = 1; // first shot
            } else {
#if 0   // FIXME: fails
                QL_WOUT("shotsTable[" << codeGenInfo.instrumentName << "] was " << shotsTable[codeGenInfo.instrumentName]);
                shotsTable[codeGenInfo.instrumentName] += 1;
                QL_WOUT("shotsTable[" << codeGenInfo.instrumentName << "] is " << shotsTable[codeGenInfo.instrumentName]);
#else
                Int shots = shotsTable[codeGenInfo.instrumentName];
                shotsTable[codeGenInfo.instrumentName] = shots+1;
#endif
            }
        }

        if (bundleHasMeasRsltRealTime) {
            emitMeasRsltRealTime(
                codeGenInfo.measResultRealTimeMap,
                instrIdx,
                startCycle,
                codeGenInfo.slot,
                codeGenInfo.instrumentName
            );
        }

        // for last bundle, pad end of bundle to align durations
        if (isLastBundle) {
            emitPadToCycle(
                instrIdx, startCycle + durationInCycles,
                codeGenInfo.slot,
                codeGenInfo.instrumentName
            );        // FIXME: use instrMaxDurationInCycles and/or check consistency
        }

        vcd.bundleFinish(
            startCycle,
            codeGenInfo.digOut,
            codeGenInfo.instrMaxDurationInCycles,
            instrIdx
        );    // FIXME: conditional gates, etc

    } // for(instrIdx)

    // save measurements if present
    if(!measTableEntry.empty()) {
        measTable.push_back(measTableEntry);
    }

    comment("");    // blank line to separate bundles
}

/************************************************************************\
| Quantum instructions
\************************************************************************/

void Codegen::custom_instruction(const ir::CustomInstruction &custom) {
    Operands ops;

    // Handle the template operands for the instruction_type we got. Note that these are empty if that is a 'root'
    // InstructionType, and only contains data if it is one of the specializations (see ir.gen.h)
    // FIXME: check for existing decompositions (which should have been performed already by an upstream pass)
    /*
     FIXME: these are operands that match a specialized instruction definition, e.g. "cz q0,q10"
     FIXME: these are not handled below, so things fail if we have such definitions
    cQASM "cz q[0],q[10]" with JSON "cz q0,q10" results in:
    [OPENQL] /Volumes/Data/shared/GIT/OpenQL/src/ql/arch/cc/pass/gen/vq1asm/detail/backend.cc:152 custom instruction: name=cz
    [OPENQL] /Volumes/Data/shared/GIT/OpenQL/src/ql/arch/cc/pass/gen/vq1asm/detail/codegen.cc:814 found template_operands: JSON = {"cc":{"signal":[],"static_codeword_override":[0]},"duration":40}
    [OPENQL] /Volumes/Data/shared/GIT/OpenQL/src/ql/arch/cc/pass/gen/vq1asm/detail/codegen.cc:798 template operand: q[0]
    [OPENQL] /Volumes/Data/shared/GIT/OpenQL/src/ql/arch/cc/pass/gen/vq1asm/detail/codegen.cc:798 template operand: q[10]

    cQASM "cz q[0],q[10]" with JSON "cz q0,q9" results in:
    [OPENQL] /Volumes/Data/shared/GIT/OpenQL/src/ql/arch/cc/pass/gen/vq1asm/detail/backend.cc:152 custom instruction: name=cz
    [OPENQL] /Volumes/Data/shared/GIT/OpenQL/src/ql/arch/cc/pass/gen/vq1asm/detail/codegen.cc:814 found template_operands: JSON = {"cc":{"signal":[],"static_codeword_override":[0]},"duration":40}
    [OPENQL] /Volumes/Data/shared/GIT/OpenQL/src/ql/arch/cc/pass/gen/vq1asm/detail/codegen.cc:798 template operand: q[0]
    [OPENQL] /Volumes/Data/shared/GIT/OpenQL/src/ql/arch/cc/pass/gen/vq1asm/detail/codegen.cc:809 operand: q[10]

    cQASM "cz q[0],q[10]" with JSON "cz q1,q10" results in:
    [OPENQL] /Volumes/Data/shared/GIT/OpenQL/src/ql/arch/cc/pass/gen/vq1asm/detail/backend.cc:152 custom instruction: name=cz
    [OPENQL] /Volumes/Data/shared/GIT/OpenQL/src/ql/arch/cc/pass/gen/vq1asm/detail/codegen.cc:829 operand: q[0]
    [OPENQL] /Volumes/Data/shared/GIT/OpenQL/src/ql/arch/cc/pass/gen/vq1asm/detail/codegen.cc:829 operand: q[10]
    */
#if 1   // FIXME
    if(!custom.instruction_type->template_operands.empty()) {
        QL_DOUT("found template_operands: JSON = " << custom.instruction_type->data.data );
        QL_INPUT_ERROR(
            "CC backend requires specialized instruction '" << ir::describe(custom) <<
            "' to be decomposed: check gate decompositions and parameters"
        );
    }
#else
    for (const auto &ob : custom.instruction_type->template_operands) {
        QL_DOUT("template operand: " + ir::describe(ob));
        try {
            ops.append(operandContext, ob);
        } catch (utils::Exception &e) {
            e.add_context("name=" + custom.instruction_type->name + ", qubits=" + ops.qubits.to_string());
            throw;
        }
    }
#endif

    // Process the 'plain' operands for custom instructions.
    for (utils::UInt i = 0; i < custom.operands.size(); i++) {
        QL_DOUT("operand: " + ir::describe(custom.operands[i]));
        try {
            ops.append(operandContext, custom.operands[i]);     // append utils::One<expression>
        } catch (utils::Exception &e) {
            e.add_context(
                "name=" + custom.instruction_type->name
                + ", qubits=" + ops.qubits.to_string()
                + ", operand=" + std::to_string(i)
                );
            throw;
        }
    }
    if (ops.has_integer) {
        QL_INPUT_ERROR("CC backend cannot handle integer operands yet");
    }
    if (ops.has_angle) {
        QL_INPUT_ERROR("CC backend cannot handle real (angle) operands yet");
    }

    // lambda to check measurement operands
    auto checkMeasOps = [ops]() {
        // Note that if all instruction definitions have proper prototypes this would be guaranteed upstream.
        if (ops.qubits.size() != 1) {
            QL_INPUT_ERROR(
                "Measurement instruction requires exactly 1 quantum operand, not " << ops.qubits.size()
            );
        }
    };

    // some shorthand for parameter fields
    const Str iname = custom.instruction_type->name;
    UInt durationInCycles = custom.instruction_type->duration;

    // generate VCD
    vcd.customGate(iname, ops.qubits, custom.cycle, durationInCycles);

    // generate comment
    comment(Str(" # gate '") + ir::describe(custom) + "'");

    // find signal vector definition for instruction
    const Json &instruction = custom.instruction_type->data.data;
    Settings::SignalDef sd = settings.findSignalDefinition(instruction, iname);

    // turn signals defined for instruction into instruments & groups, and update matching BundleInfo records
    for (UInt s = 0; s < sd.signal.size(); s++) {
        Settings::CalcSignalValue csv = settings.calcSignalValue(sd, s, ops.qubits, iname);

        comment(QL_SS2S(
            "  # slot=" << csv.si.ic.ii.slot
            << ", instrument='" << csv.si.ic.ii.instrumentName << "'"
            << ", group=" << csv.si.group
            << "': signalValue='" << csv.signalValueString << "'"
        ));

        // store signal value, checking for conflicts
        BundleInfo &bi = bundleInfo[csv.si.instrIdx][csv.si.group];         // shorthand
        if (!csv.signalValueString.empty()) {                               // empty implies no signal
            if (bi.signalValue.empty()) {                                   // signal not yet used
                bi.signalValue = csv.signalValueString;
#if OPT_SUPPORT_STATIC_CODEWORDS
                // FIXME: this does not only provide support, but findStaticCodewordOverride() currently actually requires static codewords
                // NB: value NO_STATIC_CODEWORD_OVERRIDE (-1) means 'no override'
                bi.staticCodewordOverride = Settings::findStaticCodewordOverride(instruction, csv.operandIdx, iname);
#endif
            } else if (bi.signalValue == csv.signalValueString) {           // signal unchanged
                // do nothing
            } else {
                showCodeSoFar();
                QL_USER_ERROR(
                    "Signal conflict on instrument='" << csv.si.ic.ii.instrumentName
                    << "', group=" << csv.si.group
                    << ", between '" << bi.signalValue
                    << "' and '" << csv.signalValueString << "'"
                );
            }
        }

        // store some attributes
        bi.durationInCycles = durationInCycles;
        bi.isMeasure = csv.isMeasure;
        bi.describe = ir::describe(custom);

        // handle measurement (for which we'll generate output allowing downstream software to retrieve them)
        if (csv.isMeasure) {
            checkMeasOps();
            bi.measQubit = ops.qubits[0];
        }

        // store operands used for real-time measurements, actual work is postponed to bundle_finish()
        if (settings.isMeasRsltRealTime(*custom.instruction_type)) {
            // FIXME: move the checks to collectCodeGenInfo?
            // FIXME: at the output side, similar checks are not performed
            /*
             * In the old IR, kernel->gate allows 3 types of measurement:
             *         - no explicit result. Historically this implies either:
             *             - no result, measurement results are often read offline from the readout device (mostly the raw values
             *             instead of the binary result), without the control device ever taking notice of the value
             *             - implicit bit result for qubit, e.g. for the CC-light using conditional gates
             *         - creg result (old, no longer valid)
             *             note that Creg's are managed through a class, whereas bregs are just numbers
             *         - breg result (new)
             *
             *  In the new IR (or, better said, in the new way "prototype"s for instruction operands van be defined
             *  using access modes as described in
             *  https://openql.readthedocs.io/en/latest/gen/reference_configuration.html#instructions-section
             *  it is not well possible to specify a measurement that returns its result in a different bit than
             *  the default bit.
             *  Since this poses no immediate problem, we only support measurements to the implicit default bit.
             *
             *  Also note that old_to_new.cc only uses the qubit operand, and the whole fact that any type of
             *  operand could be specified to any gate is a quirk of the kernel.gate() functions of the API.
             */

            // operand checks.
            checkMeasOps();
#if 0   // FIXME
            if (!ops.cregs.empty()) {
                QL_INPUT_ERROR("Using Creg as measurement target is deprecated, use new breg_operands");
            }
            if (ops.bregs.size() > 1) {
                QL_INPUT_ERROR(
                    "Readout instruction requires 0 or 1 bit operands, not " << ops.bregs.size()
                );
            }
#endif
            // flag this bundle as performing real-time measurements, and store operands
            bi.isMeasRsltRealTime = true;

            // handle classic operand
            if (ops.bregs.empty()) {    // FIXME: currently always
                bi.breg_operand = ops.qubits[0];                    // implicit classic bit for qubit
                QL_IOUT("using implicit bit " << bi.breg_operand << " for qubit " << ops.qubits[0]);
            } else {    // FIXME: currently impossible
                bi.breg_operand = ops.bregs[0];
                QL_IOUT("using explicit bit " << bi.breg_operand << " for qubit " << ops.qubits[0]);
            }
        }

        // Handle the condition. NB: the 'condition' field exists for all conditional_instruction sub types,
        // but we only handle it for custom_instruction
        tInstructionCondition instrCond = decode_condition(operandContext, custom.condition);
        bi.instructionCondition = instrCond;

        QL_DOUT("custom_instruction(): iname='" << iname <<
             "', duration=" << durationInCycles <<
             " [cycles], instrIdx=" << csv.si.instrIdx <<
             ", group=" << csv.si.group);

        // NB: code is generated in bundle_finish()
    }   // for(signal)
}

/************************************************************************\
| Structured control flow
\************************************************************************/

void Codegen::if_elif(const ir::ExpressionRef &condition, const Str &label, Int branch) {
    // finish previous branch
    if (branch>0) {
        cs.emit("", "jmp", as_target(to_end(label)));
    }

    comment(
        "# IF_ELIF: "
        "condition = '" + ir::describe(condition) + "'"
        ", label = '" + label + "'"
    );

    if(branch > 0) {    // label not used if branch==0
        Str my_label = to_ifbranch(label, branch);
        cs.emit(as_label(my_label));
    }

    Str jmp_label = to_ifbranch(label, branch+1);
    handle_expression(condition, jmp_label, "if.condition");
}


void Codegen::if_otherwise(const Str &label, Int branch) {
    comment(
        "# IF_OTHERWISE: "
        ", label = '" + label + "'"
    );

    Str my_label = to_ifbranch(label, branch);
    cs.emit(as_label(my_label));
}


void Codegen::if_end(const Str &label) {
    comment(
        "# IF_END: "
        ", label = '" + label + "'"
    );

    cs.emit(as_label(to_end(label)));
}


void Codegen::foreach_start(const ir::Reference &lhs, const ir::IntLiteral &frm, const Str &label) {
    comment(
        "# FOREACH_START: "
        "from = " + ir::describe(frm)
        + ", label = '" + label + "'"
    );

    auto reg = as_reg(cs.creg2reg(lhs));
    cs.emit("", "move", as_int(frm.value) + "," + reg);
    // FIXME: if loop has no contents at all, register dependency is violated
    cs.emit(as_label(to_start(label)));    // label for looping or 'continue'
}


void Codegen::foreach_end(const ir::Reference &lhs, const ir::IntLiteral &frm, const ir::IntLiteral &to, const Str &label) {
    comment(
        "# FOREACH_END: "
        "from = " + ir::describe(frm)
        + ", to = " + ir::describe(to)
        + ", label = '" + label + "'"
    );

    auto reg = as_reg(cs.creg2reg(lhs));

    if(to.value >= frm.value) {     // count up
        cs.emit("", "add", reg + ",1," + reg);
        cs.emit("", "nop");
        cs.emit(
            "",
            "jlt",
            reg + "," + as_int(to.value, 1) + "," + as_target(to_start(label)),
            "# loop");
    } else {
        if(to.value == 0) {
            cs.emit("", "loop", reg + "," + as_target(to_start(label)), "# loop");
        } else {
            cs.emit("", "sub", reg + ",1," + reg);
            cs.emit("", "nop");
            cs.emit("", "jge", reg + "," + as_int(to.value) + "," + as_target(to_start(label)), "# loop");
        }
    }

    cs.emit(as_label(to_end(label)));    // label for loop end or 'break'
}


void Codegen::repeat(const Str &label) {
    comment(
        "# REPEAT: "
        ", label = '" + label + "'"
    );
    cs.emit(as_label(to_start(label)));    // label for looping or 'continue'
}


void Codegen::until(const ir::ExpressionRef &condition, const Str &label) {
    comment(
        "# UNTIL: "
        "condition = '" + ir::describe(condition) + "'"
        ", label = '" + label + "'"
    );
    handle_expression(condition, to_end(label), "until.condition");
    cs.emit("", "jmp", as_target(to_start(label)), "# loop");
    cs.emit(as_label(to_end(label)));    // label for loop end or 'break'
}

// NB: also used for 'while' loops
void Codegen::for_start(utils::Maybe<ir::SetInstruction> &initialize, const ir::ExpressionRef &condition, const Str &label) {
    comment(
        "# LOOP_START: "
        + (!initialize.empty() ? "initialize = '"+ir::describe(initialize)+"', " : "")
        + "condition = '" + ir::describe(condition) + "'"
    );

    // for loop: initialize
    if (!initialize.empty()) {
        handle_set_instruction(*initialize, "for.initialize");
        cs.emit("", "nop");    // register dependency between initialize and handle_expression (if those use the same register, which is likely)
    }

    cs.emit(as_label(to_start(label)));    // label for looping or 'continue'
    handle_expression(condition, to_end(label), "for/while.condition");
}


void Codegen::for_end(utils::Maybe<ir::SetInstruction> &update, const Str &label) {
    comment(
        "# LOOP_END: "
        + (!update.empty() ? " update = '"+ir::describe(update)+"'" : "")
    );
    if (!update.empty()) {
        handle_set_instruction(*update, "for.update");
    }
    cs.emit("", "jmp", as_target(to_start(label)), "# loop");
    cs.emit(as_label(to_end(label)));    // label for loop end or 'break'
}


void Codegen::do_break(const Str &label) {
    cs.emit("", "jmp", as_target(to_end(label)), "# break");
}


void Codegen::do_continue(const Str &label) {
    cs.emit("", "jmp", as_target(to_start(label)), "# continue");
}


void Codegen::comment(const Str &c) {
    if (options->verbose) {
        cs.emit(Str(2*depth, ' ') + c);  // indent by depth
    }
}

/************************************************************************\
| new IR expressions
\************************************************************************/

// FIXME: recursion?
// FIXME: or pass SetInstruction or Expression depending on use
// FIXME: adopt structure of cQASM's cqasm-v1-functions-gen.cpp register_into used for constant propagation

// FIXME: see expression_mapper.cc for inspiration
// FIXME: split with next function, move to .h

void Codegen::handle_set_instruction(const ir::SetInstruction &set, const Str &descr)
{
    Str describe = ir::describe(set);
    QL_DOUT(descr + ": '" + describe + "'");

    try {
        // enforce set instruction is unconditional (since we don't handle conditionality)
        CHECK_COMPAT(
            set.condition->as_bit_literal()
            && set.condition->as_bit_literal()->value
            , "conditions other then 'true' are not supported for set instruction"
        );

        comment("# Expression '" + descr + "': " + describe);

        if (auto ilit = set.rhs->as_int_literal()) {
            cs.emit(
                "",
                "move",
                as_int(ilit->value) + "," + as_reg(cs.dest_reg(set.lhs)),
                "# " + ir::describe(set.rhs) //FIXME: full expression
            );
        } else if (auto ref = set.rhs->as_reference()) {
            if(operandContext.is_creg_reference(*ref)) {
                auto reg = cs.creg2reg(*ref);
                cs.emit(
                    "",
                    "move",
                    as_reg(reg) + "," + as_reg(cs.dest_reg(set.lhs)),
                    "# " + describe
                );
            } else {
                QL_ICE("expected reference to creg, but got: " << ir::describe(set.rhs));
            }
        } else if (auto fn = set.rhs->as_function_call()) {
            // handle int cast
            if (fn->function_type->name == "int") {
                CHECK_COMPAT(
                    fn->operands.size() == 1 &&
                    fn->operands[0]->as_function_call(),
                    "'int()' cast target must be a function"
                );
                fn = fn->operands[0]->as_function_call();   // step into. FIXME: Shouldn't we recurse to allow e.g. casting a breg??
            }

            // handle the function
            fncs.dispatch(set.lhs, fn, describe);
        } else {
            QL_ICE("unsupported expression type");
        }
    }
    catch (utils::Exception &e) {
        e.add_context("in expression '" + describe + "'", true);
        throw;
    }
}

void Codegen::handle_expression(const ir::ExpressionRef &expression, const Str &label_if_false, const Str &descr)
{
    Str describe = ir::describe(expression);
    QL_DOUT(descr + ": '" + describe + "'");

    try {
#if 0   // NB: redundant information, also provided by structured control flow comments
        comment("# Expression '" + descr + "': " << ir::describe(expression));
#endif

        if (auto blit = expression->as_bit_literal()) {
            if(blit->value) {
                // do nothing (to jump out of loop). FIXME: other contexts may exist
            } else {    // FIXME: should be handled by constant removal pass
                QL_ICE("bit literal 'false' currently not supported in '" << ir::describe(expression) << "'");
            }
        } else if (auto ref = expression->as_reference()) {
            if (operandContext.is_breg_reference(*ref)) {    // breg as condition, like in "if(b[0])"
                // transfer single breg to REG_TMP0
                auto breg = operandContext.convert_breg_reference(*ref);
                utils::Vec<utils::UInt> bregs{breg};
                UInt mask = fncs.emit_bin_cast(bregs, 1);

                cs.emit(
                    "",
                    "and",
                    QL_SS2S(REG_TMP0 << "," << mask << "," << REG_TMP1),
                    "# mask for '" + ir::describe(expression) + "'"
                );    // results in '0' for 'bit==0' and 'mask' for 'bit==1'
                cs.emit("", "nop");
                cs.emit(
                    "",
                    "jlt",
                    Str(REG_TMP1) + ",1," + as_target(label_if_false),
                    "# skip next part if condition is false"
                );
            } else {
                QL_ICE("expected reference to breg, but got: " << ir::describe(expression));
            }
        } else if (auto fn = expression->as_function_call()) {
            // FIXME: handle (bit) cast?

            // handle the function
            fncs.dispatch(fn, label_if_false, describe);
        } else {
            QL_ICE("unsupported expression type");
        }
    }
    catch (utils::Exception &e) {
        e.add_context("in expression '" + describe + "'", true);
        throw;
    }
}


/************************************************************************\
|
| private functions
|
\************************************************************************/

/************************************************************************\
| helpers
\************************************************************************/

void Codegen::emitProgramStart(const Str &progName) {
    cs.emitProgramHeader(progName);

    cs.emit(".CODE");   // start .CODE section

    // NB: new seq_bar semantics (firmware from 20191219 onwards)
    comment("# synchronous start and latency compensation");
    cs.emit("",                "seq_bar",  "",                 "# synchronization, delay set externally through SET_SEQ_BAR_CNT");
    cs.emit("",                "seq_out",  "0x00000000,1",     "# allows monitoring actual start time using trace unit");
    if (!options->run_once) {
        comment("# start of main loop that runs indefinitely");
        cs.emit("__mainLoop:",     "",         "",                 "# ");    // FIXME: __mainLoop should be a forbidden kernel name
    }

    // initialize state
    cs.emit("",                "seq_state","0",                "# clear Programmable Logic state");
}


void Codegen::emitProgramFinish() {
    comment("# finish program");
    if (options->run_once) {   // program runs once only
        cs.emit("", "stop");
    } else {   // CC-light emulation: loop indefinitely
        // prevent real time pipeline emptying during jmp below (especially in conjunction with pragma/break
        cs.emit("", "seq_wait", "1");

        // loop indefinitely
        cs.emit("",      // no CCIO selector
             "jmp",
             "@__mainLoop",
             "# loop indefinitely");
    }

    cs.emit(".END");   // end .CODE section
}


// generate code to input measurement results and distribute them via DSM
void Codegen::emitMeasRsltRealTime(
    const MeasResultRealTimeMap &measResultRealTimeMap,
    UInt instrIdx,
    UInt startCycle,
    Int slot,
    const Str &instrumentName
) {
    if (startCycle > lastEndCycle[instrIdx]) {  // i.e. if(!instrHasOutput)
        emitPadToCycle(instrIdx, startCycle, slot, instrumentName);
    }

    // code generation for participating and non-participating instruments
    // NB: both code paths must take equal number of sequencer cycles
    if (!measResultRealTimeMap.empty()) {    // this instrument produces real-time measurements now
        UInt mux = dp.getOrAssignMux(instrIdx, measResultRealTimeMap);
        dp.emitMux(mux, measResultRealTimeMap, slot);

        // emit code for slot input
        UInt sizeTag = Datapath::getSizeTag(measResultRealTimeMap.size());        // compute DSM transfer size tag (for 'seq_in_sm' instruction)
        UInt smAddr = Datapath::getMuxSmAddr(measResultRealTimeMap);
        cs.emit(
            slot,
            "seq_in_sm",
            QL_SS2S("S" << smAddr << ","  << mux << "," << sizeTag),
            QL_SS2S("# cycle " << lastEndCycle[instrIdx] << "-" << lastEndCycle[instrIdx]+1 << ": real-time measurement result on '" << instrumentName+"'")
        );
        lastEndCycle[instrIdx]++;
    } else {    // this instrument does not produce real-time measurements now
        // emit code for non-participating instrument
        // FIXME FIXME FIXME: may invalidate DSM that just arrived dependent on individual SEQBAR counts
        UInt smAddr = 0;
        UInt smTotalSize = 1;    // FIXME: inexact, but me must not invalidate memory that we will not write
        cs.emit(
            slot,
            "seq_inv_sm",
            QL_SS2S("S" << smAddr << ","  << smTotalSize),
            QL_SS2S("# cycle " << lastEndCycle[instrIdx] << "-" << lastEndCycle[instrIdx]+1 << ": invalidate SM on '" << instrumentName+"'")
        );
        lastEndCycle[instrIdx]++;
    }
}


void Codegen::emitOutput(
        const CondGateMap &condGateMap,
        tDigital digOut,
        UInt instrMaxDurationInCycles,
        UInt instrIdx,
        UInt startCycle,
        Int slot,
        const Str &instrumentName
) {
    comment(QL_SS2S(
        "  # slot=" << slot
        << ", instrument='" << instrumentName << "'"
        << ": lastEndCycle=" << lastEndCycle[instrIdx]
        << ", startCycle=" << startCycle
        << ", instrMaxDurationInCycles=" << instrMaxDurationInCycles
    ));

    emitPadToCycle(instrIdx, startCycle, slot, instrumentName);

    // emit code for slot output
    if (condGateMap.empty()) {    // all groups unconditional
        cs.emit(
            slot,
            "seq_out",
            QL_SS2S("0x" << std::hex << std::setfill('0') << std::setw(8) << digOut << std::dec << "," << instrMaxDurationInCycles),
            QL_SS2S("# cycle " << startCycle << "-" << startCycle + instrMaxDurationInCycles << ": code word/mask on '" << instrumentName + "'")
        );
    } else {    // at least one group conditional
        // configure datapath PL
        UInt pl = dp.getOrAssignPl(instrIdx, condGateMap);
        UInt smAddr = dp.emitPl(pl, condGateMap, instrIdx, slot);

        // emit code for conditional gate
        cs.emit(
            slot,
            "seq_out_sm",
            QL_SS2S("S" << smAddr << "," << pl << "," << instrMaxDurationInCycles),
            QL_SS2S("# cycle " << startCycle << "-" << startCycle + instrMaxDurationInCycles << ": conditional code word/mask on '" << instrumentName << "'")
        );
    }

    // update lastEndCycle
    lastEndCycle[instrIdx] = startCycle + instrMaxDurationInCycles;
}


void Codegen::emitPadToCycle(UInt instrIdx, UInt startCycle, Int slot, const Str &instrumentName) {
    // compute prePadding: time to bridge to align timing
    Int prePadding = startCycle - lastEndCycle[instrIdx];
    if (prePadding < 0) {
        QL_EOUT("Inconsistency detected in bundle contents: printing code generated so far");
        showCodeSoFar();
        QL_INPUT_ERROR(
            "Inconsistency detected in bundle contents: time travel not yet possible in this version: prePadding=" << prePadding
            << ", startCycle=" << startCycle
            << ", lastEndCycle=" << lastEndCycle[instrIdx]
            << ", instrumentName='" << instrumentName << "'"
            << ", instrIdx=" << instrIdx
        );
    }

    if (prePadding > 0) {     // we need to align
        cs.emit(
            slot,
            "seq_wait",
            QL_SS2S(prePadding),
            QL_SS2S("# cycle " << lastEndCycle[instrIdx] << "-" << startCycle << ": padding on '" << instrumentName+"'")
        );
    }

    // update lastEndCycle
    lastEndCycle[instrIdx] = startCycle;
}


// FIXME: broken, but we do want to reinstate automatic codeword generation at some point
#if !OPT_SUPPORT_STATIC_CODEWORDS
tCodeword Codegen::assignCodeword(const Str &instrumentName, Int instrIdx, Int group) {
    tCodeword codeword;
    Str signalValue = bi->signalValue;

    if (QL_JSON_EXISTS(codewordTable, instrumentName)                       // instrument exists
        && codewordTable[instrumentName].size() > group                     // group exists
    ) {
        Bool cwFound = false;
        // try to find signalValue
        Json &myCodewordArray = codewordTable[instrumentName][group];
        for (codeword = 0; codeword < myCodewordArray.size() && !cwFound; codeword++) {   // NB: JSON find() doesn't work for arrays
            if (myCodewordArray[codeword] == signalValue) {
                QL_DOUT("signal value found at cw=" << codeword);
                cwFound = true;
            }
        }
        if (!cwFound) {
            Str msg = QL_SS2S("signal value '" << signalValue
                    << "' not found in group " << group
                    << ", which contains " << myCodewordArray);
            if (mapPreloaded) {
                QL_USER_ERROR("mismatch between preloaded 'map_input_file' and program requirements:" << msg)
            } else {
                QL_DOUT(msg);
                // NB: codeword already contains last used value + 1
                // FIXME: check that number is available
                myCodewordArray[codeword] = signalValue;                    // NB: structure created on demand
            }
        }
    } else {    // new instrument or group
        if (mapPreloaded) {
            QL_USER_ERROR("mismatch between preloaded 'map_input_file' and program requirements: instrument '"
                  << instrumentName << "', group "
                  << group
                  << " not present in file");
        } else {
            codeword = 1;
            codewordTable[instrumentName][group][0] = "";                   // code word 0 is empty
            codewordTable[instrumentName][group][codeword] = signalValue;   // NB: structure created on demand
        }
    }
    return codeword;
}
#endif


} // namespace detail
} // namespace vq1asm
} // namespace gen
} // namespace pass
} // namespace cc
} // namespace arch
} // namespace ql
