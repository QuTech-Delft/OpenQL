/** \file
 * handle operands within the new IR. Based on new_to_old.cc
 * FIXME: could be useful for other backends and should be moved if this becomes appropriate
 */

#include "operands.h"

#include "ql/ir/ops.h"
#include "ql/ir/describe.h"

namespace ql {
namespace arch {
namespace cc {
namespace pass {
namespace gen {
namespace vq1asm {
namespace detail {

// FIXME: cleanup, copied from NewToOldConverter
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
    // bits associated with qubits, so there are always num_qubits of these; only
    // beyond that is the b register used.
//    auto num_bregs = num_qubits;
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

    // FIXME: add q_ob, see 'Ref convert_old_to_new(const compat::PlatformRef &old)'
    q_ob  = find_physical_object(ir, "q");

    // Determine number of cregs.
//    utils::UInt num_cregs = 0;
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

Bool OperandContext::is_qubit_reference(const ir::Reference &ref) const {
    return
        ref.target.operator==(ir->platform->qubits)
        && ref.data_type == ir->platform->qubits->data_type
        && ref.indices.size() == 1
        && ref.indices[0]->as_int_literal();
}

Bool OperandContext::is_implicit_breg_reference(const ir::Reference &ref) const {
    return
        ref.target.operator==(ir->platform->qubits)
        && ref.data_type == ir->platform->default_bit_type
        && ref.indices.size() == 1
        && ref.indices[0]->as_int_literal();
}

Bool OperandContext::is_explicit_breg_reference(const ir::Reference &ref) const {
    return
        ref.target.operator==(breg_ob)   // NB: breg_ob is the object used by the new IR to refer to bregs from num_qubits onwards
        && ref.data_type == breg_ob->data_type
        && ref.indices.size() == 1
        && ref.indices[0]->as_int_literal();
}

Bool OperandContext::is_breg_reference(const ir::Reference &ref) const {
    return is_implicit_breg_reference(ref) || is_explicit_breg_reference(ref);
}

Bool OperandContext::is_creg_reference(const ir::Reference &ref) const {
    return
        ref.target.operator==(creg_ob)
        && ref.data_type == creg_ob->data_type
        && ref.indices.size() == 1
        && ref.indices[0]->as_int_literal();
}


Int OperandContext::convert_creg_reference(const ir::Reference &ref) const {
    CHECK_COMPAT(
        is_creg_reference(ref),
        "expected creg reference, but got something else: " << ir::describe(ref)
    );
    return ref.indices[0]->as_int_literal()->value;    // NB: range checking to be done by caller
}

// NB: converts both explicit and implicit bregs
UInt OperandContext::convert_breg_reference(const ir::Reference &ref) const {
    if(is_implicit_breg_reference(ref)) {
        return ref.indices[0]->as_int_literal()->value;    // NB: range checking to be done by caller
    } else if(is_explicit_breg_reference(ref)) {
        // NB: map explicit bregs (register 'b') after those implicit to qubits.
        return ref.indices[0]->as_int_literal()->value + num_qubits;    // NB: range checking to be done by caller
    } else {
        QL_ICE("expected bit (breg) reference, but got something else: " << ir::describe(ref));
    }
}

UInt OperandContext::convert_breg_reference(const ir::ExpressionRef &expr) const {
    auto ref = expr->as_reference();
    CHECK_COMPAT(
        ref,
        "expected reference, but got something else: " << ir::describe(expr)
    );
    return convert_breg_reference(*ref);
}

// see ql::ir::cqasm::convert_expression() for how expressions are built from cQASM, and ql::ir::cqasm::read for register definitions
// see ql::ir::convert_old_to_new(const compat::PlatformRef &old) on how cregs/bregs are created. This is also used by the NEW cQASM reader
// FIXME: maybe allow multiple real and int operands at some point
// FIXME: update messages to reflect that this function is now not only used for gate parameters, but also for function parameters
void Operands::append(const OperandContext &operandContext, const ir::ExpressionRef &expr) {
    Str operand_type = "?"; // default unless overwritten. Currently, only used for function parameters

    if (auto real_lit = expr->as_real_literal()) {
        CHECK_COMPAT(!has_angle, "encountered gate with multiple angle (real) operands");
        has_angle = true;
        angle = real_lit->value;
    } else if (auto int_lit = expr->as_int_literal()) {
        CHECK_COMPAT(!has_integer, "encountered gate with multiple integer operands");
        has_integer = true;
        integer = int_lit->value;
        operand_type = "i";
    } else if (expr->as_bit_literal()) {
        // FIXME: do something
        operand_type = "b";
    } else if (auto ref = expr->as_reference()) {

        if (ref->indices.size() != 1 || !ref->indices[0]->as_int_literal()) {
            QL_ICE(
                "encountered incompatible object reference to "
                << ref->target->name
                << " (size=" << ref->indices.size() << ")"
            );
        } else if (operandContext.is_qubit_reference(*ref)) {
            qubits.push_back(ref->indices[0]->as_int_literal()->value);
        } else if(operandContext.is_breg_reference(*ref)) {
            // NB: we use convert_breg_reference here because of the special semantics of implicit vs. explicit bregs
            bregs.push_back(operandContext.convert_breg_reference(*ref));
            operand_type = "B";
        } else if (operandContext.is_creg_reference(*ref)) {
            cregs.push_back(ref->indices[0]->as_int_literal()->value);
            operand_type = "C";
#if 0   // FIXME: handle variable reference here
        } else if {
            ref->data_type == operandContext.ir->platform->
        }
        ) {
#endif
        } else {
            QL_ICE(
                "encountered unknown object reference to "
                << ref->target->name
            );
        }
    } else if (expr->as_function_call()) {
        QL_ICE("encountered unsupported function call in operand list: " << describe(expr));
//        QL_INPUT_ERROR("cannot currently handle function call within function call '" << ir::describe(op) << "'");
    } else {
        QL_ICE("unsupported expression: " << describe(expr));
    }

    // update profile
    profile += operand_type;
}

} // namespace detail
} // namespace vq1asm
} // namespace gen
} // namespace pass
} // namespace cc
} // namespace arch
} // namespace ql
