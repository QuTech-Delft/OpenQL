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
            utils::StrStrm ss;
            ss << what << " \"" << s << "\" is not a valid identifier";
            throw utils::Exception(ss.str());
        }
    }

    /**
     * Checks that the cycle numbers of the instructions in the given
     * statement list are non-decreasing.
     */
    static void check_cycles_non_decreasing(const utils::Str &what, const utils::Any<Statement> &stmts) {
        utils::UInt current = 0;
        for (const auto &stmt : stmts) {
            if (auto insn = stmt->as_instruction()) {
                if (insn->cycle < current) {
                    utils::StrStrm ss;
                    ss << "cycle numbers in " << what << " are non-decreasing: ";
                    ss << "cycle " << insn->cycle << " used after cycle " << current;
                    throw utils::Exception(ss.str());
                }
                current = insn->cycle;
            }
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
            throw utils::Exception(
                "actual vs expected operand count mismatch for " + what
            );
        }
        for (utils::UInt i = 0; i < actual.size(); i++) {
            if (get_type_of(actual[i]) != expected[i]->data_type) {
                throw utils::Exception(
                    "actual vs expected data type mismatch for operand " +
                    utils::to_string(i) + " of " + what
                );
            }
            switch (expected[i]->mode) {
                case prim::AccessMode::LITERAL:
                    if (!actual[i]->as_literal()) {
                        throw utils::Exception(
                            "operand " + utils::to_string(i) + " of " + what +
                            "must be a literal, but isn't"
                        );
                    }
                    break;
                case prim::AccessMode::READ:
                    break;
                default:
                    if (!is_assignable_or_qubit(actual[i])) {
                        throw utils::Exception(
                            "operand " + utils::to_string(i) + " of " + what +
                            "must be a assignable, but isn't"
                        );
                    }
                    break;
            }
        }
    }

public:

    /**
     * Behavior for unknown node types. Assume that means that no check is
     * needed.
     */
    void visit_node(Node &node) override {
    }

    /**
     * Checks a platform node.
     */
    void visit_platform(Platform &node) override {
        RecursiveVisitor::visit_platform(node);

        // Check uniqueness and ordering of the data type names.
        if (!std::is_sorted(
            node.data_types.get_vec().begin(),
            node.data_types.get_vec().end(),
            [](const utils::One<DataType> &lhs, const utils::One<DataType> &rhs) {
                if (lhs->name == rhs->name) {
                    throw utils::Exception("duplicate data type name " + lhs->name);
                } else {
                    return lhs->name < rhs->name;
                }
            }
        )) {
            throw utils::Exception("data types are not ordered by name");
        }

        // Check ordering of the instruction names.
        if (!std::is_sorted(
            node.instructions.get_vec().begin(),
            node.instructions.get_vec().end(),
            [](const utils::One<InstructionType> &lhs, const utils::One<InstructionType> &rhs) {
                return lhs->name < rhs->name;
            }
        )) {
            throw utils::Exception("instruction types are not ordered by name");
        }

        // Check that all toplevel instruction types are fully generalized.
        for (const auto &insn_type : node.instructions) {
            if (!insn_type->generalization.empty() || !insn_type->template_operands.empty()) {
                throw utils::Exception(
                    "toplevel entry for instruction type \"" + insn_type->name +
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
            throw utils::Exception("function types are not ordered by name");
        }

        // Check uniqueness and ordering of the physical object names.
        if (!std::is_sorted(
            node.objects.get_vec().begin(),
            node.objects.get_vec().end(),
            [](const utils::One<PhysicalObject> &lhs, const utils::One<PhysicalObject> &rhs) {
                if (lhs->name == rhs->name) {
                    throw utils::Exception("duplicate physical object name " + lhs->name);
                } else {
                    return lhs->name < rhs->name;
                }
            }
        )) {
            throw utils::Exception("physical objects are not ordered by name");
        }

        // Check data type of main qubit type.
        if (!node.qubits->data_type->as_qubit_type()) {
            throw utils::Exception("main qubit register is not of a qubit-like data type");
        }

        // Check the implicit bit type.
        if (!node.implicit_bit_type.empty()) {
            if (!node.implicit_bit_type->as_bit_type()) {
                throw utils::Exception("implicit bit type must be a bit-like type");
            }
        }
        implicit_bit_type = node.implicit_bit_type;

        // Check existence of the topology, architecture, and resources objects.
        if (!node.topology.is_populated()) {
            throw utils::Exception("IR is missing topology information");
        }
        if (!node.architecture.is_populated()) {
            throw utils::Exception("IR is missing architecture information");
        }
        if (!node.resources.is_populated()) {
            throw utils::Exception("IR is missing resource information");
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
            throw utils::Exception(
                "encountered integer data type that is 0 bits in size"
            );
        }
        if (node.is_signed) {
            if (node.bits > 64) {
                throw utils::Exception(
                    "encountered signed integer data type more than 64 bits in size"
                );
            }
        } else {
            if (node.bits > 63) {
                throw utils::Exception(
                    "encountered unsigned integer data type more than 63 bits in size"
                );
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
            throw utils::Exception(
                "encountered matrix type with illegal number of rows "
                "(" + utils::to_string(node.num_rows) + ")"
            );
        }
        if (node.num_cols < 1) {
            throw utils::Exception(
                "encountered matrix type with illegal number of columns "
                "(" + utils::to_string(node.num_cols) + ")"
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
                throw utils::Exception(
                    "invalid specialization for \"" + spec->name + "\": "
                    "has different name"
                );
            }
            if (spec->cqasm_name != node.cqasm_name) {
                throw utils::Exception(
                    "invalid specialization for \"" + spec->name + "\": "
                    "has different cQASM name"
                );
            }

            // ... the first element of operand_types removed;
            if (spec->operand_types.size() != node.operand_types.size() - 1) {
                throw utils::Exception(
                    "invalid specialization for \"" + spec->name + "\": "
                    "mismatched operand count"
                );
            }
            for (utils::UInt i = 0; i < spec->operand_types.size(); i++) {
                if (!spec->operand_types[i].equals(node.operand_types[i + 1])) {
                    throw utils::Exception(
                        "invalid specialization for \"" + spec->name + "\": "
                        "mismatched operand types"
                    );
                }
            }

            // ... an additional element at the end of template_operands;
            if (spec->template_operands.size() != node.template_operands.size() + 1) {
                throw utils::Exception(
                    "invalid specialization for \"" + spec->name + "\": "
                    "mismatched template operand count"
                );
            }
            for (utils::UInt i = 0; i < spec->template_operands.size() - 1; i++) {
                if (!spec->template_operands[i].equals(node.template_operands[i])) {
                    throw utils::Exception(
                        "invalid specialization for \"" + spec->name + "\": "
                        "mismatched template operands"
                    );
                }
            }

            // ... the type of said element must match the removed operand_type
            // element; and...
            if (get_type_of(spec->template_operands.back()) != node.operand_types.front()->data_type) {
                throw utils::Exception(
                    "invalid specialization for \"" + spec->name + "\": "
                    "type mismatch for additional template operand"
                );
            }

            // ... generalization must link back to this node.
            if (&*spec->generalization != &node) {
                throw utils::Exception(
                    "invalid specialization for \"" + spec->name + "\": "
                    "generalization does not link back correctly"
                );
            }

        }

        // Check the decompositions.
        for (const auto &dec : node.decompositions) {
            if (dec->parameters.size() != node.operand_types.size()) {
                throw utils::Exception(
                    "invalid decomposition for \"" + node.name + "\": "
                    "parameter object count mismatch"
                );
            }
            for (utils::UInt i = 0; i < dec->parameters.size(); i++) {
                if (dec->parameters[i]->data_type != node.operand_types[i]->data_type) {
                    throw utils::Exception(
                        "invalid decomposition for \"" + node.name + "\": "
                        "parameter object " + utils::to_string(i) + " type mismatch"
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
            if (optyp->mode != prim::AccessMode::READ && optyp->mode != prim::AccessMode::LITERAL) {
                throw utils::Exception(
                    "function " + node.name + " writes to one of its operands"
                );
            }
        }

        // Match the operand types of the instruction decomposition if there is
        // one.
        if (!node.decomposition.empty()) {
            auto expected = node.operand_types;
            if (auto fix = node.decomposition->return_location->as_return_in_fixed_object()) {
                if (fix->object->data_type != node.return_type) {
                    throw utils::Exception(
                        "return location type mismatch in decomposition of function " + node.name
                    );
                }
            } else if (auto ded = node.decomposition->return_location->as_return_in_dedicated_operand()) {
                if (ded->index > expected.size()) {
                    throw utils::Exception(
                        "invalid return location index in decomposition of function " + node.name
                    );
                }
                expected.add(utils::make<OperandType>(
                    prim::AccessMode::WRITE,
                    node.return_type
                ), ded->index);
            } else if (auto sha = node.decomposition->return_location->as_return_in_shared_operand()) {
                if (sha->index >= expected.size()) {
                    throw utils::Exception(
                        "invalid return location index in decomposition of function " + node.name
                    );
                }
                if (expected[sha->index]->data_type != node.return_type) {
                    throw utils::Exception(
                        "return location type mismatch in decomposition of function " + node.name
                    );
                }
                expected[sha->index]->mode = prim::AccessMode::WRITE;
            } else {
                throw utils::Exception("unknown return location type encountered");
            }
            const auto &actual = node.decomposition->instruction_type->operand_types;
            if (expected.size() != actual.size()) {
                throw utils::Exception(
                    "prototype mismatch in decomposition of function " + node.name
                );
            }
            for (utils::UInt i = 0; i < expected.size(); i++) {
                if (expected[i]->data_type != actual[i]->data_type) {
                    throw utils::Exception(
                        "prototype mismatch in decomposition of function " + node.name +
                        ": type mismatch for instruction operand index " + utils::to_string(
                            i + node.decomposition->instruction_type->template_operands.size()
                        )
                    );
                }
                if (
                    expected[i]->mode == prim::AccessMode::WRITE &&
                    actual[i]->mode != prim::AccessMode::WRITE
                ) {
                    throw utils::Exception(
                        "prototype mismatch in decomposition of function " + node.name +
                        ": instruction operand index " + utils::to_string(
                            i + node.decomposition->instruction_type->template_operands.size()
                        ) + " is not marked as writable as demanded by function"
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
            throw utils::Exception("physical object is missing name " + node.name);
        }

    }

    /**
     * Checks an operand type.
     */
    void visit_operand_type(OperandType &node) override {
        RecursiveVisitor::visit_operand_type(node);

        // Check type/access-mode consistency.
        switch (node.mode) {
            case prim::AccessMode::WRITE:
                // Used for both qubits and classical data.
                break;
            case prim::AccessMode::READ:
            case prim::AccessMode::LITERAL:
                if (!node.data_type->as_classical_type()) {
                    throw utils::Exception(
                        "encountered function/instruction parameter that "
                        "reads a non-classical type"
                    );
                }
                break;
            case prim::AccessMode::COMMUTE_X:
            case prim::AccessMode::COMMUTE_Y:
            case prim::AccessMode::COMMUTE_Z:
            case prim::AccessMode::MEASURE:
                if (!node.data_type->as_qubit_type()) {
                    throw utils::Exception(
                        "encountered function/instruction parameter marked "
                        "with a qubit commutation rule combined with a "
                        "non-qubit type"
                    );
                }
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
            throw utils::Exception(
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
                throw utils::Exception("duplicate block name " + node.name);
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
            throw utils::Exception(
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
        if (!is_assignable_or_qubit(node.lhs)) {
            throw utils::Exception(
                "encountered set instruction with non-assignable left-hand side"
            );
        }

        // Match the types of the LHS and RHS.
        if (get_type_of(node.lhs) != get_type_of(node.rhs)) {
            throw utils::Exception(
                "encountered set instruction with mismatched LHS/RHS types"
            );
        }

        // Qubits cannot be assigned.
        if (get_type_of(node.lhs)->as_qubit_type()) {
            throw utils::Exception(
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
            throw utils::Exception(
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
            (node.lhs->target->data_type != node.from->data_type) ||
            (node.lhs->target->data_type != node.to->data_type)
        ) {
            throw utils::Exception(
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
            throw utils::Exception(
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
            throw utils::Exception(
                "encountered break or continue statement outside of a loop"
            );
        }

    }

    /**
     * Checks a bit literal.
     */
    void visit_bit_literal(BitLiteral &node) override {
        RecursiveVisitor::visit_bit_literal(node);

        // Check type.
        if (!node.data_type->as_bit_type()) {
            throw utils::Exception(
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
                throw utils::Exception(
                    "encountered integer literal out of range for type " +
                    node.data_type->name + ": " + utils::to_string(node.value)
                );
            }
        } else {
            throw utils::Exception(
                "encountered int literal with non-int associated type"
            );
        }

    }

    /**
     * Checks a real number literal.
     */
    void visit_real_literal(RealLiteral &node) override {
        RecursiveVisitor::visit_real_literal(node);

        // Check type.
        if (!node.data_type->as_real_type()) {
            throw utils::Exception(
                "encountered real number literal with non-real associated type"
            );
        }

    }

    /**
     * Checks a complex number literal.
     */
    void visit_complex_literal(ComplexLiteral &node) override {
        RecursiveVisitor::visit_complex_literal(node);

        // Check type.
        if (!node.data_type->as_complex_type()) {
            throw utils::Exception(
                "encountered complex number literal with non-complex associated type"
            );
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
                throw utils::Exception(
                    "encountered matrix literal with incorrect size"
                );
            }
        } else {
            throw utils::Exception(
                "encountered real-valued matrix literal with non-matrix associated type"
            );
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
                throw utils::Exception(
                    "encountered matrix literal with incorrect size"
                );
            }
        } else {
            throw utils::Exception(
                "encountered complex-valued matrix literal with non-matrix associated type"
            );
        }

    }

    /**
     * Checks a string literal.
     */
    void visit_string_literal(StringLiteral &node) override {
        RecursiveVisitor::visit_string_literal(node);

        // Check type.
        if (!node.data_type->as_string_type()) {
            throw utils::Exception(
                "encountered string literal with non-string associated type"
            );
        }

    }

    /**
     * Checks a JSON literal.
     */
    void visit_json_literal(JsonLiteral &node) override {
        RecursiveVisitor::visit_json_literal(node);

        // Check type.
        if (!node.data_type->as_json_type()) {
            throw utils::Exception(
                "encountered JSON literal with non-JSON associated type"
            );
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
            throw utils::Exception(
                "encountered reference to object \"" + node.target->name +
                "\" using mismatched data type " + node.data_type->name
            );
        }

        // Check indices.
        if (node.indices.size() != node.target->shape.size()) {
            throw utils::Exception(
                "encountered reference to object \"" + node.target->name +
                "\" with mismatched shape"
            );
        }
        for (utils::UInt i = 0; i < node.indices.size(); i++) {
            if (!get_type_of(node.indices[i])->as_int_type()) {
                throw utils::Exception(
                    "encountered reference to object \"" + node.target->name +
                    "\" with non-integer index"
                );
            }
            if (auto ilit = node.indices[i]->as_int_literal()) {
                if (ilit->value < 0 || ilit->value >= (utils::Int)node.target->shape[i]) {
                    throw utils::Exception(
                        "encountered reference to object \"" + node.target->name +
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

    // First, check whether the tree itself is well-formed according to
    // tree-gen.
    ir.check_well_formed();

    // The well-formedness check doesn't check any of the additional constraints
    // that the IR imposes. The visitor pattern is great for doing checks like
    // this, because it recursively walks through the entire tree by default.
    ConsistencyChecker consistency_checker;
    ir->visit(consistency_checker);

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
