/**
 * @file   arch/cc/pass/gen/vq1asm/detail/backend.cc
 * @date   201809xx
 * @author Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief  backend for the Central Controller
 */

#include "backend.h"
#include "operands.h"

#include "ql/utils/str.h"
#include "ql/utils/filesystem.h"
#include "ql/ir/describe.h"
#include "ql/ir/ops.h"
#include "ql/com/options.h"

#include <regex>

namespace ql {
namespace arch {
namespace cc {
namespace pass {
namespace gen {
namespace vq1asm {
namespace detail {

using namespace utils;

// compile for Central Controller
Backend::Backend(const ir::Ref &ir, const OptionsRef &options) : codegen(ir, options) {
    QL_DOUT("Compiling Central Controller program ... ");

    // generate program header
    codegen.programStart(ir->program->unique_name);

    // FIXME: Nodes of interest:
    // ir->program->entry_point.links_to

    // generate code for all blocks
    // NB: based on NewToOldConverter::NewToOldConverter
    for (const auto &block : ir->program->blocks) {
        try {
            codegen_block(block, block->name);
        } catch (utils::Exception &e) {
            e.add_context("in block '" + block->name + "'");
            throw;
        }
    }

    codegen.programFinish(ir->program->unique_name);

    // write program to file
    Str file_name(options->output_prefix + ".vq1asm");
    QL_IOUT("Writing Central Controller program to " << file_name);
    OutFile(file_name).write(codegen.getProgram());

    // write instrument map to file (unless we were using input file)
    Str map_input_file = options->map_input_file;
    if (!map_input_file.empty()) {
        Str file_name_map(options->output_prefix + ".map");
        QL_IOUT("Writing instrument map to " << file_name_map);
        OutFile(file_name_map).write(codegen.getMap());
    }

    QL_DOUT("Compiling Central Controller program [Done]");
}



#if 0
/* get loop label from kernel name
 *
 * Program::add_for() and Program::add_do_while() generate kernels that wrap a program or kernel inside
 * a 'start' and 'end' kernel that only contain the looping information. The names of these added kernels are derived
 * from the name of the wrapped object by adding a suffix. Examples:
 * -    "program" results e.g. in "program_for3389_start" or "program_for3389_end"
 * -    "kernel" results e.g. in "kernel_do_while1_start" or "kernel_do_while1" (without "_end")
 *
 * This function obtains the stem name without the added suffix (e.g. '_for0_start') to use as a label to share between
 * the 'start' and 'end' code.
 *
 * Notes:
 * -    Other than the original in Kernel::get_epilogue, we extract the full stem, not only the part up to the first
 *      "_" to prevent duplicate labels if users choose to use names that are identical before the first "_"
 * -    numbering is performed using "static unsigned long phi_node_count = 0;" and thus persists over compiler invocations
 */

// FIXME: originally extracted from Kernel::get_epilogue, should be in a common place

Str Backend::loopLabel(const ir::compat::KernelRef &k) {
    Str label;
    Str expr;

    switch (k->type) {
        case ir::compat::KernelType::FOR_START:
            expr = "(.*)_for[0-9]+_start$";
            break;

        case ir::compat::KernelType::FOR_END:
            expr = "(.*)_for[0-9]+_end$";
            break;

        case ir::compat::KernelType::DO_WHILE_START:
            expr = "(.*)_do_while[0-9]+_start$";
            break;

        case ir::compat::KernelType::DO_WHILE_END:
            expr = "(.*)_do_while[0-9]+$";  // NB: there is no "_end" here, see quantum_program::add_do_while()
            break;

        default:
            QL_ICE("internal inconsistency: requesting kernel label for kernel type " << (int)k->type);
    }

    std::regex re(expr, std::regex_constants::egrep);   // FIXME: we are reverse engineering the naming scheme of quantum_program::add_*
    const int numMatch = 1+1;		// NB: +1 because index 0 contains full input match
    std::smatch match;
    if(std::regex_search(k->name, match, re) && match.size() == numMatch) {
        label = match.str(1);
    } else {
        QL_ICE("internal inconsistency: kernel name '" << k->name << "' does not contain loop suffix");
    }

    QL_IOUT("kernel '" << k->name << "' gets label '" << label << "'");
    return label;
}


// handle kernel conditionality at beginning of kernel
// based on cc_light_eqasm_compiler.h::get_prologue
void Backend::codegenKernelPrologue(const ir::compat::KernelRef &k) {
    codegen.comment(QL_SS2S("### Kernel: '" << k->name << "'"));

    switch (k->type) {
        case ir::compat::KernelType::IF_START: {
            auto op0 = k->br_condition->operands[0]->as_register().id;
            auto op1 = k->br_condition->operands[1]->as_register().id;
            auto opName = k->br_condition->operation_name;
            codegen.ifStart(op0, opName, op1);
            break;
        }

        case ir::compat::KernelType::ELSE_START: {
            auto op0 = k->br_condition->operands[0]->as_register().id;
            auto op1 = k->br_condition->operands[1]->as_register().id;
            auto opName = k->br_condition->operation_name;
            codegen.elseStart(op0, opName, op1);
            break;
        }

        case ir::compat::KernelType::FOR_START: {
            codegen.forStart(loopLabel(k), k->iteration_count);
            break;
        }

        case ir::compat::KernelType::DO_WHILE_START: {
            codegen.doWhileStart(loopLabel(k));
            break;
        }

        case ir::compat::KernelType::STATIC:
        case ir::compat::KernelType::FOR_END:
        case ir::compat::KernelType::DO_WHILE_END:
        case ir::compat::KernelType::IF_END:
        case ir::compat::KernelType::ELSE_END:
            // do nothing
            break;

        default:
            QL_ICE("inconsistency detected: unhandled kernel type");
            break;
    }
}


// handle kernel conditionality at end of kernel
// based on cc_light_eqasm_compiler.h::get_epilogue
void Backend::codegenKernelEpilogue(const ir::compat::KernelRef &k) {
    switch (k->type) {
        case ir::compat::KernelType::FOR_END: {
            codegen.forEnd(loopLabel(k));
            break;
        }

        case ir::compat::KernelType::DO_WHILE_END: {
            auto op0 = k->br_condition->operands[0]->as_register().id;
            auto op1 = k->br_condition->operands[1]->as_register().id;
            auto opName = k->br_condition->operation_name;
            codegen.doWhileEnd(loopLabel(k), op0, opName, op1);
            break;
        }

        case ir::compat::KernelType::IF_END:
        case ir::compat::KernelType::ELSE_END:
            // do nothing
            break;

        case ir::compat::KernelType::STATIC:
        case ir::compat::KernelType::IF_START:
        case ir::compat::KernelType::ELSE_START:
        case ir::compat::KernelType::FOR_START:
        case ir::compat::KernelType::DO_WHILE_START:
            // do nothing
            break;

        default:
            QL_ICE("inconsistency detected: unhandled kernel type");
            break;
    }
}
#endif


/*
 * Generate code for a single block (which sort of matches the concept of a Kernel in the old API). Recursively calls
 * itself where necessary.
 * Based on NewToOldConverter::convert_block
 */
// FIXME: convert block relative cycles to absolute cycles somewhere
// FIXME: runOnce automatically on cQASM input
// FIXME: provide (more) context in all QL_ICE and e.add_context
// FIXME: use annotations in comments/messages
void Backend::codegen_block(const ir::BlockBaseRef &block, const Str &name)
{
    // Return the name for a child of this block. We use "__" to prevent clashes with names assigned by user (assuming
    // they won't use names with "__", similar to the C rule for identifiers), and give some sense of hierarchy
    // level.
    auto block_child_name = [name](const Str &child_name) { return "__" + name + "__" + child_name; };

    // Return the label (stem) for this block. Note that this is used as q1asm label and must adhere to
    // the allowed structure of that. The appending of block_number is to uniquify anonymous blocks like for loops.
    auto label = [this, name]() { return QL_SS2S(name << "__" << block_number); };

#if 0   // FIXME: org
    // Whether this is the first lazily-constructed kernel. Only if this is true
    // when flushing at the end are statistics annotations copied; otherwise
    // they would be invalid anyway.
    utils::Bool first_kernel = true;

    // Cycle offset for converting from new-IR cycles to old-IR cycles. In
    // the new IR, cycles start at zero; in the old one they start at
    // compat::FIRST_CYCLE. This is set to utils::MAX as a marker after
    // structured control-flow; this implies that the next cycle number
    // encountered should map to compat::FIRST_CYCLE.
    utils::Int cycle_offset = compat::FIRST_CYCLE;

    // Whether to set the cycles_valid flag on the old-style kernel. Cycles are
    // always valid in the new IR, but when the program was previously converted
    // from the old to the new IR, annotations can be used to clear the flag.
    utils::Bool cycles_valid = true;
    if (auto kcv = block->get_annotation_ptr<KernelCyclesValid>()) {
        cycles_valid = kcv->valid;
    }
#endif



    // FIXME: add comments
    Int bundle_start_cycle = -1;
    Int bundle_end_cycle = -1;
    Bool is_bundle_open = false;

    // helper
    auto bundle_finish = [this, &bundle_start_cycle, &bundle_end_cycle](Bool is_last_bundle) {
        Int bundle_duration =  bundle_end_cycle - bundle_start_cycle;
        QL_DOUT(QL_SS2S("Finishing bundle " << bundleIdx << ": start_cycle=" << bundle_start_cycle << ", duration=" << bundle_duration));
        codegen.bundleFinish(bundle_start_cycle, bundle_duration, is_last_bundle);
    };

    QL_IOUT("Compiling block '" + label() + "'");
    codegen.block_start(name);

    // Loop over the statements and handle them individually.
    for (const auto &stmt : block->statements) {

        if (auto insn = stmt->as_instruction()) {
            //****************************************************************
            // Statement: instruction
            //****************************************************************

            auto duration = ir::get_duration_of_statement(stmt);
            QL_DOUT(
                "instruction: '" + ir::describe(stmt)
                + "', cycle=" + std::to_string(insn->cycle)
                + ", duration=" + std::to_string(duration)
            );

            bundle_end_cycle = insn->cycle + duration;  // keep updating, used by bundle_finish()
            Bool is_new_bundle = insn->cycle != bundle_start_cycle;

            // Generate bundle trailer when necessary.
            if (is_new_bundle && is_bundle_open) {
                bundle_finish(false);   // NB: finishing previous bundle, so that isn't the last one
                is_bundle_open = false;
            }

            // Generate bundle header when necessary.
            if (is_new_bundle) {
                bundleIdx++;
                QL_DOUT(QL_SS2S("Bundle " << bundleIdx << ": start_cycle=" << insn->cycle));
                // NB: first instruction may be wait with zero duration, more generally: duration of first statement != bundle duration
                codegen.bundleStart(QL_SS2S(
                    "## Bundle " << bundleIdx
                    << ": start_cycle=" << insn->cycle
                    << ":"
                ));

                is_bundle_open = true;
                bundle_start_cycle = insn->cycle;
            }

#if 0   // FIXME: org
            // Ensure that we have a kernel to add the instruction to, and
            // that cycle_offset is valid.
            if (kernel.empty()) {
                kernel.emplace(
                    make_kernel_name(block), old->platform,
                    old->qubit_count, old->creg_count, old->breg_count
                );
            }
            if (cycle_offset == utils::MAX) {
                cycle_offset = compat::FIRST_CYCLE - insn->cycle;
            }

            // The kernel.gate() calls can add more than one instruction due to
            // ad-hoc decompositions. Since we need to set the cycle numbers
            // after the fact, we need to track which gates already existed in
            // the kernel.
            auto first_gate_index = kernel->gates.size();
#endif

            // Handle the instruction subtypes.
            if (auto cinsn = stmt->as_conditional_instruction()) {

                //****************************************************************
                // Instruction: conditional
                //****************************************************************

                // Handle the conditional instruction subtypes.
                if (auto custom = cinsn->as_custom_instruction()) {

                    QL_DOUT("custom instruction: name=" + custom->instruction_type->name);
                    codegen.custom_instruction(*custom);

                } else if (auto set_instruction = cinsn->as_set_instruction()) {

                    //****************************************************************
                    // Instruction: set
                    //****************************************************************
                    CHECK_COMPAT(
                        set_instruction->condition->as_bit_literal()
                        && set_instruction->condition->as_bit_literal()->value
                        , "conditions other then 'true' are not supported for set instruction"
                    );
                    codegen.handle_set_instruction(*set_instruction, "conditional.set");

                } else if (cinsn->as_goto_instruction()) {
                    QL_INPUT_ERROR("goto instruction not supported");

                } else {
                    QL_ICE(
                        "unsupported instruction type encountered"
                        << "'" <<ir::describe(stmt) << "'"
                    );
                }

            } else if (auto wait = stmt->as_wait_instruction()) {

                //****************************************************************
                // Instruction: wait
                //****************************************************************
                // NB: waits are already accounted for during scheduling, so backend can ignore these
                QL_DOUT("wait (ignored by backend)");

            } else {
                QL_ICE(
                    "unsupported statement type encountered: "
                    << "'" <<ir::describe(stmt) << "'"
                );
            }

#if 0   // FIXME: org
            // Copy gate annotations if adding the gate resulted in just one
            // gate.
            if (kernel->gates.size() == first_gate_index + 1) {
                kernel->gates[first_gate_index]->copy_annotations(*insn);
            }

            // Assign the cycle numbers for the new gates.
            for (auto i = first_gate_index; i < kernel->gates.size(); i++) {
                kernel->gates[i]->cycle = (utils::UInt)((utils::Int)insn->cycle + cycle_offset);
            }
#endif

        } else if (stmt->as_structured()) {

            //****************************************************************
            // Statement: structured
            //****************************************************************
            QL_IOUT("structured: " + ir::describe(stmt));
#if 0   // FIXME: org
             // Flush any pending kernel not affected by control-flow.
            if (!kernel.empty()) {
                first_kernel = false;
                kernel->cycles_valid = cycles_valid;
                program->add(kernel);
                kernel.reset();
            }
            cycle_offset = utils::MAX;
#endif
            // Handle the different types of structured statements.
            if (auto if_else = stmt->as_if_else()) {

                Str saved_label = label();  // changes when recursing

                // Handle if-else (or just if) statement.
                for(UInt branch=0; branch<if_else->branches.size(); branch++) {
                    // if-condition
                    try {
                        codegen.if_else(if_else->branches[branch]->condition, saved_label, branch);
                    } catch (utils::Exception &e) {
                        e.add_context("in 'if' condition", true);
                        throw;
                    }

                    // if-block
                    try {
                        codegen_block(if_else->branches[branch]->body, block_child_name(QL_SS2S("if_else_"<<branch)));
                    } catch (utils::Exception &e) {
                        e.add_context("in 'if_else' block", true);
                        throw;
                    }
                }

                // otherwise
                codegen.if_otherwise(saved_label, if_else->branches.size());   // NB: doesn't throw exceptions

                // otherwise-block
                if (!if_else->otherwise.empty()) {
                    try {
                        codegen_block(if_else->otherwise, block_child_name("otherwise"));
                    } catch (utils::Exception &e) {
                        e.add_context("in final 'else' block", true);
                        throw;
                    }
                }

                codegen.if_end(saved_label);   // NB: doesn't throw exceptions

            } else if (auto static_loop = stmt->as_static_loop()) {

                // Handle static loops.
                loop_label.push_back(label());          // remind label for break/continue, before recursing

                // start:
                codegen.foreach_start(*static_loop->lhs, *static_loop->frm, loop_label.back());

                try {
                    codegen_block(static_loop->body, block_child_name("static_for"));
                } catch (utils::Exception &e) {
                    e.add_context("in static loop body", true);
                    throw;
                }

                // end:
                codegen.foreach_end(*static_loop->frm, *static_loop->to, loop_label.back());
                loop_label.pop_back();

            } else if (auto repeat_until_loop = stmt->as_repeat_until_loop()) {

                // Handle repeat-until loops.
                loop_label.push_back(label());          // remind label for break/continue, before recursing
                codegen.repeat(loop_label.back());

                try {
                    codegen_block(repeat_until_loop->body, block_child_name("repeat_until"));
                } catch (utils::Exception &e) {
                    e.add_context("in repeat-until loop body", true);
                    throw;
                }

                codegen.until(repeat_until_loop->condition, loop_label.back());
                loop_label.pop_back();

            } else if (auto for_loop = stmt->as_for_loop()) {

                // for loop: start
                loop_label.push_back(label());          // remind label for break/continue, before recursing
                codegen.for_start(for_loop->initialize, for_loop->condition, loop_label.back());

                // handle body
                try {
                    codegen_block(for_loop->body, block_child_name("for"));
                } catch (utils::Exception &e) {
                    e.add_context("in for loop body", true);
                    throw;
                }

                // handle looping
                codegen.for_end(for_loop->update, loop_label.back());   // NB: label() has changed because of recursion into codegen_block
                loop_label.pop_back();

            } else if (stmt->as_break_statement()) {
                codegen.do_break(loop_label.back());

            } else if (stmt->as_continue_statement()) {
                codegen.do_continue(loop_label.back());

            } else {
                QL_ICE(
                    "unsupported structured control-flow statement"
                    << " '" << ir::describe(stmt) << "' "
                    << "encountered"
                );
            }

        } else {
            QL_ICE(
                "unsupported statement type encountered"
                << "'" <<ir::describe(stmt) << "'"
            );
        }
    }

#if 0   // FIXME: org
    // Flush any pending kernel.
    if (!kernel.empty()) {

        // If this block produced only one kernel, copy kernel-wide annotations.
        if (first_kernel) {
            kernel->copy_annotations(*block);
        }

        kernel->cycles_valid = cycles_valid;
        program->add(kernel);
        kernel.reset();
    }
#endif

    // Flush any pending bundle.
    if (is_bundle_open) {
        bundle_finish(true);
    }

    codegen.block_finish(name, ir::get_duration_of_block(block));
    QL_IOUT("Finished compiling block '" + label() + "'");
    block_number++;

}


#if 0   // FIXME: old
// based on cc_light_eqasm_compiler.h::bundles2qisa()
void Backend::codegenBundles(ir::compat::Bundles &bundles, const ir::compat::PlatformRef &platform) {
    QL_IOUT("Generating .vq1asm for bundles");

    for (const auto &bundle : bundles) {
        // generate bundle header
        QL_DOUT(QL_SS2S("Bundle " << bundleIdx << ": start_cycle=" << bundle.start_cycle << ", duration_in_cycles=" << bundle.duration_in_cycles));
        codegen.bundleStart(QL_SS2S(
            "## Bundle " << bundleIdx++
            << ": start_cycle=" << bundle.start_cycle
            << ", duration_in_cycles=" << bundle.duration_in_cycles << ":"
        ));
        // NB: the "wait" instruction never makes it into the bundle. It is accounted for in scheduling though,
        // and if a non-zero duration is specified that duration is reflected in 'start_cycle' of the subsequent instruction

        // generate code for this bundle
        for (const auto &instr : bundle.gates) {
            // check whether section defines classical gate
            if (instr->type() == ir::compat::GateType::CLASSICAL) {
                QL_DOUT(QL_SS2S("Classical bundle: instr='" << instr->name << "'"));
                codegenClassicalInstruction(instr);
            } else {
                ir::compat::GateType itype = instr->type();
                Str iname = instr->name;
                QL_DOUT(QL_SS2S("Bundle section: instr='" << iname << "'"));

                switch (itype) {
                    case ir::compat::GateType::NOP:       // a quantum "nop", see gate.h
                        codegen.nopGate();
                        break;

                    case ir::compat::GateType::CLASSICAL:
                        QL_ICE("Inconsistency detected in bundle contents: classical gate found after first section (which itself was non-classical)");
                        break;

                    case ir::compat::GateType::CUSTOM:
                        QL_DOUT(QL_SS2S("Custom gate: instr='" << iname << "'" << ", duration=" << instr->duration) << " ns");
                        codegen.customGate(
                            iname,
                            instr->operands,            // qubit operands (FKA qops)
                            instr->creg_operands,        // classic operands (FKA cops)
                            instr->breg_operands,         // bit operands e.g. assigned to by measure
                            instr->condition,
                            instr->cond_operands,        // 0, 1 or 2 bit operands of condition
                               instr->angle,
                               bundle.start_cycle, platform->time_to_cycles(instr->duration)
                        );
                        break;

                    case ir::compat::GateType::DISPLAY:
                        QL_ICE("Gate type __display__ not supported");           // QX specific, according to openql.pdf
                        break;

                    case ir::compat::GateType::MEASURE:
                        QL_ICE("Gate type __measure_gate__ not supported");      // no use, because there is no way to define CC-specifics
                        break;

                    default:
                        QL_ICE(
                            "Unsupported builtin gate, type: " << itype
                            << ", instruction: '" << instr->qasm() << "'");
                }   // switch(itype)
            }
        }

        // generate bundle trailer, and code for classical gates
        Bool isLastBundle = &bundle == &bundles.back();
        codegen.bundleFinish(bundle.start_cycle, bundle.duration_in_cycles, isLastBundle);
    }   // for(bundles)

    QL_IOUT("Generating .vq1asm for bundles [Done]");
}
#endif


} // namespace detail
} // namespace vq1asm
} // namespace gen
} // namespace pass
} // namespace cc
} // namespace arch
} // namespace ql
