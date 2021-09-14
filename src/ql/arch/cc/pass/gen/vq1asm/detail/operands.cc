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

// FXIME
Bool OperandContext::is_creg_reference(const ir::ExpressionRef &ref) const {
    auto lhs = ref->as_reference();
    return lhs && lhs->target == creg_ob;
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


/**
 * Converts a bit reference to its breg index.
 */
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

} // namespace detail
} // namespace vq1asm
} // namespace gen
} // namespace pass
} // namespace cc
} // namespace arch
} // namespace ql
