/** \file
 * Defines a consistency check function for the IR.
 */

#include "ql/ir/consistency.h"

#include <regex>
#include "ql/utils/exception.h"
#include "ql/utils/set.h"
#include "ql/ir/ops.h"

namespace ql {
namespace ir {

/**
 * Visitor class for consistency-checking the IR.
 */
class ConsistencyChecker : public RecursiveVisitor {
private:

    /**
     * The set of all block names encountered.
     */
    utils::Set<utils::Str> block_names;

    /**
     * Whether we're currently traversing the tree inside a loop.
     */
    utils::Bool in_loop = false;

    /**
     * Copy of the implicit bit type from the platform, for checking references.
     */
    utils::OptLink<DataType> implicit_bit_type;

    /**
     * Checks that the given string is a valid identifier.
     */
    static void check_identifier(const utils::Str &what, const utils::Str &s) {
        if (!std::regex_match(s, IDENTIFIER_RE)) {
            QL_ICE(what << " \"" << s << "\" is not a valid identifier");
        }
    }

    /**
     * Checks that the cycle numbers of the instructions in the given
     * statement list are non-decreasing and greater or equal to zero.
     */
    static void check_cycles_non_decreasing(const utils::Str &what, const utils::Any<Statement> &stmts) {
        utils::Int current = 0;
        for (const auto &stmt : stmts) {
            if (stmt->cycle < current) {
                QL_ICE(
                    "cycle numbers in " << what << " are not non-decreasing: "
                    "cycle " << stmt->cycle << " used after cycle " << current
                );
            }
            current = stmt->cycle;
        }
    }

    /**
     * Matches the operands of an instruction or function instance against the
     * operand types in the associated instruction or function type.
     */
    static void check_prototype(
        const utils::Str &what,
        const utils::Any<Expression> &actual,
        const utils::Any<OperandType> &expected
    ) {
        if (actual.size() != expected.size()) {
            QL_ICE("actual vs expected operand count mismatch for " << what);
        }
        for (utils::UInt i = 0; i < actual.size(); i++) {
            if (get_type_of(actual[i]) != expected[i]->data_type) {
                QL_ICE(
                    "actual vs expected data type mismatch for operand " <<
                    i << " of " << what
                );
            }
            switch (expected[i]->mode) {
                case prim::OperandMode::BARRIER:
                case prim::OperandMode::WRITE:
                case prim::OperandMode::UPDATE:
                    if (!actual[i]->as_reference()) {
                        QL_ICE(
                            "operand " << i << " of " << what <<
                            " must be a reference, but isn't"
                        );
                    }
                    break;

                case prim::OperandMode::READ:
                    break;

                // TODO: check, when we create this node, with prim::OperandMode::LITERAL
                //       why we don't create a Literal, but an Expression, or a Node, or a DataType, not sure what we do

                case prim::OperandMode::LITERAL:
                    if (!actual[i]->as_literal()) {
                        QL_ICE(
                            "operand " << i << " of " << what <<
                            " must be a literal, but isn't"
                        );
                    }
                    break;

                case prim::OperandMode::COMMUTE_X:
                case prim::OperandMode::COMMUTE_Y:
                case prim::OperandMode::COMMUTE_Z:
                case prim::OperandMode::MEASURE:
                    if (
                        !actual[i]->as_reference() ||
                        !actual[i]->as_reference()->data_type->as_qubit_type()
                    ) {
                        QL_ICE(
                            "operand " << i << " of " << what <<
                            " must be a qubit reference, but isn't"
                        );
                    }
                    break;

                case prim::OperandMode::IGNORED:
                    break;

            }
            if (auto ref = actual[i]->as_reference()) {
                if (ref->indices.size() != ref->target->shape.size()) {
                    QL_ICE(
                        "operand " << i << " of " << what <<
                        " does not have the correct amount of indices"
                    );
                }
            }
        }
    }

public:

    /**
     * Behavior for unknown node types. Assume that means that no check is
     * needed.
     */
    void visit_node(Node &) override {
    }

    /**
     * Checks a platform node.
     */
    void visit_platform(Platform &node) override {
        implicit_bit_type = node.implicit_bit_type;
        RecursiveVisitor::visit_platform(node);

        // Check uniqueness and ordering of the data type names.
        if (!std::is_sorted(
            node.data_types.get_vec().begin(),
            node.data_types.get_vec().end(),
            [](const utils::One<DataType> &lhs, const utils::One<DataType> &rhs) {
                if (lhs->name == rhs->name) {
                    QL_ICE("duplicate data type name " << lhs->name);
                } else {
                    return lhs->name < rhs->name;
                }
            }
        )) {
            QL_ICE("data types are not ordered by name");
        }

        // Check ordering of the instruction names.
        if (!std::is_sorted(
            node.instructions.get_vec().begin(),
            node.instructions.get_vec().end(),
            [](const utils::One<InstructionType> &lhs, const utils::One<InstructionType> &rhs) {
                return lhs->name < rhs->name;
            }
        )) {
            QL_ICE("instruction types are not ordered by name");
        }

        // Check that all toplevel instruction types are fully generalized.
        for (const auto &insn_type : node.instructions) {
            if (!insn_type->generalization.empty() || !insn_type->template_operands.empty()) {
                QL_ICE(
                    "toplevel entry for instruction type \"" << insn_type->name <<
                    "\" is not fully generic"
                );
            }
        }

        // Check ordering of the function names.
        if (!std::is_sorted(
            node.functions.get_vec().begin(),
            node.functions.get_vec().end(),
            [](const utils::One<FunctionType> &lhs, const utils::One<FunctionType> &rhs) {
                return lhs->name < rhs->name;
            }
        )) {
            QL_ICE("function types are not ordered by name");
        }

        // Check uniqueness and ordering of the physical object names.
        if (!std::is_sorted(
            node.objects.get_vec().begin(),
            node.objects.get_vec().end(),
            [](const utils::One<PhysicalObject> &lhs, const utils::One<PhysicalObject> &rhs) {
                if (lhs->name == rhs->name) {
                    QL_ICE("duplicate physical object name " << lhs->name);
                } else {
                    return lhs->name < rhs->name;
                }
            }
        )) {
            QL_ICE("physical objects are not ordered by name");
        }

        // Check data type of main qubit type.
        if (!node.qubits->data_type->as_qubit_type()) {
            QL_ICE("main qubit register is not of a qubit-like data type");
        }

        // Check the implicit bit type.
        if (!node.implicit_bit_type.empty()) {
            if (!node.implicit_bit_type->as_bit_type()) {
                QL_ICE("implicit bit type must be a bit-like type");
            }
        }

        // Check existence of the topology, architecture, and resources objects.
        if (!node.topology.is_populated()) {
            QL_ICE("IR is missing topology information");
        }
        if (!node.architecture.is_populated()) {
            QL_ICE("IR is missing architecture information");
        }
        if (!node.resources.is_populated()) {
            QL_ICE("IR is missing resource information");
        }

    }

    /**
     * Checks a data type node.
     */
    void visit_data_type(DataType &node) override {
        RecursiveVisitor::visit_data_type(node);

        // Check names. Note that uniqueness is tested by visit_platform().
        check_identifier("data type name", node.name);

    }

    /**
     * Checks an integer data type.
     */
    void visit_int_type(IntType &node) override {
        RecursiveVisitor::visit_int_type(node);

        // Check size limits.
        if (node.bits == 0) {
            QL_ICE("encountered integer data type that is 0 bits in size");
        }
        if (node.is_signed) {
            if (node.bits > 64) {
                QL_ICE("encountered signed integer data type more than 64 bits in size");
            }
        } else {
            if (node.bits > 63) {
                QL_ICE("encountered unsigned integer data type more than 63 bits in size");
            }
        }

    }

    /**
     * Checks a matrix data type.
     */
    void visit_matrix_type(MatrixType &node) override {
        RecursiveVisitor::visit_matrix_type(node);

        // Check size.
        if (node.num_rows < 1) {
            QL_ICE(
                "encountered matrix type with illegal number of rows "
                "(" << node.num_rows << ")"
            );
        }
        if (node.num_cols < 1) {
            QL_ICE(
                "encountered matrix type with illegal number of columns "
                "(" << node.num_cols << ")"
            );
        }

    }

    /**
     * Checks an instruction type node.
     */
    void visit_instruction_type(InstructionType &node) override {
        RecursiveVisitor::visit_instruction_type(node);

        // Check names. Note that uniqueness of the name/operand-types pair is
        // not actually checked; it's technically fine if there are more, that
        // just means the later ones are never utilized.
        check_identifier("instruction type name", node.name);
        check_identifier("instruction type cQASM name", node.cqasm_name);

        // Each specialization in this list must have...
        for (const auto &spec : node.specializations) {

            // ... the same name and cqasm_name;
            if (spec->name != node.name) {
                QL_ICE(
                    "invalid specialization for \"" << spec->name << "\": "
                    "has different name"
                );
            }
            if (spec->cqasm_name != node.cqasm_name) {
                QL_ICE(
                    "invalid specialization for \"" << spec->name << "\": "
                    "has different cQASM name"
                );
            }

            // ... the first element of operand_types removed;
            if (spec->operand_types.size() != node.operand_types.size() - 1) {
                QL_ICE(
                    "invalid specialization for \"" << spec->name << "\": "
                    "mismatched operand count"
                );
            }
            for (utils::UInt i = 0; i < spec->operand_types.size(); i++) {
                if (!spec->operand_types[i].equals(node.operand_types[i + 1])) {
                    QL_ICE(
                        "invalid specialization for \"" << spec->name << "\": "
                        "mismatched operand types"
                    );
                }
            }

            // ... an additional element at the end of template_operands;
            if (spec->template_operands.size() != node.template_operands.size() + 1) {
                QL_ICE(
                    "invalid specialization for \"" << spec->name << "\": "
                    "mismatched template operand count"
                );
            }
            for (utils::UInt i = 0; i < spec->template_operands.size() - 1; i++) {
                if (!spec->template_operands[i].equals(node.template_operands[i])) {
                    QL_ICE(
                        "invalid specialization for \"" << spec->name << "\": "
                        "mismatched template operands"
                    );
                }
            }

            // ... the type of said element must match the removed operand_type
            // element; and...
            if (get_type_of(spec->template_operands.back()) != node.operand_types.front()->data_type) {
                QL_ICE(
                    "invalid specialization for \"" << spec->name << "\": "
                    "type mismatch for additional template operand"
                );
            }

            // ... generalization must link back to this node.
            if (&*spec->generalization != &node) {
                QL_ICE(
                    "invalid specialization for \"" << spec->name << "\": "
                    "generalization does not link back correctly"
                );
            }

        }

        // Check the decompositions.
        for (const auto &dec : node.decompositions) {
            if (dec->parameters.size() != node.operand_types.size()) {
                QL_ICE(
                    "invalid decomposition for \"" << node.name << "\": "
                    "parameter object count mismatch"
                );
            }
            for (utils::UInt i = 0; i < dec->parameters.size(); i++) {
                if (dec->parameters[i]->data_type != node.operand_types[i]->data_type) {
                    QL_ICE(
                        "invalid decomposition for \"" << node.name << "\": "
                        "parameter object " << i << " type mismatch"
                    );
                }
            }
        }

    }

    /**
     * Checks an instruction decomposition node.
     */
    void visit_instruction_decomposition(InstructionDecomposition &node) override {
        RecursiveVisitor::visit_instruction_decomposition(node);

        // Check cycle order in expansion.
        check_cycles_non_decreasing("instruction decomposition", node.expansion);

    }

    /**
     * Checks a function type node, along with the decomposition and return
     * value location belonging to it.
     */
    void visit_function_type(FunctionType &node) override {
        RecursiveVisitor::visit_function_type(node);

        // Check names. Note that uniqueness of the name/operand-types pair is
        // not actually checked; it's technically fine if there are more, that
        // just means the later ones are never utilized. Note also that the
        // actual operator name is not checked (if this becomes an issue it
        // should be added).
        if (!utils::starts_with(node.name, "operator")) {
            check_identifier("function type name", node.name);
        }

        // Check the access mode of the operands. Functions must be free of side
        // effects, so the mode can only be read or literal.
        for (const auto &optyp : node.operand_types) {
            if (optyp->mode != prim::OperandMode::READ && optyp->mode != prim::OperandMode::LITERAL) {
                QL_ICE(
                    "function " << node.name << " writes to one of its operands"
                );
            }
        }

        // Match the operand types of the instruction decomposition if there is
        // one.
        if (!node.decomposition.empty()) {
            auto expected = node.operand_types;
            if (auto fix = node.decomposition->return_location->as_return_in_fixed_object()) {
                if (fix->object->data_type != node.return_type) {
                    QL_ICE(
                        "return location type mismatch in decomposition of function " << node.name
                    );
                }
            } else if (auto ded = node.decomposition->return_location->as_return_in_dedicated_operand()) {
                if (ded->index > expected.size()) {
                    QL_ICE(
                        "invalid return location index in decomposition of function " << node.name
                    );
                }
                expected.add(utils::make<OperandType>(
                    prim::OperandMode::WRITE,
                    node.return_type
                ), ded->index);
            } else if (auto sha = node.decomposition->return_location->as_return_in_shared_operand()) {
                if (sha->index >= expected.size()) {
                    QL_ICE(
                        "invalid return location index in decomposition of function " << node.name
                    );
                }
                if (expected[sha->index]->data_type != node.return_type) {
                    QL_ICE(
                        "return location type mismatch in decomposition of function " << node.name
                    );
                }
                expected[sha->index]->mode = prim::OperandMode::UPDATE;
            } else {
                QL_ICE("unknown return location type encountered");
            }
            const auto &actual = node.decomposition->instruction_type->operand_types;
            if (expected.size() != actual.size()) {
                QL_ICE(
                    "prototype mismatch in decomposition of function " << node.name
                );
            }
            for (utils::UInt i = 0; i < expected.size(); i++) {
                if (expected[i]->data_type != actual[i]->data_type) {
                    QL_ICE(
                        "prototype mismatch in decomposition of function " << node.name <<
                        ": type mismatch for instruction operand index " << (
                            i + node.decomposition->instruction_type->template_operands.size()
                        )
                    );
                }
                if (
                    (
                        expected[i]->mode == prim::OperandMode::WRITE ||
                        expected[i]->mode == prim::OperandMode::UPDATE
                    ) &&
                    actual[i]->mode != expected[i]->mode
                ) {
                    QL_ICE(
                        "prototype mismatch in decomposition of function " << node.name <<
                        ": instruction operand index " << (
                            i + node.decomposition->instruction_type->template_operands.size()
                        ) << " is not marked as " << expected[i]->mode << " as demanded by function"
                    );
                }
            }
        }

    }

    /**
     * Checks a regular object node.
     */
    void visit_object(Object &node) override {
        RecursiveVisitor::visit_object(node);

        // Check name.
        if (!node.name.empty()) {
            check_identifier("object name", node.name);
        } else if (node.as_physical_object()) {
            QL_ICE("physical object is missing name " << node.name);
        }

    }

    /**
     * Checks an operand type.
     */
    void visit_operand_type(OperandType &node) override {
        RecursiveVisitor::visit_operand_type(node);

        // Check type/access-mode consistency.
        switch (node.mode) {
            case prim::OperandMode::BARRIER:
            case prim::OperandMode::WRITE:
            case prim::OperandMode::UPDATE:
                // Used for both qubits and classical data.
                break;

            case prim::OperandMode::READ:
            case prim::OperandMode::LITERAL:
                if (!node.data_type->as_classical_type()) {
                    QL_ICE(
                        "encountered function/instruction parameter that "
                        "reads a non-classical type"
                    );
                }
                break;

            case prim::OperandMode::COMMUTE_X:
            case prim::OperandMode::COMMUTE_Y:
            case prim::OperandMode::COMMUTE_Z:
            case prim::OperandMode::MEASURE:
                if (!node.data_type->as_qubit_type()) {
                    QL_ICE(
                        "encountered function/instruction parameter marked "
                        "with a qubit commutation rule combined with a "
                        "non-qubit type"
                    );
                }
                break;

            case prim::OperandMode::IGNORED:
                break;
        }

    }

    /**
     * Checks the program node.
     */
    void visit_program(Program &node) override {
        RecursiveVisitor::visit_program(node);

        // Check validity of the entry point.
        utils::Bool ok = false;
        for (const auto &block : node.blocks) {
            if (node.entry_point.links_to(block)) {
                ok = true;
                break;
            }
        }
        if (!ok) {
            QL_ICE(
                "program entry point does not link to block in program root"
            );
        }

    }

    /**
     * Checks a sub-block.
     */
    void visit_sub_block(SubBlock &node) override {
        RecursiveVisitor::visit_sub_block(node);

        // Checks statement order.
        check_cycles_non_decreasing("block", node.statements);

    }

    /**
     * Checks a block.
     */
    void visit_block(Block &node) override {
        RecursiveVisitor::visit_block(node);

        // Check name.
        if (!node.name.empty()) {
            check_identifier("object name", node.name);
            if (!block_names.insert(node.name).second) {
                QL_ICE("duplicate block name " << node.name);
            }
        }

    }

    /**
     * Checks the condition expression of a conditional instruction.
     */
    void visit_conditional_instruction(ConditionalInstruction &node) override {
        RecursiveVisitor::visit_conditional_instruction(node);

        // Check the return type of the expression.
        if (!get_type_of(node.condition)->as_bit_type()) {
            QL_ICE(
                "encountered conditional instruction with non-boolean condition"
            );
        }

    }

    /**
     * Checks the operands of a regular instruction instance.
     */
    void visit_custom_instruction(CustomInstruction &node) override {
        RecursiveVisitor::visit_custom_instruction(node);

        // Check the actual vs expected operands.
        check_prototype(
            node.instruction_type->name + " instruction",
            node.operands,
            node.instruction_type->operand_types
        );

    }

    /**
     * Checks the operands of a set instruction.
     */
    void visit_set_instruction(SetInstruction &node) override {
        RecursiveVisitor::visit_set_instruction(node);

        // Check assignability.
        if (!node.lhs->as_reference()) {
            QL_ICE(
                "encountered set instruction with non-assignable left-hand side"
            );
        }

        // Match the types of the LHS and RHS.
        if (get_type_of(node.lhs) != get_type_of(node.rhs)) {
            QL_ICE(
                "encountered set instruction with mismatched LHS/RHS types"
            );
        }

        // Qubits cannot be assigned.
        if (get_type_of(node.lhs)->as_qubit_type()) {
            QL_ICE(
                "encountered set instruction operating on qubits"
            );
        }

    }

    /**
     * Checks the condition of an if-else branch.
     */
    void visit_if_else_branch(IfElseBranch &node) override {
        RecursiveVisitor::visit_if_else_branch(node);

        // Check the return type of the condition expression.
        if (!get_type_of(node.condition)->as_bit_type()) {
            QL_ICE(
                "encountered if conditional of non-boolean type"
            );
        }

    }

    /**
     * Tracks whether we're traversing inside a loop body.
     */
    void visit_loop(Loop &node) override {
        auto prev = in_loop;
        in_loop = true;
        RecursiveVisitor::visit_loop(node);
        in_loop = prev;
    }

    /**
     * Type-checks a static loop.
     */
    void visit_static_loop(StaticLoop &node) override {
        RecursiveVisitor::visit_static_loop(node);

        // Check that the types are consistent.
        if (
            (node.lhs->target->data_type != node.frm->data_type) ||
            (node.lhs->target->data_type != node.to->data_type)
        ) {
            QL_ICE(
                "data type mismatch in static loop"
            );
        }

    }

    /**
     * Checks the condition of a for loop.
     */
    void visit_dynamic_loop(DynamicLoop &node) override {
        RecursiveVisitor::visit_dynamic_loop(node);

        // Check the return type of the condition expression.
        if (!get_type_of(node.condition)->as_bit_type()) {
            QL_ICE(
                "encountered if conditional of non-boolean type"
            );
        }

    }

    /**
     * Checks the condition of a for loop.
     */
    void visit_loop_control_statement(LoopControlStatement &node) override {
        RecursiveVisitor::visit_loop_control_statement(node);

        // Loop control statements are only allowed within a loop.
        if (!in_loop) {
            QL_ICE(
                "encountered break or continue statement outside of a loop"
            );
        }

    }

    /**
     * Ensures that no sentinels have popped up in the IR.
     */
    void visit_sentinel_statement(SentinelStatement &) override {
        QL_ASSERT(false);
    }

    /**
     * Checks a bit literal.
     */
    void visit_bit_literal(BitLiteral &node) override {
        RecursiveVisitor::visit_bit_literal(node);

        // Check type.
        if (!node.data_type->as_bit_type()) {
            QL_ICE(
                "encountered bit literal with non-bit associated type"
            );
        }

    }

    /**
     * Checks an integer literal.
     */
    void visit_int_literal(IntLiteral &node) override {
        RecursiveVisitor::visit_int_literal(node);

        // Check type.
        if (auto ityp = node.data_type->as_int_type()) {
            if (node.value < get_min_int_for(*ityp) || node.value > get_max_int_for(*ityp)) {
                QL_ICE(
                    "encountered integer literal out of range for type " <<
                    node.data_type->name << ": " << node.value
                );
            }
        } else {
            QL_ICE("encountered int literal with non-int associated type");
        }

    }

    /**
     * Checks a real number literal.
     */
    void visit_real_literal(RealLiteral &node) override {
        RecursiveVisitor::visit_real_literal(node);

        // Check type.
        if (!node.data_type->as_real_type()) {
            QL_ICE("encountered real number literal with non-real associated type");
        }

    }

    /**
     * Checks a complex number literal.
     */
    void visit_complex_literal(ComplexLiteral &node) override {
        RecursiveVisitor::visit_complex_literal(node);

        // Check type.
        if (!node.data_type->as_complex_type()) {
            QL_ICE("encountered complex number literal with non-complex associated type");
        }

    }

    /**
     * Checks a real-valued matrix literal.
     */
    void visit_real_matrix_literal(RealMatrixLiteral &node) override {
        RecursiveVisitor::visit_real_matrix_literal(node);

        // Check type.
        if (auto mtyp = node.data_type->as_real_matrix_type()) {
            if (
                (mtyp->num_rows != node.value.size_rows()) ||
                (mtyp->num_cols != node.value.size_cols())
            ) {
                QL_ICE("encountered matrix literal with incorrect size");
            }
        } else {
            QL_ICE("encountered real-valued matrix literal with non-matrix associated type");
        }

    }

    /**
     * Checks a complex-valued matrix literal.
     */
    void visit_complex_matrix_literal(ComplexMatrixLiteral &node) override {
        RecursiveVisitor::visit_complex_matrix_literal(node);

        // Check type.
        if (auto mtyp = node.data_type->as_complex_matrix_type()) {
            if (
                (mtyp->num_rows != node.value.size_rows()) ||
                (mtyp->num_cols != node.value.size_cols())
            ) {
                QL_ICE("encountered matrix literal with incorrect size");
            }
        } else {
            QL_ICE("encountered complex-valued matrix literal with non-matrix associated type");
        }

    }

    /**
     * Checks a string literal.
     */
    void visit_string_literal(StringLiteral &node) override {
        RecursiveVisitor::visit_string_literal(node);

        // Check type.
        if (!node.data_type->as_string_type()) {
            QL_ICE("encountered string literal with non-string associated type");
        }

    }

    /**
     * Checks a JSON literal.
     */
    void visit_json_literal(JsonLiteral &node) override {
        RecursiveVisitor::visit_json_literal(node);

        // Check type.
        if (!node.data_type->as_json_type()) {
            QL_ICE("encountered JSON literal with non-JSON associated type");
        }

    }

    /**
     * Checks a reference.
     */
    void visit_reference(Reference &node) override {
        RecursiveVisitor::visit_reference(node);

        // Check data type.
        if (
            (node.data_type != node.target->data_type) &&
            (node.data_type != implicit_bit_type || !node.target->data_type->as_qubit_type())
        ) {
            QL_ICE(
                "encountered reference to object \"" << node.target->name <<
                "\" with type \"" << node.target->data_type->name <<
                "\" using mismatched data type " << node.data_type->name
            );
        }

        // Check indices.
        if (node.indices.size() > node.target->shape.size()) {
            QL_ICE(
                "encountered reference to object \"" << node.target->name <<
                "\" with mismatched shape"
            );
        }
        for (utils::UInt i = 0; i < node.indices.size(); i++) {
            if (!get_type_of(node.indices[i])->as_int_type()) {
                QL_ICE(
                    "encountered reference to object \"" << node.target->name <<
                    "\" with non-integer index"
                );
            }
            if (auto ilit = node.indices[i]->as_int_literal()) {
                if (ilit->value < 0 || ilit->value >= (utils::Int)node.target->shape[i]) {
                    QL_ICE(
                        "encountered reference to object \"" << node.target->name <<
                        "\" with static index out of range"
                    );
                }
            }
        }

    }

    /**
     * Checks the operands of a function call.
     */
    void visit_function_call(FunctionCall &node) override {
        RecursiveVisitor::visit_function_call(node);

        // Check the actual vs expected operands.
        check_prototype(
            node.function_type->name + " function",
            node.operands,
            node.function_type->operand_types
        );

    }

};

/**
 * Performs a consistency check of the IR. An exception is thrown if a problem
 * is found. The constraints checked by this must be met on any interface that
 * passes an IR reference, although actually checking it on every interface
 * might be detrimental for performance.
 */
void check_consistency(const Ref &ir) {
    try {

        // First, check whether the tree itself is well-formed according to
        // tree-gen.
        ir.check_well_formed();

        // The well-formedness check doesn't check any of the additional constraints
        // that the IR imposes. The visitor pattern is great for doing checks like
        // this, because it recursively walks through the entire tree by default.
        ConsistencyChecker consistency_checker;
        ir->visit(consistency_checker);

    } catch (utils::Exception &e) {

        // If the check fails, dump the tree.
        QL_EOUT(
            "IR consistency check failed, about to throw the exception. "
            "Here's the IR tree:"
        );
        ir->dump_seq(std::cerr);
        throw;

    }
}

/**
 * Determines whether the IR is in basic-block form. This returns true only if:
 *  - all statements in the program's blocks are instructions (i.e., no
 *    structured control-flow remains); and
 *  - no non-control-flow instruction follows a control-flow instruction in any
 *    block.
 *
 * The result is only valid if the incoming IR is consistent.
 */
utils::Bool is_basic_block_form(const Ref &ir) {
    for (const auto &blk : ir->program->blocks) {
        auto dataflow = true;
        for (const auto &stmt : blk->statements) {
            auto insn = stmt->as_instruction();
            if (!insn) {
                // Non-instruction statement encountered.
                return false;
            }
            if (insn->as_goto_instruction()) {
                dataflow = false;
            } else if (!dataflow) {
                // Non-control-flow instruction following control-flow.
                return false;
            }
        }
    }
    return true;
}

} // namespace ir
} // namespace ql
