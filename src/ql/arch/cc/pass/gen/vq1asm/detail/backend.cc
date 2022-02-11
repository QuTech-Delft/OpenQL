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
    //  - ir->program->entry_point.links_to

    // generate code for all blocks
    // NB: based on NewToOldConverter::NewToOldConverter
    for (const auto &block : ir->program->blocks) {
        try {
            codegen_block(block, block->name, 0);
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

    // write map to file (unless we were using input file)
    if (options->map_input_file.empty()) {
        Str file_name_map(options->output_prefix + ".map");
        QL_IOUT("Writing instrument map to " << file_name_map);
        OutFile(file_name_map).write(codegen.getMap());
    }

    QL_DOUT("Compiling Central Controller program [Done]");
}


/*
 * Generate code for a single block (which sort of matches the concept of a Kernel in the old API). Recursively calls
 * itself where necessary.
 * Based on NewToOldConverter::convert_block
 */
// FIXME: runOnce automatically on cQASM input
// FIXME: provide (more) context in all QL_ICE and e.add_context
// FIXME: process block->next? And entrypoint?
void Backend::codegen_block(const ir::BlockBaseRef &block, const Str &name, Int depth)
{
    // Return the name for a child of this block. We use "__" to prevent clashes with names assigned by user (assuming
    // they won't use names with "__", similar to the C rule for identifiers), and give some sense of hierarchy
    // level.
    auto block_child_name = [name](const Str &child_name) { return "__" + name + "__" + child_name; };

    // Return the label (stem) for this block. Note that this is used as q1asm label and must adhere to
    // the allowed structure of that. The appending of block_number is to uniquify anonymous blocks like for loops.
    auto label = [this, name]() { return QL_SS2S(name << "__" << block_number); };

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

    QL_IOUT("compiling block '" + label() + "'");
    codegen.block_start(name, depth);

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

            // Handle the instruction subtypes.
            if (auto cinsn = stmt->as_conditional_instruction()) {

                //****************************************************************
                // Instruction: conditional
                //****************************************************************

                // Handle the conditional instruction subtypes.
                if (auto custom = cinsn->as_custom_instruction()) {

                    QL_DOUT("custom instruction: name=" + custom->instruction_type->name);
                    try {
                        codegen.custom_instruction(*custom);
                    } catch (utils::Exception &e) {
                        e.add_context(QL_SS2S("in custom instruction '" << ir::describe(*custom) << "'"), true);
                        throw;
                    }

                } else if (auto set_instruction = cinsn->as_set_instruction()) {

                    //****************************************************************
                    // Instruction: set
                    //****************************************************************
                    CHECK_COMPAT(
                        set_instruction->condition->as_bit_literal()
                        && set_instruction->condition->as_bit_literal()->value
                        , "conditions other then 'true' are not supported for set instruction"
                    );
                    try {
                        codegen.handle_set_instruction(*set_instruction, "conditional.set");
                    } catch (utils::Exception &e) {
                        e.add_context(QL_SS2S("in set_instruction '" << ir::describe(*set_instruction) << "'") , true);
                        throw;
                    }

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

        } else if (stmt->as_structured()) {

            //****************************************************************
            // Statement: structured
            //****************************************************************
            QL_IOUT("structured: " + ir::describe(stmt));

            // All structured statements except loop_control_statement (break/continue) contain at least one sub-block.
            // Every (scheduled) block restarts cycle numbers from zero, because a block constitutes a scheduling realm.
            // Any statements _after_ a sub-block _also_ restart numbering from zero, which makes a lot of sense.
            // We handle that by wrapping the relevant structured statements in block_finish/block_start, as if the
            // different parts were in separate blocks

            if(!stmt->as_loop_control_statement()) {
                // FIXME: add part number to name
                codegen.block_finish(name, ir::get_duration_of_block(block), depth);    // FIXME: duration for full block, excluding sub-blocks
            }

            // Handle the different types of structured statements.
            if (auto if_else = stmt->as_if_else()) {

                Str saved_label = label();  // changes when recursing

                // Handle if-else (or just if) statement.
                for(UInt branch=0; branch<if_else->branches.size(); branch++) {
                    // if-condition
                    try {
                        codegen.if_elif(if_else->branches[branch]->condition, saved_label, branch);
                    } catch (utils::Exception &e) {
                        e.add_context("in 'if' condition", true);
                        throw;
                    }

                    // if-block
                    try {
                        codegen_block(if_else->branches[branch]->body, block_child_name(QL_SS2S("if_elif_"<<branch)), depth+1);
                    } catch (utils::Exception &e) {
                        e.add_context("in 'if_elif' block", true);
                        throw;
                    }
                }

                // otherwise
                codegen.if_otherwise(saved_label, if_else->branches.size());   // NB: doesn't throw exceptions

                // otherwise-block
                if (!if_else->otherwise.empty()) {
                    try {
                        codegen_block(if_else->otherwise, block_child_name("otherwise"), depth+1);
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
                    codegen_block(static_loop->body, block_child_name("static_for"), depth+1);
                } catch (utils::Exception &e) {
                    e.add_context("in static loop body", true);
                    throw;
                }

                // end:
                codegen.foreach_end(*static_loop->lhs, *static_loop->frm, *static_loop->to, loop_label.back());
                loop_label.pop_back();

            } else if (auto repeat_until_loop = stmt->as_repeat_until_loop()) {

                // Handle repeat-until loops.
                loop_label.push_back(label());          // remind label for break/continue, before recursing
                codegen.repeat(loop_label.back());

                try {
                    codegen_block(repeat_until_loop->body, block_child_name("repeat_until"), depth+1);
                } catch (utils::Exception &e) {
                    e.add_context("in repeat-until loop body", true);
                    throw;
                }

                codegen.until(repeat_until_loop->condition, loop_label.back());
                loop_label.pop_back();

            } else if (auto for_loop = stmt->as_for_loop()) {

                // NB: 'while' statements are also represented as a for_loop in the IR
                // for loop: start
                loop_label.push_back(label());          // remind label for break/continue, before recursing
                codegen.for_start(for_loop->initialize, for_loop->condition, loop_label.back());

                // handle body
                try {
                    codegen_block(for_loop->body, block_child_name("body"), depth+1);
                } catch (utils::Exception &e) {
                    e.add_context("in for/while loop body", true);
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

            if(!stmt->as_loop_control_statement()) {
                // FIXME: reopen block, see comment
                // FIXME: add part number to name
                codegen.block_start(name, depth);
            }

        } else {
            QL_ICE(
                "unsupported statement type encountered"
                << "'" <<ir::describe(stmt) << "'"
            );
        }
    }

    // Flush any pending bundle.
    if (is_bundle_open) {
        bundle_finish(true);
    }

    codegen.block_finish(name, ir::get_duration_of_block(block), depth);    // FIXME: duration for full block, excluding sub-blocks
    QL_IOUT("finished compiling block '" + label() + "'");
    block_number++;

}

} // namespace detail
} // namespace vq1asm
} // namespace gen
} // namespace pass
} // namespace cc
} // namespace arch
} // namespace ql
