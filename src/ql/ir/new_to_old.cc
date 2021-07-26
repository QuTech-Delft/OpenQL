/** \file
 * Provides the conversion from the new one to the old one for compatibility and
 * testing purposes.
 */

#include "ql/ir/new_to_old.h"

#include "ql/ir/old_to_new.h"
#include "ql/ir/ops.h"
#include "ql/ir/describe.h"
#include "ql/arch/diamond/annotations.h"

namespace ql {
namespace ir {

/**
 * Helper macro for QL_ICE() that throws when the given condition is not
 * true.
 */
#define CHECK_COMPAT(val, s) do { if (!(val)) QL_ICE(s); } while (false)

/**
 * Implementation of the new-to-old conversion.
 */
class NewToOldConverter {
private:
    friend class Operands;

    /**
     * The root of the new IR structure that serves as our input.
     */
    Ref ir;

    /**
     * The root of the old IR structure being built.
     */
    compat::ProgramRef old;

    /**
     * In the old IR, all blocks have (unique) names, whereas in the new one
     * only the toplevel blocks do. That means we'll have to infer unique names.
     * This is the set of names that has been used thus far.
     */
    utils::Set<utils::Str> kernel_names;

    /**
     * The number of qubits.
     */
    utils::UInt num_qubits;

    /**
     * The object used by the new IR to refer to bregs from num_qubits onwards.
     */
    ObjectLink breg_ob;

    /**
     * The object used by the new IR to refer to cregs.
     */
    ObjectLink creg_ob;

    /**
     * Makes a unique kernel/program name based on the name of the given block,
     * if any.
     */
    utils::Str make_kernel_name(
        const BlockBaseRef &block
    );

    /**
     * Converts a bit reference to its breg index.
     */
    utils::UInt convert_breg_reference(
        const ExpressionRef &ref
    );

    /**
     * Converts a creg reference to a compat::ClassicalRegister.
     */
    compat::ClassicalRegister convert_creg_reference(
        const ExpressionRef &ref
        );

    /**
     * Converts a condition for structured control-flow to a
     * compat::ClassicalOperation.
     */
    compat::ClassicalOperation convert_classical_condition(
        const ExpressionRef &ref,
        utils::Bool invert
    );

    /**
     * Adds the given (sub)block to a compat::Program.
     */
    void convert_block(
        const BlockBaseRef &block,
        const compat::ProgramRef &program
    );

    /**
     * Private constructor for the new-to-old conversion object. This actually
     * does the conversion.
     */
    NewToOldConverter(const Ref &ir);

public:

    /**
     * Public entry point for the conversion.
     */
    static compat::ProgramRef convert(const Ref &ir);

};

/**
 * Handles gathering the operands for a gate in the legacy format.
 */
class Operands {
public:

    /**
     * Qubit operand indices.
     */
    utils::Vec<utils::UInt> qubits;

    /**
     * Creg operand indices.
     */
    utils::Vec<utils::UInt> cregs;

    /**
     * Breg operand indices.
     */
    utils::Vec<utils::UInt> bregs;

    /**
     * Angle operand existence.
     */
    utils::Bool has_angle = false;

    /**
     * Angle operand value.
     */
    utils::Real angle = 0.0;

    /**
     * Integer operand existence.
     */
    utils::Bool has_integer = false;

    /**
     * Integer operand value.
     */
    utils::Int integer = 0;

    /**
     * Appends an operand.
     */
    void append(const NewToOldConverter &conv, const ExpressionRef &expr);

};

/**
 * Makes a unique kernel/program name based on the name of the given block,
 * if any.
 */
utils::Str NewToOldConverter::make_kernel_name(const BlockBaseRef &block) {
    utils::Str name;
    if (auto kn = block->get_annotation_ptr<KernelName>()) {
        name = kn->name;
    } else if (auto b = block->as_block()) {
        name = b->name;
    }
    if (!name.empty() && kernel_names.insert(name).second) {
        return name;
    }
    for (utils::UInt i = 1; true; i++) {
        auto unique_name = name + "_" + utils::to_string(i);
        if (kernel_names.insert(unique_name).second) {
            return unique_name;
        }
    }
}

/**
 * Converts a bit reference to its breg index.
 */
utils::UInt NewToOldConverter::convert_breg_reference(
    const ExpressionRef &ref
) {
    Operands ops;
    ops.append(*this, ref);
    CHECK_COMPAT(
        ops.bregs.size() == 1,
        "expected bit reference (breg), but got something else"
    );
    return ops.bregs[0];
}

/**
 * Converts a creg reference to a compat::ClassicalRegister.
 */
compat::ClassicalRegister NewToOldConverter::convert_creg_reference(
    const ExpressionRef &ref
) {
    auto lhs = ref->as_reference();
    CHECK_COMPAT(
        lhs &&
        lhs->target == creg_ob &&
        lhs->data_type == creg_ob->data_type &&
        lhs->indices.size() == 1 &&
        lhs->indices[0]->as_int_literal(),
        "expected creg reference, but got something else"
    );
    return compat::ClassicalRegister(lhs->indices[0]->as_int_literal()->value);
}

/**
 * Converts a condition for structured control-flow to a
 * compat::ClassicalOperation.
 */
compat::ClassicalOperation NewToOldConverter::convert_classical_condition(
    const ExpressionRef &ref,
    utils::Bool invert
) {
    auto fn = ref->as_function_call();
    CHECK_COMPAT(
        fn &&
        fn->operands.size() == 2,
        "expected classical relational operator, but got something else"
    );
    utils::Str operation;
    if (fn->function_type->name == "operator==") {
        operation = invert ? "!=" : "==";
    } else if (fn->function_type->name == "operator!=") {
        operation = invert ? "==" : "!=";
    } else if (fn->function_type->name == "operator<") {
        operation = invert ? ">=" : "<";
    } else if (fn->function_type->name == "operator<=") {
        operation = invert ? ">" : "<=";
    } else if (fn->function_type->name == "operator>") {
        operation = invert ? "<=" : ">";
    } else if (fn->function_type->name == "operator>=") {
        operation = invert ? "<" : ">=";
    } else {
        QL_ICE("expected classical relational operator, but got something else");
    }
    try {
        return compat::ClassicalOperation(
            convert_creg_reference(fn->operands[0]),
            operation,
            convert_creg_reference(fn->operands[1])
        );
    } catch (utils::Exception &e) {
        e.add_context("classical relational operator function", true);
        throw;
    }
}

/**
 * Adds the given (sub)block to a compat::Program.
 */
void NewToOldConverter::convert_block(
    const BlockBaseRef &block,
    const compat::ProgramRef &program
) {

    // Gather instructions immediately in this block into a
    // lazily-constructed kernel, to be flushed to program when a structured
    // control-flow statement appears, or at the end.
    compat::KernelRef kernel;

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

    // Loop over the statements and handle them individually.
    for (const auto &stmt : block->statements) {
        if (auto insn = stmt->as_instruction()) {

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

            // Handle the instruction subtypes.
            if (auto cinsn = stmt->as_conditional_instruction()) {

                // Handle the condition.
                try {
                    utils::Vec<utils::UInt> cond_operands;
                    compat::ConditionType cond_type;
                    if (auto blit = cinsn->condition->as_bit_literal()) {
                        if (blit->value) {
                            cond_type = compat::ConditionType::ALWAYS;
                        } else {
                            cond_type = compat::ConditionType::NEVER;
                        }
                    } else if (cinsn->condition->as_reference()) {
                        cond_operands.push_back(convert_breg_reference(cinsn->condition));
                        cond_type = compat::ConditionType::UNARY;
                    } else if (auto fn = cinsn->condition->as_function_call()) {
                        if (
                            fn->function_type->name == "operator!" ||
                            fn->function_type->name == "operator~"
                        ) {
                            CHECK_COMPAT(fn->operands.size() == 1, "unsupported condition function");
                            if (fn->operands[0]->as_reference()) {
                                cond_operands.push_back(convert_breg_reference(fn->operands[0]));
                                cond_type = compat::ConditionType::NOT;
                            } else if (auto fn2 = fn->operands[0]->as_function_call()) {
                                CHECK_COMPAT(fn2->operands.size() == 2, "unsupported condition function");
                                cond_operands.push_back(convert_breg_reference(fn2->operands[0]));
                                cond_operands.push_back(convert_breg_reference(fn2->operands[1]));
                                if (
                                    fn2->function_type->name == "operator&" ||
                                    fn2->function_type->name == "operator&&"
                                ) {
                                    cond_type = compat::ConditionType::NAND;
                                } else if (
                                    fn2->function_type->name == "operator|" ||
                                    fn2->function_type->name == "operator||"
                                ) {
                                    cond_type = compat::ConditionType::NOR;
                                } else if (
                                    fn2->function_type->name == "operator^" ||
                                    fn2->function_type->name == "operator^^" ||
                                    fn2->function_type->name == "operator!="
                                ) {
                                    cond_type = compat::ConditionType::NXOR;
                                } else if (
                                    fn2->function_type->name == "operator=="
                                ) {
                                    cond_type = compat::ConditionType::XOR;
                                } else {
                                    QL_ICE("unsupported gate condition");
                                }
                            } else {
                                QL_ICE("unsupported gate condition");
                            }
                        } else {
                            CHECK_COMPAT(fn->operands.size() == 2, "unsupported condition function");
                            cond_operands.push_back(convert_breg_reference(fn->operands[0]));
                            cond_operands.push_back(convert_breg_reference(fn->operands[1]));
                            if (
                                fn->function_type->name == "operator&" ||
                                fn->function_type->name == "operator&&"
                            ) {
                                cond_type = compat::ConditionType::AND;
                            } else if (
                                fn->function_type->name == "operator|" ||
                                fn->function_type->name == "operator||"
                            ) {
                                cond_type = compat::ConditionType::OR;
                            } else if (
                                fn->function_type->name == "operator^" ||
                                fn->function_type->name == "operator^^" ||
                                fn->function_type->name == "operator!="
                            ) {
                                cond_type = compat::ConditionType::XOR;
                            } else if (
                                fn->function_type->name == "operator=="
                            ) {
                                cond_type = compat::ConditionType::NXOR;
                            } else {
                                QL_ICE("unsupported condition function");
                            }
                        }
                    } else {
                        QL_ICE("unsupported condition expression");
                    }
                    kernel->gate_preset_condition(cond_type, cond_operands);
                } catch (utils::Exception &e) {
                    e.add_context("in gate condition", true);
                    throw;
                }

                // Handle the conditional instruction subtypes.
                if (auto custom = cinsn->as_custom_instruction()) {

                    // Handle special Diamond architecture gates that use more
                    // operands than the old IR could handle using annotations.
                    // The new IR exposes these operands as regular operands, so
                    // we need to copy them back into the annotations in case a
                    // pass changed them since the old-to-new conversion. Note
                    // that we still need the annotations to exist (despite not
                    // using their contents) to determine which special case to
                    // use!
                    utils::UInt diamond_op_count = 0;
                    if (custom->has_annotation<arch::diamond::annotations::ExciteMicrowaveParameters>()) {
                        diamond_op_count = 5;
                    } else if (custom->has_annotation<arch::diamond::annotations::MemSwapParameters>()) {
                        diamond_op_count = 1;
                    } else if (custom->has_annotation<arch::diamond::annotations::QEntangleParameters>()) {
                        diamond_op_count = 1;
                    } else if (custom->has_annotation<arch::diamond::annotations::SweepBiasParameters>()) {
                        diamond_op_count = 6;
                    } else if (custom->has_annotation<arch::diamond::annotations::CRCParameters>()) {
                        diamond_op_count = 2;
                    } else if (custom->has_annotation<arch::diamond::annotations::RabiParameters>()) {
                        diamond_op_count = 3;
                    }
                    if (diamond_op_count) {
                        CHECK_COMPAT(
                            custom->operands.size() >= diamond_op_count,
                            "Diamond arch gate " << custom->instruction_type->name <<
                            " must have at least " << diamond_op_count << " arguments"
                        );
                        utils::Vec<utils::UInt> diamond_ops;
                        diamond_ops.reserve(diamond_op_count);
                        for (
                            utils::UInt i = custom->operands.size() - diamond_op_count;
                            i < custom->operands.size();
                            i++
                        ) {
                            auto ilit = custom->operands[i]->as_int_literal();
                            CHECK_COMPAT(
                                ilit && ilit->value >= 0,
                                "operand " << i << " of Diamond arch gate " <<
                                custom->instruction_type->name << " must be an "
                                "unsigned integer literal"
                            );
                            diamond_ops.push_back((utils::UInt)ilit->value);
                        }
                        utils::UInt i = 0;
                        if (auto emp = custom->get_annotation_ptr<arch::diamond::annotations::ExciteMicrowaveParameters>()) {
                            emp->envelope    = diamond_ops[i++];
                            emp->duration    = diamond_ops[i++];
                            emp->frequency   = diamond_ops[i++];
                            emp->phase       = diamond_ops[i++];
                            emp->amplitude   = diamond_ops[i++];
                        } else if (auto msp = custom->get_annotation_ptr<arch::diamond::annotations::MemSwapParameters>()) {
                            msp->nuclear     = diamond_ops[i++];
                        } else if (auto qep = custom->get_annotation_ptr<arch::diamond::annotations::QEntangleParameters>()) {
                            qep->nuclear     = diamond_ops[i++];
                        } else if (auto sbp = custom->get_annotation_ptr<arch::diamond::annotations::SweepBiasParameters>()) {
                            sbp->value       = diamond_ops[i++];
                            sbp->dacreg      = diamond_ops[i++];
                            sbp->start       = diamond_ops[i++];
                            sbp->step        = diamond_ops[i++];
                            sbp->max         = diamond_ops[i++];
                            sbp->memaddress  = diamond_ops[i++];
                        } else if (auto cp = custom->get_annotation_ptr<arch::diamond::annotations::CRCParameters>()) {
                            cp->threshold    = diamond_ops[i++];
                            cp->value        = diamond_ops[i++];
                        } else if (auto rp = custom->get_annotation_ptr<arch::diamond::annotations::RabiParameters>()) {
                            rp->measurements = diamond_ops[i++];
                            rp->duration     = diamond_ops[i++];
                            rp->t_max        = diamond_ops[i++];
                        }
                        QL_ASSERT(i == diamond_op_count);
                    }

                    // Handle the normal operands for custom instructions.
                    Operands ops;
                    for (const auto &ob : custom->instruction_type->template_operands) {
                        try {
                            ops.append(*this, ob);
                        } catch (utils::Exception &e) {
                            e.add_context("name="+custom->instruction_type->name+", qubits="+ops.qubits.to_string());
                            throw;
                        }
                    }
                    for (utils::UInt i = 0; i < custom->operands.size() - diamond_op_count; i++) {
                        try {
                            ops.append(*this, custom->operands[i]);
                        } catch (utils::Exception &e) {
                            e.add_context(
                                "name=" + custom->instruction_type->name
                                + ", qubits=" + ops.qubits.to_string()
                                + ", operand=" + std::to_string(i)
                                );
                            throw;
                        }
                    }
                    kernel->gate(
                        custom->instruction_type->name, ops.qubits, ops.cregs,
                        0, ops.angle, ops.bregs
                    );
                    if (ops.has_integer) {
                        CHECK_COMPAT(
                            kernel->gates.size() == first_gate_index + 1,
                            "gate with integer operand cannot be ad-hoc decomposed"
                        );
                        kernel->gates.back()->int_operand = ops.integer;
                    }

                } else if (auto set = cinsn->as_set_instruction()) {

                    // Handle classical gates.
                    utils::Opt<compat::ClassicalRegister> lhs;
                    try {
                        lhs.emplace(convert_creg_reference(set->lhs));
                    } catch (utils::Exception &e) {
                        e.add_context("unsupported LHS for set instruction encountered");
                        throw;
                    }
                    try {
                        if (auto ilit = set->rhs->as_int_literal()) {
                            kernel->classical(
                                *lhs,
                                compat::ClassicalOperation(
                                    ilit->value
                                )
                            );
                        } else if (set->rhs->as_reference()) {
                            kernel->classical(
                                *lhs,
                                compat::ClassicalOperation(
                                    convert_creg_reference(set->rhs)
                                )
                            );
                        } else if (auto fn = set->rhs->as_function_call()) {
                            utils::Str operation;
                            utils::UInt operand_count = 2;
                            if (fn->function_type->name == "int") {
                                CHECK_COMPAT(
                                    fn->operands.size() == 1 &&
                                    fn->operands[0]->as_function_call(),
                                    "int() cast target must be a function"
                                );
                                fn = fn->operands[0]->as_function_call();
                            }
                            if (fn->function_type->name == "operator~") {
                                operation = "~";
                                operand_count = 1;
                            } else if (fn->function_type->name == "operator+") {
                                operation = "+";
                            } else if (fn->function_type->name == "operator-") {
                                operation = "-";
                            } else if (fn->function_type->name == "operator&") {
                                operation = "&";
                            } else if (fn->function_type->name == "operator|") {
                                operation = "|";
                            } else if (fn->function_type->name == "operator^") {
                                operation = "^";
                            } else if (fn->function_type->name == "operator==") {
                                operation = "==";
                            } else if (fn->function_type->name == "operator!=") {
                                operation = "!=";
                            } else if (fn->function_type->name == "operator>") {
                                operation = ">";
                            } else if (fn->function_type->name == "operator>=") {
                                operation = ">=";
                            } else if (fn->function_type->name == "operator<") {
                                operation = "<";
                            } else if (fn->function_type->name == "operator<=") {
                                operation = "<=";
                            } else {
                                QL_ICE(
                                    "no conversion known for function " << fn->function_type->name
                                );
                            }
                            CHECK_COMPAT(
                                fn->operands.size() == operand_count,
                                "function " << fn->function_type->name << " has wrong operand count"
                            );
                            if (operand_count == 1) {
                                kernel->classical(
                                    *lhs,
                                    compat::ClassicalOperation(
                                        operation,
                                        convert_creg_reference(fn->operands[0])
                                    )
                                );
                            } else if (fn->operands.size() == 2) {
                                kernel->classical(
                                    *lhs,
                                    compat::ClassicalOperation(
                                        convert_creg_reference(fn->operands[0]),
                                        operation,
                                        convert_creg_reference(fn->operands[1])
                                    )
                                );
                            } else {
                                QL_ASSERT(false);
                            }
                        } else {
                            QL_ICE(
                                "must be integer literal, creg reference, or simple "
                                "function of cregs"
                            );
                        }

                    } catch (utils::Exception &e) {
                        e.add_context("unsupported RHS for set instruction encountered");
                        throw;
                    }

                } else {
                    QL_ICE("unsupported instruction type encountered");
                }

                // Reset the gate condition.
                kernel->gate_clear_condition();

            } else if (auto wait = stmt->as_wait_instruction()) {

                // Handle wait instructions.
                Operands ops;
                for (const auto &ob : wait->objects) {
                    ops.append(*this, ob);
                }
                kernel->gate(
                    "wait", ops.qubits, ops.cregs,
                    wait->duration * old->platform->cycle_time,
                    ops.angle, ops.bregs
                );

            } else {
                QL_ICE("unsupported instruction type encountered");
            }

            // Copy gate annotations if adding the gate resulted in just one
            // gate.
            if (kernel->gates.size() == first_gate_index + 1) {
                kernel->gates[first_gate_index]->copy_annotations(*insn);
            }

            // Assign the cycle numbers for the new gates.
            for (auto i = first_gate_index; i < kernel->gates.size(); i++) {
                kernel->gates[i]->cycle = (utils::UInt)((utils::Int)insn->cycle + cycle_offset);
            }

        } else if (stmt->as_structured()) {

            // Flush any pending kernel not affected by control-flow.
            if (!kernel.empty()) {
                first_kernel = false;
                kernel->cycles_valid = cycles_valid;
                program->add(kernel);
                kernel.reset();
            }
            cycle_offset = utils::MAX;

            // Handle the different types of structured statements.
            if (auto if_else = stmt->as_if_else()) {

                // Handle if-else or if statement.
                CHECK_COMPAT(
                    if_else->branches.size() == 1,
                    "encountered if-else chain with multiple conditions"
                );
                compat::ProgramRef if_program;
                if_program.emplace(
                    make_kernel_name(block), old->platform,
                    old->qubit_count, old->creg_count, old->breg_count
                );
                try {
                    convert_block(if_else->branches[0]->body, if_program);
                } catch (utils::Exception &e) {
                    e.add_context("in 'if' block", true);
                    throw;
                }
                if (if_else->otherwise.empty()) {
                    try {
                        program->add_if(
                            if_program,
                            convert_classical_condition(
                                if_else->branches[0]->condition,
                                false
                            )
                        );
                    } catch (utils::Exception &e) {
                        e.add_context("in 'if' condition", true);
                        throw;
                    }
                } else {
                    compat::ProgramRef else_program;
                    else_program.emplace(
                        make_kernel_name(block), old->platform,
                        old->qubit_count, old->creg_count, old->breg_count
                    );
                    try {
                        convert_block(if_else->otherwise, else_program);
                    } catch (utils::Exception &e) {
                        e.add_context("in 'else' block", true);
                        throw;
                    }
                    try {
                        program->add_if_else(
                            if_program,
                            else_program,
                            convert_classical_condition(
                                if_else->branches[0]->condition,
                                false
                            )
                        );
                    } catch (utils::Exception &e) {
                        e.add_context("in 'if' condition", true);
                        throw;
                    }
                }

            } else if (auto static_loop = stmt->as_static_loop()) {

                // Handle static loops. Note that the old IR conceptually
                // doesn't have a loop variable for these, so the loop var can't
                // be a creg (or anything else that's referenced elsewhere as
                // well).
                CHECK_COMPAT(
                    static_loop->lhs->target != creg_ob,
                    "static loop variable cannot be a mapped creg"
                );
                compat::ProgramRef body;
                body.emplace(
                    make_kernel_name(block), old->platform,
                    old->qubit_count, old->creg_count, old->breg_count
                );
                try {
                    convert_block(static_loop->body, body);
                } catch (utils::Exception &e) {
                    e.add_context("in static loop body", true);
                    throw;
                }
                program->add_for(
                    body,
                    utils::abs<utils::Int>(
                        static_loop->to->value - static_loop->frm->value
                    ) + 1
                );

            } else if (auto repeat_until_loop = stmt->as_repeat_until_loop()) {

                // Handle repeat-until/do-while loops.
                compat::ProgramRef body;
                body.emplace(
                    make_kernel_name(block), old->platform,
                    old->qubit_count, old->creg_count, old->breg_count
                );
                try {
                    convert_block(repeat_until_loop->body, body);
                } catch (utils::Exception &e) {
                    e.add_context("in repeat-until/do-while loop body", true);
                    throw;
                }
                try {
                    program->add_do_while(
                        body,
                        convert_classical_condition(
                            repeat_until_loop->condition,
                            true
                        )
                    );
                } catch (utils::Exception &e) {
                    e.add_context("in repeat-until/do-while condition", true);
                    throw;
                }

            } else {
                QL_ICE("unsupported structured control-flow statement encountered");
            }

        } else {
            QL_ICE("unsupported statement type encountered");
        }
    }

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

}

/**
 * Private constructor for the new-to-old conversion object. This actually
 * does the conversion.
 */
NewToOldConverter::NewToOldConverter(const Ref &ir) : ir(ir) {

    // Build the platform. If there is a compat::PlatformRef annotation, as
    // there would be when convert_old_to_new() was used, use that structure
    // directly. Otherwise, build a new compat::Platform based on the raw
    // JSON data associated with the new platform. This is not foolproof
    // however, as architectures may preprocess the structure during
    // construction of the compat::Platform node, and this preprocessing
    // would already have happened to the raw JSON data associated with
    // ir->platform.
    compat::PlatformRef old_platform;
    if (ir->platform->has_annotation<compat::PlatformRef>()) {
        old_platform = ir->platform->get_annotation<compat::PlatformRef>();
    } else {
        old_platform = compat::Platform::build(
            ir->platform->name,
            ir->platform->data.data
        );
    }

    // If the program node is empty, build an empty dummy program.
    if (ir->program.empty()) {
        old.emplace("empty", old_platform, num_qubits);
        return;
    }

    // Determine number of qubits.
    CHECK_COMPAT(
        ir->platform->qubits->shape.size() == 1,
        "main qubit register has wrong dimensionality"
    );
    if (ir->program->has_annotation<ObjectUsage>()) {
        num_qubits = ir->program->get_annotation<ObjectUsage>().num_qubits;
    } else {
        num_qubits = ir->platform->qubits->shape[0];
    }

    // Determine number of bregs. The first num_qubits bregs are the implicit
    // bits associated with qubits, so there are always num_qubits of these.
    auto num_bregs = num_qubits;
    breg_ob = find_physical_object(ir, "breg");
    if (ir->program->has_annotation<ObjectUsage>()) {
        num_bregs = ir->program->get_annotation<ObjectUsage>().num_bregs;
    } else if (!breg_ob.empty()) {
        CHECK_COMPAT(
            breg_ob->shape.size() == 1,
            "breg register has has wrong dimensionality"
        );
        CHECK_COMPAT(
            breg_ob->data_type == ir->platform->default_bit_type,
            "breg register is not of the default bit type"
        );
        num_bregs += breg_ob->shape[0];
    }

    // Determine number of cregs.
    utils::UInt num_cregs = 0;
    creg_ob = find_physical_object(ir, "creg");
    if (ir->program->has_annotation<ObjectUsage>()) {
        num_cregs = ir->program->get_annotation<ObjectUsage>().num_cregs;
    } else if (!creg_ob.empty()) {
        CHECK_COMPAT(
            creg_ob->shape.size() == 1,
            "creg register has has wrong dimensionality"
        );
        CHECK_COMPAT(
            creg_ob->data_type == ir->platform->default_int_type,
            "creg register is not of the default integer type"
        );
        num_cregs += creg_ob->shape[0];
    }

    // Build the program/root node for the old IR.
    old.emplace(
        ir->program->name, old_platform,
        num_qubits, num_cregs, num_bregs
    );
    old->unique_name = ir->program->unique_name;

    // Copy program-wide annotations.
    old->copy_annotations(*ir->program);

    // Check that the blocks that constitute the program are ordered
    // linearly, with no control-flow in between; any goto-based control is
    // not supported by the old IR. After this, the only additional
    // requirement is that there are no goto instructions within the blocks.
    CHECK_COMPAT(
        ir->program->entry_point.links_to(ir->program->blocks[0]),
        "program has unsupported nontrivial goto-based control-flow: "
        "first block is not the entry point"
    );
    for (utils::UInt i = 0; i < ir->program->blocks.size() - 1; i++) {
        CHECK_COMPAT(
            ir->program->blocks[i]->next.links_to(ir->program->blocks[i + 1]),
            "program has unsupported nontrivial goto-based control-flow: "
            "block " << i << " does not link to next"
        );
    }
    CHECK_COMPAT(
        ir->program->blocks.back()->next.empty(),
        "program has unsupported nontrivial goto-based control-flow: "
        "last block does not end program"
    );

    // Convert all the blocks and add them to the root program.
    for (const auto &block : ir->program->blocks) {
        try {
            convert_block(block, old);
        } catch (utils::Exception &e) {
            e.add_context("in block \"" + block->name + "\"");
            throw;
        }
    }

}

/**
 * Public entry point for the conversion.
 */
compat::ProgramRef NewToOldConverter::convert(const Ref &ir) {
    try {
        return NewToOldConverter(ir).old;
    } catch (utils::Exception &e) {
        e.add_context("new-to-old IR conversion", true);
        throw;
    }
}

/**
 * Appends an operand.
 */
void Operands::append(const NewToOldConverter &conv, const ExpressionRef &expr) {
    if (auto real_lit = expr->as_real_literal()) {
        CHECK_COMPAT(!has_angle, "encountered gate with multiple angle (real) operands");
        has_angle = true;
        angle = real_lit->value;
    } else if (auto int_lit = expr->as_int_literal()) {
        CHECK_COMPAT(!has_integer, "encountered gate with multiple integer operands");
        has_integer = true;
        integer = int_lit->value;
    } else if (auto ref = expr->as_reference()) {
        if (ref->indices.size() != 1 || !ref->indices[0]->as_int_literal()) {
            QL_ICE(
                "encountered incompatible object reference to "
                << ref->target->name
                << " (size=" << ref->indices.size() << ")"
            );
        } else if (
            ref->target == conv.ir->platform->qubits &&
            ref->data_type == conv.ir->platform->qubits->data_type
        ) {
            qubits.push_back(ref->indices[0].as<IntLiteral>()->value);
        } else if (
            ref->target == conv.ir->platform->qubits &&
            ref->data_type == conv.ir->platform->default_bit_type
        ) {
            bregs.push_back(ref->indices[0].as<IntLiteral>()->value);
        } else if (
            ref->target == conv.breg_ob &&
            ref->data_type == conv.breg_ob->data_type
        ) {
            bregs.push_back(ref->indices[0].as<IntLiteral>()->value + conv.num_qubits);
        } else if (
            ref->target == conv.creg_ob &&
            ref->data_type == conv.creg_ob->data_type
        ) {
            cregs.push_back(ref->indices[0].as<IntLiteral>()->value);
        } else {
            QL_ICE(
                "encountered unknown object reference to "
                << ref->target->name
            );
        }
    } else if (expr->as_function_call()) {
        QL_ICE("encountered unsupported function call in gate operand list");
    } else {
        QL_ICE("cannot convert operand expression to old IR: " << describe(expr));
    }
}

/**
 * Converts the new IR to the old one. This requires that the platform was
 * constructed using convert_old_to_new(), and (obviously) that no features of
 * the new IR are used that are not supported by the old IR.
 */
compat::ProgramRef convert_new_to_old(const Ref &ir) {
    return NewToOldConverter::convert(ir);
}

} // namespace ir
} // namespace ql
