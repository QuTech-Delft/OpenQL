/** \file
 * Control-flow structure decomposition implementation (i.e. conversion to basic
 * block form).
 */

#include "ql/com/dec/structure.h"

#include "ql/utils/opt.h"
#include "ql/ir/ops.h"
#include "ql/ir/describe.h"
#include "ql/ir/consistency.h"

namespace ql {
namespace com {
namespace dec {

/**
 * Structure decomposition implementation.
 */
class StructureDecomposer {
private:

    /**
     * Reference to the root of the IR.
     */
    ir::Ref ir;

    /**
     * The blocks we've processed thus far. The back of the list is the block
     * that we're currently adding statements to. If this ends up empty when all
     * statements have been processed, all references to it (in goto
     * instructions and the blocks themselves) will be removed and the block
     * won't be added to the program.
     */
    utils::List<ir::BlockRef> blocks;

    /**
     * The entry point of the program within the blocks list.
     */
    ir::BlockRef entry_point;

    /**
     * Break statements should be turned into a goto to the block at the back of
     * this list. If empty, a break statement is illegal.
     */
    utils::List<ir::BlockRef> break_to;

    /**
     * Continue statements should be turned into a goto to the block at the back
     * of this list. If empty, a continue statement is illegal.
     */
    utils::List<ir::BlockRef> continue_to;

    /**
     * Forward references in the blocks in the block list may still point to the
     * blocks of the original program. This map tracks the mapping from (the
     * entry points of) the original program blocks to the new blocks. Once
     * the program has been fully converted to the blocks list, this mapping is
     * applied to all goto and next block targets.
     */
    utils::Map<ir::BlockRef, ir::BlockRef> remap;

    /**
     * The set of names already in use for blocks, used for name uniquification.
     */
    utils::Set<utils::Str> used_names;

    /**
     * Name stack for the original program as we're traversing it. The names of
     * new blocks are generated based on the name at the back of this list.
     */
    utils::List<utils::Str> name_stack;

    /**
     * Offset to apply to cycle numbers of incoming instructions to make the
     * cycle numbers consistent with the new block structure.
     */
    utils::Int cycle_offset = 0;

    /**
     * Incoming cycle number of the previous statement that we processed.
     */
    utils::Int previous_cycle = 0;

    /**
     * Class that adds a suffix to name_stack while it's in scope using RAII.
     */
    class NameSuffix {
    private:
        StructureDecomposer &sd;
    public:
        NameSuffix(StructureDecomposer &sd, const utils::Str &suffix);
        ~NameSuffix();
    };

    /**
     * Class that handles initialization and finalization of a loop structure.
     * After construction:
     *  - processed statements are added to the loop body;
     *  - the back of continue_to points to the first block of the loop body;
     *    and
     *  - the back of break_to points to the block that will come after the
     *    loop body.
     * When destroyed by RAII, the continue_to and break_to stacks are popped,
     * and the break_to block is pushed to the block list, so subsequently
     * added statements will come after the loop. Name suffixes are also handled
     * by this class.
     */
    class LoopBody {
    private:
        StructureDecomposer &sd;
        utils::Opt<NameSuffix> ns;
        utils::Int cycle_offset_after_loop;
    public:
        LoopBody(StructureDecomposer &sd, const utils::Str &suffix);
        void start_loop_condition();
        ~LoopBody();
    };

    /**
     * Makes a new block with the given name (uniquified if it already existed).
     * If requested (which is the default), also adds it to the back of the
     * blocks list, and if the previous block doesn't link to anything yet,
     * links it to the new block. Otherwise, we just return the block.
     */
    ir::BlockRef new_block(utils::Bool also_add = true);

    /**
     * Adds an instruction to the back of the blocks list, handling the cycle
     * numbers and basic block invariants (i.e. if the last block ends in a goto
     * instruction, we make a new block first).
     */
    void process_instruction(const ir::InstructionRef &insn);

    /**
     * Processes an instruction that was created as part of structure expansion.
     * That is, an instruction that doesn't have a valid cycle number yet.
     */
    void process_new_instruction(const ir::InstructionRef &insn);

    /**
     * Processes a statement from an incoming block, adding it to a block in the
     * blocks list.
     */
    void process_statement(const ir::StatementRef &stmt);

    /**
     * Processes the statements of a block, handling cycle numbers accordingly.
     */
    void process_block_base(const ir::BlockBaseRef &block);

    /**
     * Processes a toplevel block in the original program.
     */
    void process_block(const ir::BlockRef &block);

    /**
     * For the given block reference, which may be a reference to a block in the
     * original IR or in the new blocks list, convert to the appropriate block
     * in the blocks list in the former case. The incoming block reference may
     * also be empty in case of block->next (implying end of program), in which
     * case we also return empty.
     */
    ir::BlockRef update_block_reference(const ir::BlockRef &block);

    /**
     * Processes the program for the given IR node. This must only be called
     * once!
     */
    ir::ProgramRef process_program(const ir::Ref &incoming_ir);

    /**
     * Default constructor.
     */
    StructureDecomposer() = default;

public:

    /**
     * Runs structure decomposition.
     */
    static ir::ProgramRef run(const ir::Ref &ir);

};

StructureDecomposer::NameSuffix::NameSuffix(
    StructureDecomposer &sd,
    const utils::Str &suffix
) : sd(sd) {
    if (sd.name_stack.empty()) {
        sd.name_stack.push_back(suffix);
    } else {
        sd.name_stack.push_back(sd.name_stack.back() + "_" + suffix);
    }
}

StructureDecomposer::NameSuffix::~NameSuffix() {
    sd.name_stack.pop_back();
}

StructureDecomposer::LoopBody::LoopBody(
    StructureDecomposer &sd,
    const utils::Str &suffix
) : sd(sd) {

    // Make the block that comes after the loop using the current name
    // suffix.
    auto after_loop = sd.new_block(false);
    cycle_offset_after_loop = -sd.previous_cycle;

    // Update the name for the loop body blocks.
    ns.emplace(sd, suffix);

    // Make the block that evaluates the loop condition and jumps back
    // if true.
    auto loop_condition_block = sd.new_block(false);

    // Create the (first) loop body block.
    sd.new_block();

    // Update the break/continue stacks.
    sd.continue_to.push_back(loop_condition_block);
    sd.break_to.push_back(after_loop);

}

void StructureDecomposer::LoopBody::start_loop_condition() {

    // Connect the (last) loop body block to the loop condition block.
    auto loop_condition_block = sd.continue_to.back();
    sd.blocks.back()->next = loop_condition_block;

    // Add the loop condition block to the block list.
    sd.blocks.push_back(loop_condition_block);
    cycle_offset_after_loop = -sd.previous_cycle;

}

StructureDecomposer::LoopBody::~LoopBody() {

    // Add the first block that comes after the loop to the block list.
    sd.blocks.push_back(sd.break_to.back());
    sd.cycle_offset = cycle_offset_after_loop;

    // Clean up the break/continue stacks.
    sd.break_to.pop_back();
    sd.continue_to.pop_back();

}

/**
 * Makes a new block with the given name (uniquified if it already existed).
 * If requested (which is the default), also adds it to the back of the
 * blocks list, and if the previous block doesn't link to anything yet,
 * links it to the new block. Otherwise, we just return the block.
 */
ir::BlockRef StructureDecomposer::new_block(utils::Bool also_add) {

    // Make a unique name for the block.
    utils::Str name;
    if (name_stack.empty()) {
        name = "unknown";
    } else {
        name = name_stack.back();
    }
    auto unique_name = name;
    if (!used_names.insert(name).second) {
        utils::UInt i = 1;
        do {
            unique_name = name + "_" + utils::to_string(i++);
        } while (!used_names.insert(unique_name).second);
    }

    // Make a new block with that name.
    auto new_block = utils::make<ir::Block>(unique_name);
    if (!also_add) return new_block;

    // Link the previous block to it by default.
    if (!blocks.empty() && blocks.back()->next.empty()) {
        blocks.back()->next = new_block;
    }

    // Cycle numbers need to restart from zero, but we might still be taking
    // instructions from the same block at the input as before. So we need
    // to update the cycle offset accordingly.
    cycle_offset = -previous_cycle;

    // Add the new block to the back of the list and return it.
    blocks.push_back(new_block);
    return new_block;
}

/**
 * Adds an instruction to the back of the blocks list, handling the cycle
 * numbers and basic block invariants (i.e. if the last block ends in a goto
 * instruction, we make a new block first).
 */
void StructureDecomposer::process_instruction(const ir::InstructionRef &insn) {

    // Make a new block if the current block already ends in a goto
    // instruction.
    if (
        !blocks.back()->statements.empty() &&
        blocks.back()->statements.back()->as_goto_instruction()
    ) {
        new_block();
    }

    // Add the instruction to the last block.
    insn->cycle += cycle_offset;
    blocks.back()->statements.add(insn);

}

/**
 * Processes an instruction that was created as part of structure expansion.
 * That is, an instruction that doesn't have a valid cycle number yet.
 */
void StructureDecomposer::process_new_instruction(const ir::InstructionRef &insn) {
    cycle_offset = (utils::Int)ir::get_duration_of_block(blocks.back());
    insn->cycle = 0;
    process_statement(insn);
}

/**
 * Processes a statement from an incoming block, adding it to a block in the
 * blocks list.
 */
void StructureDecomposer::process_statement(const ir::StatementRef &stmt) {
    auto incoming_cycle = stmt->cycle;
    if (stmt->as_instruction()) {
        process_instruction(stmt.as<ir::Instruction>());
    } else if (auto ie = stmt->as_if_else()) {
        auto ns = NameSuffix(*this, "if");

        // Make the instruction that will conditionally jump to the block
        // for the branch and process them.
        utils::Vec<utils::One<ir::GotoInstruction>> branch_insns;
        for (const auto &br : ie->branches) {
            auto branch_insn = utils::make<ir::GotoInstruction>();
            branch_insn->condition = br->condition;
            branch_insns.push_back(branch_insn);
            process_new_instruction(branch_insn);
        }

        // Process the otherwise block, if any.
        if (!ie->otherwise.empty()) {
            process_block_base(ie->otherwise);
        }

        // Remember the current last block; we'll have to link it to the
        // block that will follow the branch bodies.
        utils::List<ir::BlockRef> bodies;
        bodies.push_back(blocks.back());

        // Create the branch bodies.
        QL_ASSERT(ie->branches.size() == branch_insns.size());
        for (utils::UInt i = 0; i < ie->branches.size(); i++) {
            branch_insns[i]->target = new_block();
            process_block_base(ie->branches[i]->body);
            bodies.push_back(blocks.back());
        }

        // Make the block that will follow everything and link up its
        // predecessors.
        new_block();
        for (const auto &body : bodies) {
            body->next = blocks.back();
        }

    } else if (auto sl = stmt->as_static_loop()) {

        // Generate the loop as follows:
        //
        //   ...
        //   set lhs = frm
        //   goto .body
        // .update
        //   set lhs = lhs +/- 1
        // .body
        //   <loop body>
        //   cond (lhs != to) goto .update
        // .after
        //   ...

        // Initialize the loop variable.
        process_new_instruction(ir::make_set_instruction(ir, sl->lhs->clone(), sl->frm));
        auto before = blocks.back();

        // Open the loop and add the update assignment.
        auto lb = LoopBody(*this, "foreach");
        auto start_of_loop = blocks.back();
        process_new_instruction(ir::make_set_instruction(
            ir,
            sl->lhs.clone(),
            ir::make_function_call(
                ir,
                (sl->to->value > sl->frm->value) ? "operator+" : "operator-",
                {
                    sl->lhs,
                    ir::make_int_lit(
                        ir,
                        1,
                        sl->lhs->target->data_type
                    )
                }
            )
        ));

        // Loop entry must skip the initial update assignment, so we need to
        // make a new block and link "before" accordingly.
        before->next = new_block();

        // Handle the loop body.
        process_block_base(sl->body);

        // Continue if the loop var equals the target value.
        lb.start_loop_condition();
        auto branch_insn = utils::make<ir::GotoInstruction>();
        branch_insn->condition = ir::make_function_call(
            ir,
            "operator!=",
            {sl->lhs->clone(), sl->to}
        );
        branch_insn->target = start_of_loop;
        process_new_instruction(branch_insn);

        // Break otherwise.
        blocks.back()->next = break_to.back();

    } else if (auto fl = stmt->as_for_loop()) {

        // Generate the loop as follows:
        //
        //   ...
        //   <initialize>
        //   cond (!<condition>) goto .after
        //   goto .body
        // .update
        //   <update>
        // .body
        //   <loop body>
        //   cond (<condition>) goto .update
        // .after
        //   ...

        // Handle the initializing assignment.
        if (!fl->initialize.empty()) {
            process_new_instruction(fl->initialize);
        }

        // Jump past the loop if the condition is already false.
        auto branch_past_insn = utils::make<ir::GotoInstruction>();
        branch_past_insn->condition = ir::make_function_call(
            ir,
            "operator!",
            {fl->condition.clone()}
        );
        process_new_instruction(branch_past_insn);

        // Remember this block to link up its successor.
        auto before = blocks.back();

        // Open the loop.
        auto lb = LoopBody(*this, (fl->initialize.empty() && fl->update.empty()) ? "while" : "for");
        auto start_of_loop = blocks.back();

        // Link up the skip-loop branch target now that the block after the
        // loop has been created by LoopBody.
        branch_past_insn->target = break_to.back();

        // Handle the update assignment.
        if (!fl->update.empty()) {
            process_new_instruction(fl->update);

            // Loop entry must skip the initial update assignment, so we
            // need to make a new block.
            new_block();

        }

        // Link "before" to whatever block is now at the back. If there is
        // no update assignment this will be the first block of the body,
        // otherwise it will be the block after that.
        before->next = blocks.back();

        // Handle the loop body.
        process_block_base(fl->body);

        // Continue if the loop var equals the target value.
        lb.start_loop_condition();
        auto branch_back_insn = utils::make<ir::GotoInstruction>();
        branch_back_insn->condition = fl->condition;
        branch_back_insn->target = start_of_loop;
        process_new_instruction(branch_back_insn);

        // Break otherwise.
        blocks.back()->next = break_to.back();

    } else if (auto ru = stmt->as_repeat_until_loop()) {

        // Generate the loop as follows:
        //
        //   ...
        // .body
        //   <loop body>
        //   cond (!<condition>) goto .body
        // .after
        //   ...

        // Open the loop.
        auto lb = LoopBody(*this, "repeat_until");
        auto start_of_loop = blocks.back();

        // Handle the loop body.
        process_block_base(ru->body);

        // Continue if the condition is false.
        lb.start_loop_condition();
        auto branch_insn = utils::make<ir::GotoInstruction>();
        branch_insn->condition = ir::make_function_call(
            ir,
            "operator!",
            {ru->condition}
        );
        branch_insn->target = start_of_loop;
        process_new_instruction(branch_insn);

        // Break otherwise.
        blocks.back()->next = break_to.back();

    } else if (stmt->as_loop_control_statement()) {

        // Handle the statement.
        if (stmt->as_break_statement()) {
            if (break_to.empty()) {
                QL_USER_ERROR("encountered break statement outside of a loop");
            }
            blocks.back()->next = break_to.back();
        } else if (stmt->as_continue_statement()) {
            if (continue_to.empty()) {
                QL_USER_ERROR("encountered continue statement outside of a loop");
            }
            blocks.back()->next = continue_to.back();
        } else {
            QL_ASSERT(false);
        }

        // We need to make a new block, because otherwise all other logic
        // will fall apart/need special cases. But this block won't be
        // reachable, so it will probably be optimized out.
        new_block(true);

    } else {
        QL_ASSERT(false);
    }
    previous_cycle = incoming_cycle;
}

/**
 * Processes the statements of a block, handling cycle numbers accordingly.
 */
void StructureDecomposer::process_block_base(const ir::BlockBaseRef &block) {

    // The incoming block starts at cycle zero per IR conventions, but the
    // current block may already have instructions in it. The incoming
    // instructions have to start after those.
    cycle_offset = (utils::Int)ir::get_duration_of_block(blocks.back());

    // Process the statements in the block.
    for (const auto &stmt : block->statements) {
        process_statement(stmt);
    }

}

/**
 * Processes a toplevel block in the original program.
 */
void StructureDecomposer::process_block(const ir::BlockRef &block) {

    // Handle naming of the blocks.
    QL_ASSERT(name_stack.empty());
    name_stack.push_back(block->name);

    // Just to be sure, check that there's no open loop (which is logically
    // impossible for toplevel blocks).
    QL_ASSERT(break_to.empty());
    QL_ASSERT(continue_to.empty());

    // Make sure all toplevel blocks start with a new block in the result,
    // because there might be incoming edges. We also need to remember to
    // rename those references.
    auto start = new_block();
    remap.insert({block, start});

    // If this block is the entry point, store the equivalent entry point
    // in the basic block form.
    if (ir->program->entry_point.links_to(block)) {
        entry_point = start;
    }

    // Handle the contents of the block.
    process_block_base(block);

    // Don't forget about the target of the original block.
    blocks.back()->next = block->next;

    // Make sure everything is appropriately closed again now.
    name_stack.pop_back();
    QL_ASSERT(name_stack.empty());
    QL_ASSERT(break_to.empty());
    QL_ASSERT(continue_to.empty());

}

/**
 * For the given block reference, which may be a reference to a block in the
 * original IR or in the new blocks list, convert to the appropriate block
 * in the blocks list in the former case. The incoming block reference may
 * also be empty in case of block->next (implying end of program), in which
 * case we also return empty.
 */
ir::BlockRef StructureDecomposer::update_block_reference(const ir::BlockRef &block) {
    if (block.empty()) {
        return {};
    }
    auto it = remap.find(block);
    if (it != remap.end()) {
        return it->second;
    } else {
        return block;
    }
}

/**
 * Processes the program for the given IR node. This must only be called
 * once!
 */
ir::ProgramRef StructureDecomposer::process_program(const ir::Ref &incoming_ir) {

    // Save the IR node for further processing.
    QL_ASSERT(ir.empty());
    ir = incoming_ir;

    // Special case for empty programs.
    if (ir->program.empty()) {
        return {};
    }

    // Handle the blocks in the program.
    QL_ASSERT(blocks.empty());
    for (const auto &block : ir->program->blocks) {
        process_block(block);
    }

    // Point all the goto and next block targets to the new blocks list.
    for (const auto &block : blocks) {
        block->next = update_block_reference(block->next.as_mut());
        for (const auto &stmt : block->statements) {
            if (auto gi = stmt->as_goto_instruction()) {
                gi->target = update_block_reference(gi->target.as_mut());
            }
        }
    }

    // Make the new program node.
    auto new_program = ir->program.copy();
    new_program->copy_annotations(*ir->program);
    new_program->entry_point = entry_point;
    new_program->blocks.reset();
    for (const auto &block : blocks) {
        new_program->blocks.add(block);
    }

    return std::move(new_program);
}

/**
 * Runs structure decomposition.
 */
ir::ProgramRef StructureDecomposer::run(const ir::Ref &ir) {
    return StructureDecomposer().process_program(ir);
}

/**
 * Decomposes the control-flow structure of the program in the given IR such
 * that it is in basic block form. Specifically:
 *
 *  - all blocks consist of only instructions (no control-flow statements like
 *    loops or if-conditionals); and
 *  - only the last instruction of each block may be a goto instruction.
 *
 * The ir tree is not modified. Instead, a new program node is returned. This
 * node is such that the original program node in ir can be replaced with it.
 * Note that nodes/subtrees may be shared between the structured and basic block
 * representations of the programs.
 *
 * If check is set, a consistency and basic-block form check is done before
 * returning the created program. This is also done if debugging is enabled via
 * the loglevel.
 */
ir::ProgramRef decompose_structure(const ir::Ref &ir, utils::Bool check) {
    auto program = StructureDecomposer::run(ir);

    // If we're in debug mode, check postconditions.
    if (QL_IS_LOG_DEBUG || check) {
        auto new_ir = ir.copy();
        new_ir->program = program;
        ir::check_consistency(new_ir);
        check_basic_block_form(program);
    };

    return program;
}

/**
 * Checks whether the given program is in basic block form, as defined by
 * decompose_structure(). If yes, an empty string is returned. Otherwise a
 * string with an appropriate message is returned.
 */
static utils::Str check_basic_block_form_str(const ir::ProgramRef &program) {
    for (const auto &block : program->blocks) {
        for (utils::UInt i = 0; i < block->statements.size(); i++) {
            const auto &stmt = block->statements[i];
            if (!stmt->as_instruction()) {
                return
                    "in block " + block->name + ": "
                    "found non-instruction: " + ir::describe(stmt);
            }
            if (stmt->as_goto_instruction() && i < block->statements.size() - 1) {
                return
                    "in block " + block->name + ": "
                    "found goto statement not at the end of the block: " +
                    ir::describe(stmt);
            }
        }
    }
    return {};
}

/**
 * Returns whether the given program is in basic block form, as defined by
 * decompose_structure(). Assumes that the program is otherwise consistent.
 */
utils::Bool is_in_basic_block_form(const ir::ProgramRef &program) {
    return check_basic_block_form_str(program).empty();
}

/**
 * Throws an appropriate exception if the given program is not in basic block
 * form. Assumes that the program is otherwise consistent.
 */
void check_basic_block_form(const ir::ProgramRef &program) {
    auto s = check_basic_block_form_str(program);
    if (!s.empty()) {
        QL_USER_ERROR(s);
    }
}

} // namespace dec
} // namespace com
} // namespace ql
