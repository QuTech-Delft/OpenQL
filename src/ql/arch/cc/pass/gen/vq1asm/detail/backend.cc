/**
 * @file   arch/cc/pass/gen/vq1asm/detail/backend.cc
 * @date   201809xx
 * @author Wouter Vlothuizen (wouter.vlothuizen@tno.nl)
 * @brief  backend for the Central Controller
 */

/*
    Change log:
    20200116:
    - changed JSON field "signal_ref" to "ref_signal" to improve consistency
    - idem "ref_signals_type" to "signal_type"

    Todo:
    - finish support for kernel conditionality
    - port https://github.com/QE-Lab/OpenQL/pull/238 to CC
*/


#include "backend.h"

#include "ql/utils/str.h"
#include "ql/utils/filesystem.h"
#include "ql/ir/describe.h"
#include "ql/ir/ops.h"
#include "ql/com/options.h"

#include <regex>

/**
 * Helper macro for QL_ICE() that throws when the given condition is not
 * true. From new_to_old.cc
 */
#define CHECK_COMPAT(val, s) do { if (!(val)) QL_ICE(s); } while (false)

namespace ql {
namespace arch {
namespace cc {
namespace pass {
namespace gen {
namespace vq1asm {
namespace detail {

using namespace utils;

// FIXME: stuff below extracted from NewToOldConverter, required by class Operands
// FIXME: move to separate file, could be of use to other backends
class OperandContext {
public:
    OperandContext(const ir::Ref &ir);
    Int convert_creg_reference(const ir::ExpressionRef &ref) const;
    UInt convert_breg_reference(const ir::ExpressionRef &ref) const;    // FIXME:UInt vs Int

private:
    friend class Operands;

    /**
     * The root of the new IR structure that serves as our input.
     */
    ir::Ref ir;

    /**
     * The number of qubits.
     */
    utils::UInt num_qubits;

    /**
     * The object used by the new IR to refer to bregs from num_qubits onwards.
     */
    ir::ObjectLink breg_ob;

    /**
     * The object used by the new IR to refer to cregs.
     */
    ir::ObjectLink creg_ob;
};

OperandContext::OperandContext(const ir::Ref &ir) : ir(ir) {
    // Determine number of qubits.
    CHECK_COMPAT(
        ir->platform->qubits->shape.size() == 1,
        "main qubit register has wrong dimensionality"
    );
//    if (ir->program->has_annotation<ObjectUsage>()) {
//        num_qubits = ir->program->get_annotation<ObjectUsage>().num_qubits;
//    } else {
        num_qubits = ir->platform->qubits->shape[0];
//    }

    // Determine number of bregs. The first num_qubits bregs are the implicit
    // bits associated with qubits, so there are always num_qubits of these.
    auto num_bregs = num_qubits;
    breg_ob = find_physical_object(ir, "breg");
#if 0   // FIXME
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
#endif

    // Determine number of cregs.
    utils::UInt num_cregs = 0;
    creg_ob = find_physical_object(ir, "creg");
#if 0   // FIXME
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
#endif
}


/**
 * Converts a creg reference to a register index.
 */
Int OperandContext::convert_creg_reference(const ir::ExpressionRef &ref) const {
    auto lhs = ref->as_reference();
    CHECK_COMPAT(
        lhs &&
        lhs->target == creg_ob &&
        lhs->data_type == creg_ob->data_type &&
        lhs->indices.size() == 1 &&
        lhs->indices[0]->as_int_literal(),
        "expected creg reference, but got something else"
    );
    return lhs->indices[0]->as_int_literal()->value;
    // FIXME: check index against number of regs
}



// FIXME: shameless copy of new_to_old.cc::Operands, edited to suit

/**
 * Handles gathering the operands for a gate.
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
    void append(const OperandContext &operandContext, const ir::ExpressionRef &expr);

};


/**
 * Converts a bit reference to its breg index.
 */
// FIXME: change source code position, uses class Operands
UInt OperandContext::convert_breg_reference(const ir::ExpressionRef &ref) const {
    Operands ops;
    ops.append(*this, ref);
    CHECK_COMPAT(
        ops.bregs.size() == 1,
        "expected bit reference (breg), but got something else"
    );
    return ops.bregs[0];
}


/**
 * Appends an operand.
 */
// FIXME: see ql::ir::cqasm::convert_expression() for how expressions are built from cQASM, and ql::ir::cqasm::read for register definitions
// FIXME: see ql::ir::convert_old_to_new(const compat::PlatformRef &old) on how cregs/bregs are created. This is also used by the NEW cQASM reader
void Operands::append(const OperandContext &operandContext, const ir::ExpressionRef &expr) {
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
            ref->target == operandContext.ir->platform->qubits &&
            ref->data_type == operandContext.ir->platform->qubits->data_type
        ) {
            qubits.push_back(ref->indices[0].as<ir::IntLiteral>()->value);
        } else if (
            ref->target == operandContext.ir->platform->qubits &&
            ref->data_type == operandContext.ir->platform->default_bit_type
        ) {
            bregs.push_back(ref->indices[0].as<ir::IntLiteral>()->value);
        } else if (
            ref->target == operandContext.breg_ob &&
            ref->data_type == operandContext.breg_ob->data_type
        ) {
            bregs.push_back(ref->indices[0].as<ir::IntLiteral>()->value + operandContext.num_qubits);
        } else if (
            ref->target == operandContext.creg_ob &&
            ref->data_type == operandContext.creg_ob->data_type
        ) {
            cregs.push_back(ref->indices[0].as<ir::IntLiteral>()->value);
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




// compile for Central Controller
// NB: a new eqasm_backend_cc is instantiated per call to compile, so we don't need to cleanup
void Backend::compile(const ir::Ref &ir, const OptionsRef &options) {
    QL_DOUT("Compiling Central Controller program ... ");

    // init
    OperandContext oc(ir);
    codegen.init(ir->platform, options);
    bundleIdx = 0;

    // generate program header
    codegen.programStart(ir->program->unique_name);

#if 0   // FIXME: WIP
    // generate code for all kernels
    for (auto &kernel : program->kernels) {
        QL_IOUT("Compiling kernel: " << kernel->name);
        codegenKernelPrologue(kernel);

        if (!kernel->gates.empty()) {
            ir::compat::Bundles bundles = ir::compat::bundler(kernel);
            codegen.kernelStart();
            codegenBundles(bundles, program->platform);
            codegen.kernelFinish(kernel->name, bundles.back().start_cycle+bundles.back().duration_in_cycles);
        } else {
            QL_DOUT("Empty kernel: " << kernel->name);                      // NB: normal situation for kernels with classical control
        }

        codegenKernelEpilogue(kernel);
    }
#else   // new
    // FIXME: Nodes of interest:
    // ir->program->entry_point.links_to

    // NB: based on NewToOldConverter::NewToOldConverter
    for (const auto &block : ir->program->blocks) {
        try {
            codegen_block(oc, block, block->name);
        } catch (utils::Exception &e) {
            e.add_context("in block '" + block->name + "'");
            throw;
        }
    }
#endif

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
            QL_FATAL("internal inconsistency: requesting kernel label for kernel type " << (int)k->type);
    }

    std::regex re(expr, std::regex_constants::egrep);   // FIXME: we are reverse engineering the naming scheme of quantum_program::add_*
    const int numMatch = 1+1;		// NB: +1 because index 0 contains full input match
    std::smatch match;
    if(std::regex_search(k->name, match, re) && match.size() == numMatch) {
        label = match.str(1);
    } else {
        QL_FATAL("internal inconsistency: kernel name '" << k->name << "' does not contain loop suffix");
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
            QL_FATAL("inconsistency detected: unhandled kernel type");
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
            QL_FATAL("inconsistency detected: unhandled kernel type");
            break;
    }
}
#endif


// helpers
void handle_set_instruction(const OperandContext &operandContext, const ir::SetInstruction &set, const Str &descr= "")
{
    QL_IOUT(descr << ": '" << ir::describe(set) << "'");

    int lhs;
    try {
        lhs = operandContext.convert_creg_reference(set.lhs);
        // FIXME: also allow breg?
    } catch (utils::Exception &e) {
        e.add_context("unsupported LHS for set instruction encountered");
        throw;
    }

// FIXME: RHS is an expression
    try {
        if (auto ilit = set.rhs->as_int_literal()) {
            QL_IOUT(
                "set: "
                << "creg[" << lhs << "]"
                << " = "
                << "#" << ilit->value);
            // code: "move X,Rd"
        } else if (set.rhs->as_reference()) {
            auto creg = operandContext.convert_creg_reference(set.rhs);
            QL_IOUT(
                "set: "
                << "creg[" << lhs << "]"
                << " = "
                "creg[" << creg << "]");
            // code: "move Rs,Rd"
        } else if (auto fn = set.rhs->as_function_call()) {
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
                // code: "not Rs,Rd"

            // arithmetic, 2 operands
            } else if (fn->function_type->name == "operator+") {
                operation = "+";
                // code: "add Ra,Rb,Rd"
                // code: "add Ra,X,Rd"
            } else if (fn->function_type->name == "operator-") {
                operation = "-";
                // code: "sub Ra,Rb,Rd"
                // code: "sub Ra,X,Rd"  only Ra-X, not X-Ra
            } else if (fn->function_type->name == "operator&") {
                operation = "&";
                // code: "and Ra,Rb,Rd"
                // code: "and Ra,X,Rd"
            } else if (fn->function_type->name == "operator|") {
                operation = "|";
                // code: "or Ra,Rb,Rd"
                // code: "or Ra,X,Rd"
            } else if (fn->function_type->name == "operator^") {
                operation = "^";
                // code: "xor Ra,Rb,Rd"
                // code: "xor Ra,X,Rd"

            // relop
            // FIXME: should also support bregs
            } else if (fn->function_type->name == "operator==") {
                operation = "==";
                // code: "sub Ra,Rb,Rd" + not
                // code: "sub Ra,X,Rd" + not
            } else if (fn->function_type->name == "operator!=") {
                operation = "!=";
                // code: "sub Ra,Rb,Rd"
                // code: "sub Ra,X,Rd"
            } else if (fn->function_type->name == "operator>") {
                operation = ">";
            } else if (fn->function_type->name == "operator>=") {
                operation = ">=";
                // code "jge Rx,X,@label"
            } else if (fn->function_type->name == "operator<") {
                operation = "<";
                // code "jlt Rx,X,@label"
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
                auto creg = operandContext.convert_creg_reference(fn->operands[0]);
                QL_IOUT(
                    "set: "
                    << "creg[" << lhs << "]"
                    << " = "
                    << operation << " "
                    << "creg[" << creg << "]"
                );
                // FIXME: handle literal
//                kernel->classical(
//                    *lhs,
//                    compat::ClassicalOperation(
//                        operation,
//                        convert_creg_reference(fn->operands[0])
//                    )
//                );
            } else if (fn->operands.size() == 2) {
                auto &op0 = fn->operands[0];
                auto &op1 = fn->operands[1];

                if(op0->as_reference() && op1->as_reference()) {
                    auto creg0 = operandContext.convert_creg_reference(op0);
                    auto creg1 = operandContext.convert_creg_reference(op1);
                    QL_IOUT(
                        "set: "
                        << "creg[" << lhs << "]"
                        << " = "
                        << "creg[" << creg0 << "]"
                        << " " << operation << " "
                        << "creg[" << creg1 << "]"
                    );
                } else if(op0->as_reference() && op1->as_int_literal()) {
                    auto creg0 = operandContext.convert_creg_reference(op0);
                    QL_IOUT(
                        "set: "
                        << "creg[" << lhs << "]"
                        << " = "
                        << "creg[" << creg0 << "]"
                        << " " << operation << " "
                        << "#" << op1->as_int_literal()->value
                    );
                } // FIXME: etc, also handle "creg(0)=creg(0)+1+1"


//                kernel->classical(
//                    *lhs,
//                    compat::ClassicalOperation(
//                        convert_creg_reference(fn->operands[0]),
//                        operation,
//                        convert_creg_reference(fn->operands[1])
//                    )
//                );
            } else {
                QL_ASSERT(false);
            }
        }
    }
    catch (utils::Exception &e) {
        e.add_context("in gate condition", true);
        throw;
    }

}

void handle_expression(const ir::Expression &expression, const Str &descr= "")
{
    QL_IOUT(descr << ": '" << ir::describe(expression) << "'");
    // FIXME
}

typedef struct {
    utils::Vec<utils::UInt> cond_operands;
    ConditionType cond_type;
} tInstructionCondition;

/**
 * Decode the expression for a conditional instruction into the old format as used for the API. Eventually this will have
 * to be changed, but as long as the CC can handle expressions with 2 variables only this covers all we need.
 */
tInstructionCondition decodeCondition(const OperandContext &operandContext, const ir::ExpressionRef &condition) {
    utils::Vec<utils::UInt> cond_operands;
    ConditionType cond_type;
    try {
        if (auto blit = condition->as_bit_literal()) {
            if (blit->value) {
                cond_type = ConditionType::ALWAYS;
            } else {
                cond_type = ConditionType::NEVER;
            }
        } else if (condition->as_reference()) {
            cond_operands.push_back(operandContext.convert_breg_reference(condition));
            cond_type = ConditionType::UNARY;
        } else if (auto fn = condition->as_function_call()) {
            if (
                fn->function_type->name == "operator!" ||
                fn->function_type->name == "operator~"
            ) {
                CHECK_COMPAT(fn->operands.size() == 1, "unsupported condition function");
                if (fn->operands[0]->as_reference()) {
                    cond_operands.push_back(operandContext.convert_breg_reference(fn->operands[0]));
                    cond_type = ConditionType::NOT;
                } else if (auto fn2 = fn->operands[0]->as_function_call()) {
                    CHECK_COMPAT(fn2->operands.size() == 2, "unsupported condition function");
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
            } else {
                CHECK_COMPAT(fn->operands.size() == 2, "unsupported condition function");
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
        e.add_context("in gate condition", true);
        throw;
    }
    return {cond_operands, cond_type};
}

#if 0
// FIXME: names must be useable as q1asm labels
Str block_id(const Str &block_name, Int block_number) {
    return QL_SS2S(block_name << "_" << block_number);
}
Str block_child_name(const Str &block_name, const Str &child_name) {
    return block_name + "_" + child_name;
}
#endif

// Based on NewToOldConverter::convert_block
// FIXME: we need to collect 'Bundles' (i.e. statements starting in the same cycle)
// FIXME: convert block relative cycles to absolute cycles somewhere
// FIXME: runOnce automatically on cQASM input
// FIXME: provide (more) context in all QL_ICE and e.add_context
void Backend::codegen_block(const OperandContext &operandContext, const ir::BlockBaseRef &block, const Str &name)
{
    // helper lambdas
    // FIXME: names must be useable as q1asm labels
    auto block_child_name = [name](const Str &child_name) { return name + "_" + child_name; };
    auto label = [this, name]() { return QL_SS2S(name << "_" << block_number); };   // block_number is to uniquify anonymous blocks like for loops
    auto to_start = [](const Str &base) { return base + "_start"; };
    auto to_end = [](const Str &base) { return base + "_end"; };
    auto label_start = [label]() { return label() + "_start"; };
    auto label_end = [label]() { return label() + "_end"; };

    // Whether this is the first lazily-constructed kernel. Only if this is true
    // when flushing at the end are statistics annotations copied; otherwise
    // they would be invalid anyway.
//    utils::Bool first_kernel = true;

    // Cycle offset for converting from new-IR cycles to old-IR cycles. In
    // the new IR, cycles start at zero; in the old one they start at
    // compat::FIRST_CYCLE. This is set to utils::MAX as a marker after
    // structured control-flow; this implies that the next cycle number
    // encountered should map to compat::FIRST_CYCLE.
//    utils::Int cycle_offset = compat::FIRST_CYCLE;

    // Whether to set the cycles_valid flag on the old-style kernel. Cycles are
    // always valid in the new IR, but when the program was previously converted
    // from the old to the new IR, annotations can be used to clear the flag.
//    utils::Bool cycles_valid = true;
//    if (auto kcv = block->get_annotation_ptr<KernelCyclesValid>()) {
//        cycles_valid = kcv->valid;
//    }

    // FIXME: comments
    Int current_cycle = -1;
    Bool is_first_bundle = true;

    QL_IOUT("Compiling block '" + label() + "'");
    codegen.kernelStart(name);  // FIXME: difference between block and Bundles, adapt naming

    // Loop over the statements and handle them individually.
    for (const auto &stmt : block->statements) {
        if (auto insn = stmt->as_instruction()) {
            //****************************************************************
            // Statement: instruction
            //****************************************************************
            auto duration = ir::get_duration_of_statement(stmt);

            QL_IOUT(
                "instruction: " + ir::describe(stmt)
                + ", cycle=" + std::to_string(insn->cycle)
                + ", duration=" + std::to_string(duration)
            );

            // generate bundle trailer when necessary
            Bool is_new_bundle = insn->cycle != current_cycle;
            Bool is_last_statement = stmt == block->statements.back();
            if (is_new_bundle || is_last_statement) {
                if (!is_first_bundle) {
                    Int bundleDuration = insn->cycle-current_cycle+duration;
                    QL_DOUT(QL_SS2S("Finishing bundle " << bundleIdx << ": start_cycle=" << current_cycle << ", duration=" << bundleDuration));
                    // generate bundle trailer, and code for classical gates
                    Bool isLastBundle = is_last_statement;
                    codegen.bundleFinish(current_cycle, bundleDuration, isLastBundle);
                }
            }

            // generate bundle header when necessary
            if (is_new_bundle) {
                QL_DOUT(QL_SS2S("Bundle " << bundleIdx << ": start_cycle=" << insn->cycle));
                // FIXME: first instruction may be wait with zero duration, more generally: duration of first statement != bundle duration
                codegen.bundleStart(QL_SS2S(
                    "## Bundle " << bundleIdx++
                    << ": start_cycle=" << insn->cycle
//                    << ", duration_in_cycles=" << duration
                    << ":"
                ));

                is_first_bundle = false;
                current_cycle = insn->cycle;
            }

            // Ensure that we have a kernel to add the instruction to, and
            // that cycle_offset is valid.
//            if (kernel.empty()) {
//                kernel.emplace(
//                    make_kernel_name(block), old->platform,
//                    old->qubit_count, old->creg_count, old->breg_count
//                );
//            }
//            if (cycle_offset == utils::MAX) {
//                cycle_offset = compat::FIRST_CYCLE - insn->cycle;
//            }

            // The kernel.gate() calls can add more than one instruction due to
            // ad-hoc decompositions. Since we need to set the cycle numbers
            // after the fact, we need to track which gates already existed in
            // the kernel.
//            auto first_gate_index = kernel->gates.size();

            // Handle the instruction subtypes.
            if (auto cinsn = stmt->as_conditional_instruction()) {

                //****************************************************************
                // Instruction: conditional
                //****************************************************************

                // Handle the condition.
                tInstructionCondition instrCond = decodeCondition(operandContext, cinsn->condition);

                // Handle the conditional instruction subtypes.
                if (auto custom = cinsn->as_custom_instruction()) {

                    QL_IOUT("custom instruction: name=" + custom->instruction_type->name);

                    // Handle the normal operands for custom instructions.
                    Operands ops;
                    for (const auto &ob : custom->instruction_type->template_operands) {
                        QL_IOUT("template operand: " + ir::describe(ob));
                        try {
                            ops.append(operandContext, ob);
                        } catch (utils::Exception &e) {
                            e.add_context("name=" + custom->instruction_type->name + ", qubits=" + ops.qubits.to_string());
                            throw;
                        }
                    }

                    for (utils::UInt i = 0; i < custom->operands.size(); i++) {
                        try {
                            ops.append(operandContext, custom->operands[i]);
                        } catch (utils::Exception &e) {
                            e.add_context(
                                "name=" + custom->instruction_type->name
                                + ", qubits=" + ops.qubits.to_string()
                                + ", operand=" + std::to_string(i)
                                );
                            throw;
                        }
                    }
#if 0   // org
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
#else   // FIXME: cc
                    codegen.customGate(
                        custom->instruction_type->name,
                        ops.qubits,     // operands
                        ops.cregs,      // creg_operands
                        ops.bregs,      // breg_operands
                        instrCond.cond_type,      // condition
                        instrCond.cond_operands,  // cond_operands
                        ops.angle,      // angle
                        insn->cycle,    // startCycle
                        ir::get_duration_of_statement(stmt)    // durationInCycles
                    );
#endif

                } else if (auto set_instruction = cinsn->as_set_instruction()) {

                    //****************************************************************
                    // Instruction: set
                    //****************************************************************
                    handle_set_instruction(operandContext, *set_instruction, "conditional.set");
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

            // Copy gate annotations if adding the gate resulted in just one
            // gate.
//            if (kernel->gates.size() == first_gate_index + 1) {
//                kernel->gates[first_gate_index]->copy_annotations(*insn);
//            }

            // Assign the cycle numbers for the new gates.
//            for (auto i = first_gate_index; i < kernel->gates.size(); i++) {
//                kernel->gates[i]->cycle = (utils::UInt)((utils::Int)insn->cycle + cycle_offset);
//            }

        } else if (stmt->as_structured()) {

            //****************************************************************
            // Statement: structured
            //****************************************************************
            QL_IOUT("structured: " + ir::describe(stmt));
             // Flush any pending kernel not affected by control-flow.
//            if (!kernel.empty()) {
//                first_kernel = false;
//                kernel->cycles_valid = cycles_valid;
//                program->add(kernel);
//                kernel.reset();
//            }
//            cycle_offset = utils::MAX;

            // Handle the different types of structured statements.
            if (auto if_else = stmt->as_if_else()) {

                // Handle if-else or if statement.
#if 0   // FIXME: allow
                CHECK_COMPAT(
                    if_else->branches.size() == 1,
                    "encountered if-else chain with multiple conditions"
                );
#endif
//                compat::ProgramRef if_program;
//                if_program.emplace(
//                    make_kernel_name(block), old->platform,
//                    old->qubit_count, old->creg_count, old->breg_count
//                );
                try {
//                    convert_block(if_else->branches[0]->body, if_program);
                    codegen_block(operandContext, if_else->branches[0]->body, block_child_name("if"));
                } catch (utils::Exception &e) {
                    e.add_context("in 'if' block", true);
                    throw;
                }
                if (if_else->otherwise.empty()) {
                    try {
//                        program->add_if(
//                            if_program,
//                            convert_classical_condition(
//                                if_else->branches[0]->condition,
//                                false
//                            )
//                        );
                    } catch (utils::Exception &e) {
                        e.add_context("in 'if' condition", true);
                        throw;
                    }
                } else {
//                    compat::ProgramRef else_program;
//                    else_program.emplace(
//                        make_kernel_name(block), old->platform,
//                        old->qubit_count, old->creg_count, old->breg_count
//                    );
                    try {
//                        convert_block(if_else->otherwise, else_program);
                    } catch (utils::Exception &e) {
                        e.add_context("in 'else' block", true);
                        throw;
                    }
                    try {
//                        program->add_if_else(
//                            if_program,
//                            else_program,
//                            convert_classical_condition(
//                                if_else->branches[0]->condition,
//                                false
//                            )
//                        );
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
//                CHECK_COMPAT(
//                    static_loop->lhs->target != creg_ob,
//                    "static loop variable cannot be a mapped creg"
//                );
//                compat::ProgramRef body;
//                body.emplace(
//                    make_kernel_name(block), old->platform,
//                    old->qubit_count, old->creg_count, old->breg_count
//                );
                try {
//                    convert_block(static_loop->body, body);
                } catch (utils::Exception &e) {
                    e.add_context("in static loop body", true);
                    throw;
                }
//                program->add_for(
//                    body,
//                    utils::abs<utils::Int>(
//                        static_loop->to->value - static_loop->frm->value
//                    ) + 1
//                );

            } else if (auto repeat_until_loop = stmt->as_repeat_until_loop()) {

                // Handle repeat-until/do-while loops.
//                compat::ProgramRef body;
//                body.emplace(
//                    make_kernel_name(block), old->platform,
//                    old->qubit_count, old->creg_count, old->breg_count
//                );
                try {
//                    convert_block(repeat_until_loop->body, body);
                } catch (utils::Exception &e) {
                    e.add_context("in repeat-until/do-while loop body", true);
                    throw;
                }
                try {
//                    program->add_do_while(
//                        body,
//                        convert_classical_condition(
//                            repeat_until_loop->condition,
//                            true
//                        )
//                    );
                } catch (utils::Exception &e) {
                    e.add_context("in repeat-until/do-while condition", true);
                    throw;
                }

            } else if (auto for_loop = stmt->as_for_loop()) {
                // body prelude: initialize
                if (!for_loop->initialize.empty()) {
                    handle_set_instruction(operandContext, *for_loop->initialize, "for.initialize");
                }

                // body prelude: condition
                // FIXME: emit loopLabelStart. Also see codeGen.forStart
                QL_IOUT("label=" << label_start());
                handle_expression(*for_loop->condition, "for.condition");

                // handle body
                loop_label.push_back(label());          // remind label for break/continue
                Str end_label = label_end();            // save label before recursing
                try {
                    codegen_block(operandContext, for_loop->body, block_child_name("for"));
                } catch (utils::Exception &e) {
                    e.add_context("in for loop body", true);
                    throw;
                }
                loop_label.pop_back();

                // handle looping
                if (!for_loop->update.empty()) {
                    handle_set_instruction(operandContext, *for_loop->update, "for.update");
                }
                // FIXME: jmp loopLabelStart
                // FIXME: emit loopLabelEnd for break. Also see codeGen.forEnd
                QL_IOUT("label=" << end_label);

            } else if (auto break_statement = stmt->as_break_statement()) {
                QL_IOUT("break to " << to_end(loop_label.back()));
                // FIXME: jmp loopLabelEnd

            } else if (auto continue_statement = stmt->as_continue_statement()) {
                QL_IOUT("continue to " << to_start(loop_label.back()));
                // FIXME: jmp loopLabelStart

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

    // Flush any pending kernel.
//    if (!kernel.empty()) {
//
//        // If this block produced only one kernel, copy kernel-wide annotations.
//        if (first_kernel) {
//            kernel->copy_annotations(*block);
//        }
//
//        kernel->cycles_valid = cycles_valid;
//        program->add(kernel);
//        kernel.reset();
//    }

    codegen.kernelFinish(name, ir::get_duration_of_block(block));
    QL_IOUT("Finished compiling block '" + label() + "'");
    block_number++;

}


#if 0
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
                        QL_FATAL("Inconsistency detected in bundle contents: classical gate found after first section (which itself was non-classical)");
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
                        QL_FATAL("Gate type __display__ not supported");           // QX specific, according to openql.pdf
                        break;

                    case ir::compat::GateType::MEASURE:
                        QL_FATAL("Gate type __measure_gate__ not supported");      // no use, because there is no way to define CC-specifics
                        break;

                    default:
                        QL_FATAL(
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
