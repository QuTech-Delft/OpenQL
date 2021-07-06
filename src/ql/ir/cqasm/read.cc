/** \file
 * cQASM 1.2 reader logic as human-readable complement of the IR.
 */

#include "ql/ir/cqasm/read.h"

#include "ql/utils/filesystem.h"
#include "ql/ir/compat/program.h"
#include "ql/ir/ops.h"
#include "ql/ir/consistency.h"
#include "ql/com/ddg/build.h"
#include "cqasm.hpp"

namespace ql {
namespace ir {
namespace cqasm {

namespace cq = ::cqasm::v1;
namespace cqv = ::cqasm::v1::values;
namespace cqty = ::cqasm::v1::types;
namespace cqs = ::cqasm::semantic;
namespace cqt = ::cqasm::tree;

/**
 * Marker used on cQASM nodes when they have been successfully used by
 * something that should be used exactly once. Used to throw an exception if any
 * ql.* annotations end up not being used in the end.
 */
struct Used{};

/**
 * Converts a type from the IR to a cQASM type. The assignable flag sets whether
 * libqasm should allow values of this type to be assigned. For qubits this is
 * always true, for other types it defaults to false.
 */
static cqty::Type make_cq_type(
    const DataTypeLink &ql_type,
    utils::Bool assignable = false
) {
    cqty::Type cq_type;
    if (ql_type->as_qubit_type()) {
        cq_type.emplace<cqty::Qubit>();
        assignable = true;
    } else if (ql_type->as_bit_type()) {
        cq_type.emplace<cqty::Bool>();
    } else if (ql_type->as_int_type()) {
        cq_type.emplace<cqty::Int>();
    } else if (ql_type->as_real_type()) {
        cq_type.emplace<cqty::Real>();
    } else if (ql_type->as_complex_type()) {
        cq_type.emplace<cqty::Complex>();
    } else if (auto rmat = ql_type->as_real_matrix_type()) {
        cq_type.emplace<cqty::RealMatrix>(rmat->num_rows, rmat->num_cols);
    } else if (auto cmat = ql_type->as_complex_matrix_type()) {
        cq_type.emplace<cqty::ComplexMatrix>(cmat->num_rows, cmat->num_cols);
    } else if (ql_type->as_string_type()) {
        cq_type.emplace<cqty::String>();
    } else if (ql_type->as_json_type()) {
        cq_type.emplace<cqty::Json>();
    } else {
        QL_ASSERT(false);
    }
    cq_type->set_annotation(ql_type);
    cq_type->assignable = assignable;
    return cq_type;
}

/**
 * Converts an operand type from the IR to a cQASM type.
 */
static cqty::Type make_cq_op_type(const utils::One<OperandType> &ql_op_type) {
    switch (ql_op_type->mode) {
        case prim::OperandMode::READ:
        case prim::OperandMode::LITERAL:
            return make_cq_type(ql_op_type->data_type, false);
        default:
            return make_cq_type(ql_op_type->data_type, true);
    }
}

/**
 * Makes a reference to a register, modelled as a builtin function call with the
 * indices as its operands.
 */
static cqv::Value make_cq_register_ref(
    const ObjectLink &ql_obj,
    const cqv::Values &cq_indices
) {
    cqv::Value cq_val = cqt::make<cqv::Function>(
        ql_obj->name,
        cq_indices,
        make_cq_type(ql_obj->data_type, true)
    );
    cq_val->set_annotation(ql_obj);
    cq_val->set_annotation<DataTypeLink>(ql_obj->data_type);
    return cq_val;
}

/**
 * Looks for a pragma instruction with an annotation with interface ql and the
 * specified operation. Returns an empty node if none was found.
 */
static cqt::Maybe<cqs::AnnotationData> find_pragma(
    const cqt::One<cqs::Node> &node,
    const utils::Str &operation
) {
    class FindPragma : public cqs::RecursiveVisitor {
    public:
        utils::Str operation;
        cqt::Maybe<cqs::AnnotationData> data;
        void visit_node(cqs::Node &node) override {
        }
        void visit_instruction(cqs::Instruction &node) override {
            if (!data.empty()) return;
            if (node.name != "pragma") return;
            for (const auto &annot : node.annotations) {
                if (annot->interface != "ql") continue;
                if (annot->operation != operation) continue;
                data = annot;
                return;
            }
        }
    };

    FindPragma fp;
    fp.operation = operation;
    node->visit(fp);
    return fp.data;
}

/**
 * Looks for an annotation with interface ql and the specified operation.
 */
static utils::RawPtr<cqs::AnnotationData> find_annotation(
    const cqt::One<cqs::Node> &node,
    const utils::Str &operation
) {
    class FindAnnotation : public cqs::RecursiveVisitor {
    public:
        utils::Str operation;
        utils::RawPtr<cqs::AnnotationData> data;
        void visit_node(cqs::Node &node) override {
        }
        void visit_annotation_data(cqs::AnnotationData &node) override {
            if (data) return;
            if (node.interface != "ql") return;
            if (node.operation != operation) return;
            data = &node;
        }
    };

    FindAnnotation fa;
    node->visit(fa);
    return fa.data;
}

/**
 * Looks for an annotation with interface ql and the specified operation.
 */
template <class T>
static utils::RawPtr<cqs::AnnotationData> find_annotation(
    const cqt::Any<T> &nodes,
    const utils::Str &operation
) {
    utils::RawPtr<cqs::AnnotationData> a;
    for (const auto &node : nodes) {
        a = find_annotation(node, operation);
        if (a) break;
    }
    return a;
}

/**
 * Ensures that all @ql.* annotations in the given node have been used.
 */
static void check_all_annotations_used(const cqt::One<cqs::Node> &node) {
    class FindAnnotation : public cqs::RecursiveVisitor {
    public:
        void visit_node(cqs::Node &node) override {
        }
        void visit_annotation_data(cqs::AnnotationData &node) override {
            if (node.interface == "ql") {
                if (!node.has_annotation<Used>()) {
                    QL_USER_ERROR(
                        "annotation @ql." + node.operation +
                        " is not supported or was unused"
                    );
                }
            }
        }
    };

    FindAnnotation fa;
    node->visit(fa);
}

/**
 * Parses a ql.name annotation.
 */
utils::Str parse_name_annotation(
    const cqt::One<cqs::AnnotationData> &annot,
    utils::Bool identifier = true
) {
    if (annot->operands.size() != 1) {
        QL_USER_ERROR(
            "@ql.name must have a single string argument"
        );
    }
    if (auto str = annot->operands[0]->as_const_string()) {
        if (identifier && !std::regex_match(str->value, IDENTIFIER_RE)) {
            QL_USER_ERROR(
                "name specified via @ql.name must be an identifier"
            );
        }
        annot->set_annotation<Used>({});
        return str->value;
    } else {
        QL_USER_ERROR(
            "@ql.name must have a single string argument"
        );
    }
}

/**
 * Parses a ql.type annotation.
 */
DataTypeLink parse_type_annotation(
    const Ref &ir,
    const cqt::One<cqs::AnnotationData> &annot
) {
    if (annot->operands.size() != 1) {
        QL_USER_ERROR(
            "@ql.type must have a single string argument"
        );
    }
    if (auto str = annot->operands[0]->as_const_string()) {
        auto typ = find_type(ir, str->value);
        if (typ.empty()) {
            QL_USER_ERROR(
                "type specified via @ql.type does not exist in platform"
            );
        }
        annot->set_annotation<Used>({});
        return typ;
    } else {
        QL_USER_ERROR(
            "@ql.type must have a single string argument"
        );
    }
}

/**
 * Infers a matching OpenQL type for the given cQASM type.
 */
DataTypeLink infer_ql_type(const Ref &ir, const cqty::Type &cq_type) {
    if (cq_type->as_qubit()) {
        return ir->platform->qubits->data_type;
    } else if (cq_type->as_bool()) {
        return ir->platform->default_bit_type;
    } else if (cq_type->as_int()) {
        return ir->platform->default_int_type;
    } else if (cq_type->as_real()) {
        for (const auto &ql_type : ir->platform->data_types) {
            if (ql_type->as_real_type()) {
                return ql_type;
            }
        }
    } else if (cq_type->as_complex()) {
        for (const auto &ql_type : ir->platform->data_types) {
            if (ql_type->as_complex_type()) {
                return ql_type;
            }
        }
    } else if (auto cq_rmat = cq_type->as_real_matrix()) {
        for (const auto &ql_type : ir->platform->data_types) {
            if (auto ql_rmat = ql_type->as_real_matrix_type()) {
                if (cq_rmat->num_rows != (utils::Int)ql_rmat->num_rows) continue;
                if (cq_rmat->num_cols != (utils::Int)ql_rmat->num_cols) continue;
                return ql_type;
            }
        }
    } else if (auto cq_cmat = cq_type->as_complex_matrix()) {
        for (const auto &ql_type : ir->platform->data_types) {
            if (auto ql_cmat = ql_type->as_complex_matrix_type()) {
                if (cq_cmat->num_rows != (utils::Int)ql_cmat->num_rows) continue;
                if (cq_cmat->num_cols != (utils::Int)ql_cmat->num_cols) continue;
                return ql_type;
            }
        }
    } else if (cq_type->as_string()) {
        for (const auto &ql_type : ir->platform->data_types) {
            if (ql_type->as_string_type()) {
                return ql_type;
            }
        }
    } else if (cq_type->as_json()) {
        for (const auto &ql_type : ir->platform->data_types) {
            if (ql_type->as_json_type()) {
                return ql_type;
            }
        }
    }
    QL_USER_ERROR(
        "failed to infer OpenQL type for " << cq_type << "; "
        "please use @ql.type(name) annotation and/or ensure that an applicable "
        "type exists in the platform"
    );
}

/**
 * Returns the last instruction in the given subcircuit, if any, if it is an
 * unconditional goto instruction. Also returns whether it is the only
 * instruction in the subcircuit or not.
 */
static utils::Pair<utils::RawPtr<cqs::GotoInstruction>, utils::Bool>
find_last_goto_instruction(const cqt::One<cqs::Subcircuit> &subcircuit) {

    class FindGoto : public cqs::RecursiveVisitor {
    public:
        utils::RawPtr<cqs::GotoInstruction> goto_insn;
        utils::Bool only_insn = true;
        void visit_node(cqs::Node &node) override {
        }
        void visit_instruction_base(cqs::InstructionBase &node) override {
            if (auto insn = node.as_instruction()) {
                if (insn->name == "pragma") {
                    return;
                }
            } else if (auto gi = node.as_goto_instruction()) {
                if (auto cb = gi->condition->as_const_bool()) {
                    if (cb->value) {
                        goto_insn = gi;
                        return;
                    }
                }
            }
            only_insn = false;
            goto_insn.reset();
        }
        void visit_structured(cqs::Structured &node) override {
            only_insn = false;
            goto_insn.reset();
        }
    };

    FindGoto fg;
    subcircuit->visit(fg);

    // If we got a goto instruction, ensure that it's actually the last one;
    // the visitor above would also return the last goto instruction in a
    // subblock, if no instruction follows it.
    if (fg.goto_insn) {
        if (&*subcircuit->body->statements.back().as<cqs::BundleExt>()->items.back() != fg.goto_insn.unwrap()) {
            fg.goto_insn.reset();
        }
    }

    return {fg.goto_insn, fg.only_insn};
}

/**
 * Converts a qubit/bit index to a static unsigned integer.
 */
static utils::UInt convert_index(
    const cqv::Value &cq_expr
) {
    if (auto ci = cq_expr->as_const_int()) {
        if (ci->value < 0) {
            QL_USER_ERROR("indices must be non-negative");
        }
        return (utils::UInt)ci->value;
    } else {
        QL_USER_ERROR("dynamic indices are not supported");
    }
}

/**
 * Converts a cQASM expression node to an OpenQL expression node.
 *
 * If sgmq_size is set to 0, BitRefs with more than one index are reduced to
 * a single expression using operator&&, and qubit reference indices must be
 * singular. If it is nonzero, qubit- and bit references must have exactly the
 * specified number of indices, and the sgmq_index'd index is used.
 */
static ExpressionRef convert_expression(
    const Ref &ir,
    const cqv::Value &cq_expr,
    utils::UInt sgmq_size = 1,
    utils::UInt sgmq_index = 0
) {

    // The typecast functions attach a DataTypeLink annotation to the values.
    // Look for that to determine which type to use. When as_type is empty, a
    // suitable type is inferred.
    DataTypeLink as_type;
    if (cq_expr->has_annotation<DataTypeLink>()) {
        as_type = cq_expr->get_annotation<DataTypeLink>();
    }

    if (auto cb = cq_expr->as_const_bool()) {
        return make_bit_lit(ir, cb->value, as_type);
    } else if (cq_expr->as_const_axis()) {
        QL_USER_ERROR("OpenQL does not support cQASM's axis data type");
    } else if (auto ci = cq_expr->as_const_int()) {
        return make_int_lit(ir, ci->value, as_type);
    } else if (auto cr = cq_expr->as_const_real()) {
        if (as_type.empty()) {
            as_type = infer_ql_type(ir, cqv::type_of(cq_expr));
        }
        if (!as_type->as_real_type()) {
            QL_USER_ERROR("cannot cast real number to type " << as_type->name);
        }
        return utils::make<RealLiteral>(cr->value, as_type);
    } else if (auto cc = cq_expr->as_const_complex()) {
        if (as_type.empty()) {
            as_type = infer_ql_type(ir, cqv::type_of(cq_expr));
        }
        if (!as_type->as_complex_type()) {
            QL_USER_ERROR("cannot cast complex number to type " << as_type->name);
        }
        return utils::make<ComplexLiteral>(cc->value, as_type);
    } else if (auto crm = cq_expr->as_const_real_matrix()) {
        if (as_type.empty()) {
            as_type = infer_ql_type(ir, cqv::type_of(cq_expr));
        }
        if (auto rmt = as_type->as_real_matrix_type()) {
            if (rmt->num_rows != crm->value.size_rows() || rmt->num_cols != crm->value.size_cols()) {
                QL_USER_ERROR("real matrix has incorrect size for type " << as_type->name);
            }
        } else {
            QL_USER_ERROR("cannot cast real matrix to type " << as_type->name);
        }
        return utils::make<RealMatrixLiteral>(
            prim::RMatrix(crm->value.get_data(), crm->value.size_cols()),
            as_type
        );
    } else if (auto ccm = cq_expr->as_const_complex_matrix()) {
        if (as_type.empty()) {
            as_type = infer_ql_type(ir, cqv::type_of(cq_expr));
        }
        if (auto cmt = as_type->as_complex_matrix_type()) {
            if (cmt->num_rows != ccm->value.size_rows() || cmt->num_cols != ccm->value.size_cols()) {
                QL_USER_ERROR("complex matrix has incorrect size for type " << as_type->name);
            }
        } else {
            QL_USER_ERROR("cannot cast complex matrix to type " << as_type->name);
        }
        return utils::make<ComplexMatrixLiteral>(
            prim::CMatrix(ccm->value.get_data(), ccm->value.size_cols()),
            as_type
        );
    } else if (auto cs = cq_expr->as_const_string()) {
        if (as_type.empty()) {
            as_type = infer_ql_type(ir, cqv::type_of(cq_expr));
        }
        if (!as_type->as_string_type()) {
            QL_USER_ERROR("cannot cast string to type " << as_type->name);
        }
        return utils::make<StringLiteral>(cs->value, as_type);
    } else if (auto cj = cq_expr->as_const_json()) {
        if (as_type.empty()) {
            as_type = infer_ql_type(ir, cqv::type_of(cq_expr));
        }
        if (!as_type->as_string_type()) {
            QL_USER_ERROR("cannot cast JSON to type " << as_type->name);
        }
        return utils::make<JsonLiteral>(utils::parse_json(cj->value), as_type);
    } else if (auto qr = cq_expr->as_qubit_refs()) {
        if (qr->index.size() != utils::max<utils::UInt>(1, sgmq_size)) {
            QL_USER_ERROR(
                "unexpected number of single-gate-multiple-qubit "
                "qubit indices specified; found " << qr->index.size() <<
                ", expected " << utils::max<utils::UInt>(1, sgmq_size)
            );
        }
        if (as_type.empty() || as_type == ir->platform->qubits->data_type) {
            return make_qubit_ref(ir, convert_index(qr->index[sgmq_index]));
        } else if (as_type == ir->platform->default_bit_type) {
            return make_bit_ref(ir, convert_index(qr->index[sgmq_index]));
        } else {
            QL_USER_ERROR("cannot cast qubit reference to type " << as_type->name);
        }
    } else if (auto br = cq_expr->as_bit_refs()) {
        if (!as_type.empty() && as_type != ir->platform->default_bit_type) {
            QL_USER_ERROR("cannot cast bit reference to type " << as_type->name);
        }
        if (sgmq_size) {
            if (br->index.size() != sgmq_size) {
                QL_USER_ERROR(
                    "unexpected number of single-gate-multiple-qubit "
                    "bit indices specified; found " << br->index.size() <<
                    ", expected " << utils::max<utils::UInt>(1, sgmq_size)
                );
            }
            return make_bit_ref(ir, convert_index(br->index[sgmq_index]));
        } else {
            ExpressionRef expr = make_bit_ref(ir, convert_index(br->index[0]));
            for (utils::UInt idx = 1; idx < br->index.size(); idx++) {
                expr = make_function_call(ir, "operator&&", utils::Any<Expression>({
                   expr, make_bit_ref(ir, convert_index(br->index[idx]))
                }));
            }
            return expr;
        }
    } else if (auto vr = cq_expr->as_variable_ref()) {
        auto ql_object = vr->variable->get_annotation<ObjectLink>();
        if (!as_type.empty() && as_type != ql_object->data_type) {
            QL_USER_ERROR(
                "cannot cast variable '" << ql_object->name <<
                "' to type " << as_type->name
            );
        }
        return make_reference(ir, ql_object, {});
    } else if (auto fn = cq_expr->as_function()) {
        if (fn->has_annotation<ObjectLink>()) {

            // Handle index functions for non-scalar register references.
            auto ql_object = fn->get_annotation<ObjectLink>();
            prim::UIntVec ql_indices;
            for (const auto &cq_operand : fn->operands) {
                ql_indices.push_back(convert_index(cq_operand));
            }
            return make_reference(ir, ql_object, ql_indices);

        } else {

            // Handle normal functions.
            utils::Any<Expression> ql_operands;
            for (const auto &cq_operand : fn->operands) {
                ql_operands.add(convert_expression(ir, cq_operand, sgmq_size, sgmq_index));
            }
            return make_function_call(ir, fn->name, ql_operands);

        }
    } else {
        QL_ICE("received unknown value node type from libqasm");
    }
}

/**
 * Converts a cQASM set instruction node to an OpenQL set instruction node.
 */
static utils::One<SetInstruction> convert_set_instruction(
    const Ref &ir,
    const cqs::SetInstruction &cq_set_insn
) {
    auto ql_lhs = convert_expression(ir, cq_set_insn.lhs).as<Reference>();
    if (ql_lhs.empty()) {
        QL_USER_ERROR(
            "left-hand side of assignment is not assignable"
        );
    }
    auto ql_lhs_type = get_type_of(ql_lhs);
    if (ql_lhs_type->as_qubit_type()) {
        QL_USER_ERROR(
            "qubits cannot be assigned"
        );
    }
    auto ql_rhs = convert_expression(ir, cq_set_insn.rhs);
    auto ql_rhs_type = get_type_of(ql_rhs);
    if (ql_lhs_type != ql_rhs_type) {
        QL_USER_ERROR(
            "type of left-hand side of assignment (" << ql_lhs_type->name << ") "
            "does not match type of right-hand side (" << ql_rhs_type->name << ")"
        );
    }
    return utils::make<SetInstruction>(ql_lhs, ql_rhs);
}

/**
 * Converts the contents of a cQASM block to an OpenQL block.
 */
static void convert_block(
    const Ref &ir,
    const cqt::One<cqs::Block> &cq_block,
    const utils::One<BlockBase> &ql_block,
    const ReadOptions &options
) {

    // We need to convert bundle + skip instruction representation of the
    // schedule to cycle numbers for schedulable instructions. So track the
    // cycle number, incrementing it on skip and the end of a bundle.
    utils::UInt cycle = 0;

    for (const auto &cq_stmt : cq_block->statements) {
        if (auto cq_bun = cq_stmt->as_bundle_ext()) {

            // Build a list of all the instructions in this bundle.
            utils::List<InstructionRef> ql_bundle;

            for (const auto &cq_insn_base : cq_bun->items) {

                // Parse the condition.
                auto conditional = true;
                if (auto cb = cq_insn_base->condition->as_const_bool()) {
                    if (cb->value) {
                        conditional = false;
                    }
                }
                ExpressionRef ql_condition;
                if (conditional) {
                    ql_condition = convert_expression(ir, cq_insn_base->condition, 0);
                    auto ql_type = get_type_of(ql_condition);
                    if (!ql_type->as_bit_type()) {
                        QL_USER_ERROR(
                            "type of condition (" << ql_type->name << ") is not bit-like"
                        );
                    }
                }

                // Build an instruction out of it, based on the type.
                utils::List<InstructionRef> ql_insns;
                if (auto cq_insn = cq_insn_base->as_instruction()) {
                    if (cq_insn->name == "skip") {

                        // Special skip instruction to encode advancing the
                        // cycle counter. Must have a single static non-negative
                        // integer operand.
                        if (conditional) {
                            QL_USER_ERROR(
                                "condition not supported for this instruction"
                            );
                        }
                        if (cq_insn->operands.size() != 1) {
                            QL_USER_ERROR(
                                "skip instructions must have a single "
                                "constant integer operand"
                            );
                        }
                        if (auto ci = cq_insn->operands[0]->as_const_int()) {
                            if (ci->value < 0) {
                                QL_USER_ERROR(
                                    "skip instructions cannot have a negative "
                                    "skip count"
                                );
                            }

                            // Only actually listen to the skip instruction if
                            // the schedule is to be retained.
                            if (options.schedule_mode == ScheduleMode::KEEP) {
                                cycle += (utils::UInt)ci->value;
                            }

                        } else {
                            QL_USER_ERROR(
                                "skip instructions must have a single "
                                "constant integer operand"
                            );
                        }

                    } else if (cq_insn->name == "pragma") {

                        // Special pragma instruction to attach annotations to.
                        // Currently entirely ignored by OpenQL outside of the
                        // header (i.e. the default subcircuit, which is parsed
                        // separately) So this is no-op.

                    } else if (
                        (
                            cq_insn->name == "wait" &&
                            !cq_insn->operands.empty() &&
                            cq_insn->operands[0]->as_const_int()
                        ) ||
                        cq_insn->name == "barrier"
                    ) {

                        // Handle wait and barrier instructions. These differ
                        // from normal instructions in that single-gate-
                        // multiple-qubit notation does not result in multiple
                        // parallel instructions, but rather just adds all
                        // referred qubits/bits to the object "sensitivity
                        // list". This is hacky, but was the easiest way to
                        // backport the barrier instruction to older software,
                        // since varargs are not currently supported by libqasm.
                        utils::Any<Expression> ql_operands;
                        for (const auto &cq_operand : cq_insn->operands) {
                            utils::UInt sgmq_size = 1;
                            if (auto qr = cq_operand->as_qubit_refs()) {
                                sgmq_size = qr->index.size();
                                break;
                            } else if (auto br = cq_operand->as_bit_refs()) {
                                sgmq_size = br->index.size();
                                break;
                            }
                            for (utils::UInt sgmq_index = 0; sgmq_index < sgmq_size; sgmq_index++) {
                                ql_operands.add(convert_expression(ir, cq_operand, sgmq_size, sgmq_index));
                            }
                        }
                        ql_insns.push_back(make_instruction(ir, cq_insn->name, ql_operands, ql_condition));

                    } else {

                        // Handle instructions with normal single-gate-multiple-
                        // qubit semantics.
                        utils::UInt sgmq_size = 1;
                        for (const auto &cq_operand : cq_insn->operands) {
                            if (auto qr = cq_operand->as_qubit_refs()) {
                                sgmq_size = qr->index.size();
                                break;
                            } else if (auto br = cq_operand->as_bit_refs()) {
                                sgmq_size = br->index.size();
                                break;
                            }
                        }
                        for (utils::UInt sgmq_index = 0; sgmq_index < sgmq_size; sgmq_index++) {
                            utils::Any<Expression> ql_operands;
                            for (const auto &cq_operand : cq_insn->operands) {
                                ql_operands.add(convert_expression(ir, cq_operand, sgmq_size, sgmq_index));
                            }

                            // `wait q[0], int` is unfortunately a special case,
                            // because of the agreements made for
                            // multiple-measurement support in Starmon-5; the
                            // operands are swapped in OpenQL.
                            if (
                                cq_insn->name == "wait" &&
                                ql_operands.size() == 2 &&
                                ql_operands[0]->as_reference() &&
                                ql_operands[0]->as_reference()->data_type == ir->platform->qubits->data_type &&
                                ql_operands[1]->as_int_literal()
                            ) {
                                auto x = ql_operands[0];
                                ql_operands[0] = ql_operands[1];
                                ql_operands[1] = x;
                            }

                            ql_insns.push_back(make_instruction(ir, cq_insn->name, ql_operands, ql_condition));
                        }

                    }
                } else if (auto cq_set_insn = cq_insn_base->as_set_instruction()) {

                    // Handle set instructions.
                    ql_insns.push_back(convert_set_instruction(ir, *cq_set_insn));

                } else if (auto cq_goto_insn = cq_insn_base->as_goto_instruction()) {

                    // Handle goto instructions.
                    ql_insns.push_back(utils::make<GotoInstruction>(
                        cq_goto_insn->target->get_annotation<utils::One<Block>>()
                    ));

                } else {
                    QL_ICE("received unknown instruction node type from libqasm");
                }

                // If this cQASM instruction produced an OpenQL instruction,
                // complete it, and then add it to the end of the current
                // bundle.
                for (const auto &ql_insn : ql_insns) {
                    ql_insn->cycle = cycle;
                    if (auto ql_cond_insn = ql_insn->as_conditional_instruction()) {
                        if (ql_cond_insn->condition.empty()) {
                            if (!ql_condition.empty()) {
                                ql_cond_insn->condition = ql_condition.clone();
                            } else {
                                ql_cond_insn->condition = make_bit_lit(ir, true);
                            }
                        }
                    } else if (!ql_condition.empty()) {
                        QL_USER_ERROR(
                            "condition not supported for this instruction"
                        );
                    }
                    ql_bundle.push_back(ql_insn);

                    // If scheduling information is discarded, increment the
                    // cycle number at the end of each instruction.
                    if (options.schedule_mode != ScheduleMode::KEEP) {
                        cycle++;
                    }

                }

            }

            // Add implicit barriers before and after bundles if bundles are
            // used as a shorthand notation for this rather than for scheduling
            // information.
            if (
                !ql_bundle.empty() &&
                options.schedule_mode == ScheduleMode::BUNDLES_AS_BARRIERS
            ) {

                // Figure out which objects are being used by this bundle.
                com::ddg::EventGatherer eg{ir};
                for (const auto &ql_insn : ql_bundle) {
                    eg.add_statement(ql_insn);
                }
                utils::Any<Expression> ql_operands;
                for (const auto &event : eg.get()) {
                    if (!event.first.target.empty()) {

                        // Object is accessed, barrier needs to be made
                        // sensitive to it.
                        ql_operands.add(event.first.make_reference(ir));

                    } else if (event.second == com::ddg::AccessMode::WRITE) {

                        // Null reference (unknown state) is mutated, so the
                        // barrier needs to be sensitive to everything.
                        ql_operands.reset();
                        break;

                    }
                }

                // Construct barriers sensitive to all used objects and add them
                // to the front and back of the "bundle".
                auto ql_barrier_begin = make_instruction(ir, "barrier", ql_operands);
                auto ql_barrier_end = ql_barrier_begin.clone();
                ql_barrier_begin->cycle = ql_bundle.front()->cycle;
                ql_barrier_end->cycle = ql_bundle.back()->cycle;
                ql_bundle.push_front(ql_barrier_begin);
                ql_bundle.push_back(ql_barrier_end);

            }

            // Add the completed bundle to the block.
            for (const auto &ql_insn : ql_bundle) {
                ql_block->statements.add(ql_insn);
            }

            // The cycle counter increments at the end of each bundle if
            // scheduling information is retained. Otherwise it is incremented
            // at the end of each instruction.
            if (options.schedule_mode == ScheduleMode::KEEP) {
                cycle++;
            }

        } else if (auto cq_if_else = cq_stmt->as_if_else()) {

            // Handle if-else chain.
            auto ql_if_else = utils::make<IfElse>();
            ql_if_else->cycle = cycle;

            // Handle all the if-else branches.
            for (const auto &cq_branch : cq_if_else->branches) {
                auto ql_branch = utils::make<IfElseBranch>();

                // Convert condition.
                ql_branch->condition = convert_expression(ir, cq_branch->condition);
                auto ql_type = get_type_of(ql_branch->condition);
                if (!ql_type->as_bit_type()) {
                    QL_USER_ERROR(
                        "type of if condition (" + ql_type->name + ") is not bit-like"
                    );
                }

                // Convert body.
                ql_branch->body.emplace();
                convert_block(ir, cq_branch->body, ql_branch->body, options);

                ql_if_else->branches.add(ql_branch);
            }

            // Convert final else block.
            if (!cq_if_else->otherwise.empty()) {
                ql_if_else->otherwise.emplace();
                convert_block(ir, cq_if_else->otherwise, ql_if_else->otherwise, options);
            }

            // Add to block.
            ql_block->statements.add(ql_if_else);

        } else if (auto cq_for_loop = cq_stmt->as_for_loop()) {

            // Handle for loop.
            auto ql_for_loop = utils::make<ForLoop>();
            ql_for_loop->cycle = cycle;

            // Convert initialize assignment.
            if (!cq_for_loop->initialize.empty()) {
                ql_for_loop->initialize = convert_set_instruction(ir, *cq_for_loop->initialize);
            }

            // Convert loop condition.
            ql_for_loop->condition = convert_expression(ir, cq_for_loop->condition);
            auto ql_type = get_type_of(ql_for_loop->condition);
            if (!ql_type->as_bit_type()) {
                QL_USER_ERROR(
                    "type of for loop condition (" + ql_type->name + ") is not bit-like"
                );
            }

            // Convert update assignment.
            if (!cq_for_loop->update.empty()) {
                ql_for_loop->update = convert_set_instruction(ir, *cq_for_loop->update);
            }

            // Convert body.
            ql_for_loop->body.emplace();
            convert_block(ir, cq_for_loop->body, ql_for_loop->body, options);

            // Add to block.
            ql_block->statements.add(ql_for_loop);

        } else if (auto cq_foreach_loop = cq_stmt->as_foreach_loop()) {

            // Convert foreach loop.
            auto ql_static_loop = utils::make<StaticLoop>();
            ql_static_loop->cycle = cycle;

            // Convert the loop variable reference.
            ql_static_loop->lhs = convert_expression(ir, cq_foreach_loop->lhs).as<Reference>();
            if (ql_static_loop->lhs.empty()) {
                QL_USER_ERROR(
                    "loop variable is not assignable"
                );
            }

            // Convert the integer literals.
            auto ql_type = get_type_of(ql_static_loop->lhs);
            ql_static_loop->frm = make_int_lit(ir, cq_foreach_loop->frm, ql_type);
            ql_static_loop->to = make_int_lit(ir, cq_foreach_loop->to, ql_type);

            // Convert body.
            ql_static_loop->body.emplace();
            convert_block(ir, cq_foreach_loop->body, ql_static_loop->body, options);

            // Add to block.
            ql_block->statements.add(ql_static_loop);

        } else if (auto cq_while_loop = cq_stmt->as_while_loop()) {

            // Handle while loop.
            auto ql_for_loop = utils::make<ForLoop>();
            ql_for_loop->cycle = cycle;

            // Convert loop condition.
            ql_for_loop->condition = convert_expression(ir, cq_while_loop->condition);
            auto ql_type = get_type_of(ql_for_loop->condition);
            if (!ql_type->as_bit_type()) {
                QL_USER_ERROR(
                    "type of while loop condition (" + ql_type->name + ") is not bit-like"
                );
            }

            // Convert body.
            ql_for_loop->body.emplace();
            convert_block(ir, cq_while_loop->body, ql_for_loop->body, options);

            // Add to block.
            ql_block->statements.add(ql_for_loop);

        } else if (auto cq_repeat_until = cq_stmt->as_repeat_until_loop()) {

            // Handle while loop.
            auto ql_for_loop = utils::make<RepeatUntilLoop>();
            ql_for_loop->cycle = cycle;

            // Convert body.
            ql_for_loop->body.emplace();
            convert_block(ir, cq_repeat_until->body, ql_for_loop->body, options);

            // Convert loop condition.
            ql_for_loop->condition = convert_expression(ir, cq_repeat_until->condition);
            auto ql_type = get_type_of(ql_for_loop->condition);
            if (!ql_type->as_bit_type()) {
                QL_USER_ERROR(
                    "type of repeat-until loop condition (" + ql_type->name + ") is not bit-like"
                );
            }

            // Add to block.
            ql_block->statements.add(ql_for_loop);

        } else if (cq_stmt->as_break_statement()) {

            // Handle break statement.
            ql_block->statements.emplace<BreakStatement>(cycle);

        } else if (cq_stmt->as_continue_statement()) {

            // Handle continue statement.
            ql_block->statements.emplace<ContinueStatement>(cycle);

        } else {
            QL_ICE("received unknown statement node type from libqasm");
        }
    }
}

/**
 * Reads a cQASM 1.2 file into the IR. If reading is successful, ir->program is
 * completely replaced. data represents the cQASM file contents, fname specifies
 * the filename if one exists for the purpose of generating better error
 * messages.
 */
void read(
    const Ref &ir,
    const utils::Str &data,
    const utils::Str &fname,
    const ReadOptions &options
) {

    // Create an analyzer for files with a version up to cQASM 1.2.
    cq::analyzer::Analyzer a{"1.2"};

    // Add the default constant-propagation functions and mappings such as true
    // and false.
    a.register_default_functions_and_mappings();

    // Add typecast functions that explicitly cast cQASM's types to OpenQL's
    // types by attaching a type annotation to the incoming value. Without this
    // annotation, the chosen type will simply be the first applicable type
    // encountered. This is fine when for example an integer is encountered and
    // there is only one integer type in the platform, but when there are
    // different types, for example different register sizes, these typecast
    // will be needed.
    for (const auto &dt : ir->platform->data_types) {
        a.register_function(
            dt->name,
            {make_cq_type(dt)},
            [dt](const cqv::Values &ops) -> cqv::Value {
                ops[0]->set_annotation<DataTypeLink>(dt);
                return ops[0];
            }
        );
    }

    // Add registers as default mappings and builtin function calls.
    for (const auto &obj : ir->platform->objects) {
        if (ir->platform->qubits.links_to(obj)) {

            // Predefine the q and b registers as well. These will be overridden
            // to the same thing (possibly with a different size) if the cQASM
            // file includes a qubits statement, but that's fine. We'll just
            // throw an error if the user uses an out-of-range qubit or bit.
            if (obj->shape.size() != 1) {
                QL_ICE("main qubit register must be one-dimensional");
            }
            auto q = cqt::make<cqv::QubitRefs>();
            auto b = cqt::make<cqv::BitRefs>();
            for (utils::UInt i = 0; i < obj->shape[0]; i++) {
                q->index.add(cqt::make<cqv::ConstInt>(i));
                b->index.add(cqt::make<cqv::ConstInt>(i));
            }
            a.register_mapping("q", q);
            a.register_mapping("b", b);

        } else {

            // For registers, define a function that takes an integer argument
            // for each index dimension. The function always returns a builtin
            // function call object, which we'll convert to the appropriate
            // register reference after libqasm's analysis.
            cqty::Types types;
            for (utils::UInt i = 0; i < obj->shape.size(); i++) {
                types.emplace<cqty::Int>();
            }
            a.register_function(obj->name, types, [obj](const cqv::Values &ops) -> cqv::Value {
                return make_cq_register_ref(obj, ops);
            });

            // For scalar registers, define a mapping to that function with the
            // () added to it, so you don't have to specify ().
            if (obj->shape.empty()) {
                a.register_mapping(obj->name, make_cq_register_ref(obj, {}));
            }

        }
    }

    // Add regular builtin functions.
    // NOTE: any builtin function that shares a prototype with a default
    // constant-propagation function from libqasm is overridden. That means that
    // any constant propagation from that point onwards will need to be handled
    // by OpenQL. It also means that certain arcane constructs that would
    // otherwise be legal in cQASM won't work anymore. For example, if
    // operator+(int, int) is defined here, weird stuff like "qubits 1 + 2"
    // won't work anymore.
    for (const auto &fun : ir->platform->functions) {
        cqty::Types cq_types;
        for (const auto &ql_op_type : fun->operand_types) {
            cq_types.add(make_cq_op_type(ql_op_type));
        }
        a.register_function(fun->name, cq_types, [fun](const cqv::Values &ops) -> cqv::Value {
            cqv::Value cq_val = cqt::make<cqv::Function>(
                fun->name,
                ops,
                make_cq_type(fun->return_type)
            );
            cq_val->set_annotation(fun);
            cq_val->set_annotation<DataTypeLink>(fun->return_type);
            return cq_val;
        });
    }

    // Analyze the file. Note that we didn't add any instruction or error model
    // types, which disables libqasm's resolver. This lets us completely ignore
    // error models, and handle instruction resolution ourselves using our own
    // type system.
    auto res = a.analyze_string(data, fname);
    if (!res.errors.empty()) {
        utils::StrStrm errors;
        errors << "Failed to parse " << fname << " as cQASM 1.2 for the following reasons:";
        for (const auto &error : res.errors) {
            QL_EOUT(error);
            errors << "\n  " << error;
        }
        QL_USER_ERROR(errors.str());
    }
    auto cq_program = res.root;

    // Make a corresponding OpenQL program node.
    auto ql_program = utils::make<Program>();

    // If a program node already exists in the IR, use its name. Otherwise,
    // we'll have to come up with a name of our own.
    if (!ir->program.empty()) {
        ql_program->name = ir->program->name;
        ql_program->unique_name = ir->program->unique_name;

        // The name can also be set with @ql.name, whether we're actually using
        // it or not. Which means we need to set the Used flag for it, otherwise
        // there'll be an unused-pragma exception at the end.
        if (!cq_program->subcircuits.empty()) {
            auto annot = find_pragma(cq_program->subcircuits[0], "name");
            if (!annot.empty()) annot->set_annotation<Used>({});
        }

    } else {

        // Default to just "program".
        ql_program->name = "program";

        // Look for a ql.name pragma in the first subcircuit to override the
        // default.
        if (!cq_program->subcircuits.empty()) {
            auto annot = find_pragma(cq_program->subcircuits[0], "name");
            if (!annot.empty()) {
                ql_program->name = parse_name_annotation(annot, false);
            }
        }

        // Figure out a unique name for this name if needed.
        ql_program->unique_name = compat::make_unique_name(ql_program->name);

    }

    // Platform information can be attached to the cQASM file, even though we're
    // not really using that yet. Accept the annotation either way.
    if (!cq_program->subcircuits.empty()) {
        auto annot = find_pragma(cq_program->subcircuits[0], "platform");
        if (!annot.empty()) annot->set_annotation<Used>({});
    }

    // Create variables.
    for (const auto &cq_variable : cq_program->variables) {

        // Read annotations.
        utils::Str ql_name = cq_variable->name;
        DataTypeLink ql_type;
        utils::Bool is_temp = false;
        for (const auto &annot : cq_variable->annotations) {
            if (annot->interface != "ql") continue;
            if (annot->operation == "name") {
                ql_name = parse_name_annotation(annot);
            } else if (annot->operation == "type") {
                ql_type = parse_type_annotation(ir, annot);
            } else if (annot->operation == "temp") {
                if (!annot->operands.empty()) {
                    QL_USER_ERROR("@ql.temp does not take any arguments");
                }
                is_temp = true;
            }
        }

        // If there was no @ql.type, find the first applicable type in the
        // platform.
        if (ql_type.empty()) {
            ql_type = infer_ql_type(ir, cq_variable->typ);
        }

        // Create the object.
        utils::One<VirtualObject> ql_object;
        if (is_temp) {
            ql_object.emplace<TemporaryObject>("", ql_type);
        } else {
            ql_object.emplace<VariableObject>(ql_name, ql_type);
        }

        // Add it to the OpenQL program tree.
        ql_program->objects.add(ql_object);

        // Also add it to the cQASM tree as an annotation, so we can resolve
        // references later.
        cq_variable->set_annotation<ObjectLink>(ql_object);

    }

    // See if there is a default subcircuit (nameless) with nothing but pragmas
    // in it. In that case, it's ignored.
    if (
        !cq_program->subcircuits.empty() &&
        cq_program->subcircuits[0]->name.empty() &&
        cq_program->subcircuits[0]->iterations == 1
    ) {
        auto empty = true;
        for (const auto &stmt : cq_program->subcircuits[0]->body->statements) {
            if (auto bun = stmt->as_bundle_ext()) {
                for (auto &insn_base : bun->items) {
                    if (auto insn = insn_base->as_instruction()) {
                        if (insn->name != "pragma") {
                            empty = false;
                        }
                    } else {
                        empty = false;
                    }
                    if (!empty) break;
                }
            } else {
                empty = false;
            }
            if (!empty) break;
        }
        if (empty) {
            check_all_annotations_used(cq_program->subcircuits[0]);
            cq_program->subcircuits.remove(0);
        }
    }

    // If the program has no (more) subcircuits, infer a default block.
    if (cq_program->subcircuits.empty()) {
        ql_program->blocks.emplace();
        ql_program->entry_point = ql_program->blocks[0];
    } else {

        // See if the first subcircuit is annotated with @ql.entry, indicating
        // that it serves as a placeholder for the entry point parameter and
        // isn't a real block.
        cqt::Link<cqs::Subcircuit> cq_entry = cq_program->subcircuits[0];
        if (auto annot = find_annotation(cq_program->subcircuits[0]->annotations, "entry")) {
            annot->set_annotation<Used>({});
            auto x = find_last_goto_instruction(cq_program->subcircuits[0]);
            if (!x.first || !x.second) {
                QL_USER_ERROR(
                    "subcircuit marked @ql.entry must consist of exactly "
                    "one unconditional goto instruction"
                );
            }
            cq_entry = x.first->target;
            if (cq_entry.links_to(cq_program->subcircuits[0])) {
                QL_USER_ERROR(
                    "subcircuit marked @ql.entry cannot jump to itself"
                );
            }
            check_all_annotations_used(cq_program->subcircuits[0]);
            cq_program->subcircuits.remove(0);
        }
        QL_ASSERT(!cq_program->subcircuits.empty());

        // See if the last subcircuit is annotated with @ql.exit, indicating
        // that it serves as a placeholder for program exit and isn't a real
        // block.
        cqt::Link<cqs::Subcircuit> cq_exit;
        if (auto annot = find_annotation(cq_program->subcircuits.back()->annotations, "exit")) {
            annot->set_annotation<Used>({});
            if (!cq_program->subcircuits.back()->body->statements.empty()) {
                QL_USER_ERROR(
                    "subcircuit marked @ql.exit must be empty"
                );
            }
            check_all_annotations_used(cq_program->subcircuits.back());
            cq_program->subcircuits.remove(-1);
        }

        // The program could now be empty again, so check for that.
        if (cq_program->subcircuits.empty()) {
            ql_program->blocks.emplace();
            ql_program->entry_point = ql_program->blocks[0];
        } else {

            // Create empty blocks for every subcircuit and link them up into
            // a linear chain for now.
            utils::One<Block> prev;
            for (const auto &subc : cq_program->subcircuits) {
                auto block = utils::make<Block>(subc->name);
                subc->set_annotation<utils::One<Block>>(block);
                ql_program->blocks.add(block);
                if (!prev.empty()) prev->next = block;
                prev = block;
            }

            // Link the entry point to either the start of said chain, or to
            // wherever the goto instruction in the @ql.entry block is pointed.
            ql_program->entry_point = cq_entry->get_annotation<utils::One<Block>>();

            // Now handle the contents of the subcircuits.
            for (const auto &cq_subc : cq_program->subcircuits) {
                auto ql_block = cq_subc->get_annotation<utils::One<Block>>();

                // Interpret the last unconditional goto instruction in each
                // subcircuit (if any) as the "next" target for each block,
                // rather than as a normal instruction.
                auto goto_insn = find_last_goto_instruction(cq_subc).first;
                if (goto_insn) {
                    ql_block->next = goto_insn->target->get_annotation<utils::One<Block>>();

                    // The goto instruction is necessarily the last instruction
                    // in the last statement, which must be BundleExt for it to
                    // be there. Remove it because we've used it now.
                    cq_subc->body->statements.back().as<cqs::BundleExt>()->items.remove(-1);

                }

                // Convert the rest of the block.
                convert_block(ir, cq_subc->body, ql_block, options);

                // Make sure no unused @ql.* annotations remain.
                check_all_annotations_used(cq_program->subcircuits.back());

            }

        }

    }

    // Looks like conversion was successful.
    ir->program = ql_program;
    check_consistency(ir);

}

/**
 * Same as read(), but given a file to load, rather than loading from a string.
 */
void read_file(
    const Ref &ir,
    const utils::Str &fname,
    const ReadOptions &options
) {
    read(ir, utils::InFile(fname).read(), fname, options);
}

} // namespace cqasm
} // namespace ir
} // namespace ql
