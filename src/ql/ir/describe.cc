/** \file
 * Defines a serializer for generating single-line descriptions of certain IR
 * nodes, useful within error messages and debug messages.
 */

#include "ql/ir/describe.h"

#include "ql/ir/operator_info.h"

namespace ql {
namespace ir {

/**
 * Describes visited nodes into the given stream. The description aims to be a
 * one-liner that's comprehensible to a user; for example, a function type node
 * returns its prototype. This makes it a lot more useful for error messages
 * than the dumper that's automatically generated by tree-gen. Note however that
 * no description is defined for things that are inherently multiline, like
 * blocks.
 *
 * FIXME: visitors are defined to have mutable access to the tree, even though
 *  that doesn't make sense in this context. So we act like everything is
 *  const, and do an ugly constness cast in the describe() function. Much better
 *  would be to just extend tree-gen to also generate a ConstVisitor variant,
 *  but this is more work than it's worth right now.
 */
class DescribingVisitor : public Visitor<void> {
protected:

    /**
     * Stream to write the node description to.
     */
    std::ostream &ss;

    /**
     * Precedence level of the current surrounding expression. All visit
     * functions should leave this variable the way they found it (exceptions
     * aside, things are assumed to irreparably break on exception anyway), but
     * they may modify it mid-function before recursively calling other visitor
     * functions. Only the visit_function_call() node does this and uses this,
     * though. The logic is that parentheses must be printed if the current
     * precedence level is greater than the precedence of the operator to be
     * printed.
     */
    utils::UInt precedence = 0;

public:

    /**
     * Constructs the visitor.
     */
    explicit DescribingVisitor(std::ostream &ss) : ss(ss) {};

    void visit_node(Node &node) override {
        ss << "<UNKNOWN>";
    }

    void visit_root(Root &root) override {
        if (root.program.empty()) {
            ss << "empty root";
        } else {
            ss << "root for ";
            root.program.visit(*this);
        }
    }

    void visit_platform(Platform &platform) override {
        if (platform.name.empty()) {
            ss << "anonymous platform";
        } else {
            ss << "platform " << platform.name;
        }
    }

    void visit_data_type(DataType &data_type) override {
        ss << data_type.name;
    }

protected:
    utils::Bool print_instruction_type_prefix(InstructionType &instruction_type) {
        ss << instruction_type.name;
        if (instruction_type.cqasm_name != instruction_type.name) {
            ss << "/" << instruction_type.cqasm_name;
        }
        auto first = true;
        if (!instruction_type.template_operands.empty()) {
            auto generalization = instruction_type.generalization;
            if (!generalization->generalization.empty()) {
                generalization = generalization->generalization;
            }
            for (utils::UInt i = 0; i < instruction_type.template_operands.size(); i++) {
                if (!first) ss << ",";
                first = false;
                ss << " <";
                generalization->operand_types[i].visit(*this);
                ss << "> ";
                instruction_type.template_operands[i].visit(*this);
            }
        }
        return first;
    }

public:
    void visit_instruction_type(InstructionType &instruction_type) override {
        auto first = print_instruction_type_prefix(instruction_type);
        for (auto &opt : instruction_type.operand_types) {
            if (!first) ss << ",";
            first = false;
            ss << " <";
            opt.visit(*this);
            ss << ">";
        }
    }

    void visit_function_type(FunctionType &function_type) override {
        ss << function_type.name << "(";
        auto first = true;
        for (auto &opt : function_type.operand_types) {
            if (!first) ss << ", ";
            first = false;
            opt.visit(*this);
        }
        ss << ") -> ";
        function_type.return_type.visit(*this);
    }

    void visit_object(Object &object) override {
        if (object.name.empty()) {
            ss << "<anonymous>";
        } else {
            ss << object.name;
        }
        ss << ": ";
        object.data_type.visit(*this);
        if (!object.shape.empty()) {
            ss << "[";
            auto first = true;
            for (auto size : object.shape) {
                if (!first) ss << ", ";
                first = false;
                ss << size;
            }
            ss << "]";
        }
    }

    void visit_operand_type(OperandType &operand_type) override {
        switch (operand_type.mode) {
            case prim::OperandMode::BARRIER:   ss << "B:"; break;
            case prim::OperandMode::WRITE:     ss << "W:"; break;
            case prim::OperandMode::UPDATE:    ss << "U:"; break;
            case prim::OperandMode::READ:      ss << "R:"; break;
            case prim::OperandMode::LITERAL:   ss << "L:"; break;
            case prim::OperandMode::COMMUTE_X: ss << "X:"; break;
            case prim::OperandMode::COMMUTE_Y: ss << "Y:"; break;
            case prim::OperandMode::COMMUTE_Z: ss << "Z:"; break;
            case prim::OperandMode::MEASURE:   ss << "M:"; break;
            case prim::OperandMode::IGNORED:    ss << "I:"; break;
        }
        operand_type.data_type.visit(*this);
    }

    void visit_program(Program &program) override {
        if (program.name.empty()) {
            ss << "anonymous program";
        } else {
            ss << "program " << program.name;
        }
    }

    void visit_block(Block &block) override {
        if (block.name.empty()) {
            ss << "anonymous block";
        } else {
            ss << "block " << block.name;
        }
    }

    void visit_conditional_instruction(ConditionalInstruction &conditional_instruction) override {
        if (
            !conditional_instruction.condition->as_bit_literal() ||
            !conditional_instruction.condition->as_bit_literal()->value
        ) {
            ss << "cond (";
            conditional_instruction.condition.visit(*this);
            ss << ") ";
        }
    }

    void visit_custom_instruction(CustomInstruction &custom_instruction) override {
        visit_conditional_instruction(custom_instruction);
        auto first = print_instruction_type_prefix(*custom_instruction.instruction_type);
        for (utils::UInt i = 0; i < custom_instruction.operands.size(); i++) {
            if (!first) ss << ",";
            first = false;
            ss << " <";
            custom_instruction.instruction_type->operand_types[i].visit(*this);
            ss << "> ";
            custom_instruction.operands[i].visit(*this);
        }
    }

    void visit_set_instruction(SetInstruction &set_instruction) override {
        visit_conditional_instruction(set_instruction);
        set_instruction.lhs.visit(*this);
        ss << " = ";
        set_instruction.rhs.visit(*this);
    }

    void visit_goto_instruction(GotoInstruction &goto_instruction) override {
        visit_conditional_instruction(goto_instruction);
        ss << "goto ";
        goto_instruction.target.visit(*this);
    }

    void visit_wait_instruction(WaitInstruction &wait_instruction) override {
        ss << "wait";
        if (wait_instruction.duration) {
            ss << " " << wait_instruction.duration;
            if (wait_instruction.duration == 1) {
                ss << " cycle";
            } else {
                ss << " cycles";
            }
            if (!wait_instruction.objects.empty()) {
                ss << " after";
            }
        } else if (!wait_instruction.objects.empty()) {
            ss << " on";
        }
        auto first = true;
        for (auto &ref : wait_instruction.objects) {
            if (!first) ss << ",";
            first = false;
            ss << " ";
            ref.visit(*this);
        }
    }

    void visit_if_else(IfElse &if_else) override {
        ss << "if (";
        if_else.branches[0]->condition.visit(*this);
        ss << ") ...";
    }

    void visit_loop(Loop &loop) override {
        ss << "loop ...";
    }

    void visit_break_statement(BreakStatement &break_statement) override {
        ss << "break";
    }

    void visit_continue_statement(ContinueStatement &continue_statementn) override {
        ss << "continue";
    }

    void visit_sentinel_statement(SentinelStatement &source_instruction) override {
        ss << "SENTINEL";
    }

    void visit_bit_literal(BitLiteral &bit_literal) override {
        if (bit_literal.value) {
            ss << "true";
        } else {
            ss << "false";
        }
    }

    void visit_int_literal(IntLiteral &int_literal) override {
        ss << int_literal.value;
    }

    void visit_real_literal(RealLiteral &real_literal) override {
        ss << real_literal.value;
    }

    void visit_complex_literal(ComplexLiteral &complex_literal) override {
        ss << complex_literal.value;
    }

    void visit_real_matrix_literal(RealMatrixLiteral &real_matrix_literal) override {
        ss << real_matrix_literal.value;
    }

    void visit_complex_matrix_literal(ComplexMatrixLiteral &complex_matrix_literal) override {
        ss << complex_matrix_literal.value;
    }

    void visit_string_literal(StringLiteral &string_literal) override {
        auto esc = string_literal.value;
        esc = utils::replace_all(esc, "\\", "\\\\");
        esc = utils::replace_all(esc, "\"", "\\\"");
        ss << "\"" << esc << "\"";
    }

    void visit_json_literal(JsonLiteral &json_literal) override {
        ss << json_literal.value;
    }

    void visit_reference(Reference &reference) override {
        if (reference.data_type != reference.target->data_type) {
            ss << "(";
            reference.data_type.visit(*this);
            ss << ")";
        }
        if (reference.target->name.empty()) {
            ss << "<anonymous>";
        } else {
            ss << reference.target->name;
        }
        if (!reference.indices.empty()) {
            ss << "[";
            auto first = true;
            for (auto &index : reference.indices) {
                if (!first) ss << ", ";
                first = false;
                index.visit(*this);
            }
            ss << "]";
        }
    }

    void visit_function_call(FunctionCall &function_call) override {
        auto prev_precedence = precedence;
        auto op_inf = OPERATOR_INFO.find({
            function_call.function_type->name,
            function_call.operands.size()
        });
        if (op_inf == OPERATOR_INFO.end()) {

            // Reset precedence for the function operands.
            precedence = 0;
            ss << function_call.function_type->name << "(";
            auto first = true;
            for (auto &operand : function_call.operands) {
                if (!first) ss << ", ";
                first = false;
                operand.visit(*this);
            }
            ss << ")";

        } else {
            if (precedence > op_inf->second.precedence) {
                ss << "(";
            }

            ss << op_inf->second.prefix;
            if (function_call.operands.size() == 1) {

                // Print the only operand with this precedence level.
                // Associativity doesn't matter for unary operators because we
                // don't have postfix operators.
                precedence = op_inf->second.precedence;
                function_call.operands.front().visit(*this);

            } else if (function_call.operands.size() > 1) {

                // Print the first operand with this precedence level if
                // left-associative, or with one level higher precedence if
                // right-associative to force parentheses for equal precedence
                // in that case.
                precedence = op_inf->second.precedence;
                if (op_inf->second.associativity == OperatorAssociativity::RIGHT) {
                    precedence++;
                }
                function_call.operands.front().visit(*this);
                ss << op_inf->second.infix;

                // If this is a ternary operator, print the middle operand.
                // Always place parentheses around it in case it's another
                // operator with the same precedence; I don't think this is
                // actually necessary, but more readable in my opinion.
                if (function_call.operands.size() > 2) {
                    QL_ASSERT(function_call.operands.size() <= 3);
                    precedence = op_inf->second.precedence + 1;
                    function_call.operands[1].visit(*this);
                    ss << op_inf->second.infix2;
                }

                // Print the second operand with this precedence level if
                // right-associative, or with one level higher precedence if
                // left-associative to force parentheses for equal precedence
                // in that case.
                precedence = op_inf->second.precedence;
                if (op_inf->second.associativity == OperatorAssociativity::LEFT) {
                    precedence++;
                }
                function_call.operands.back().visit(*this);

            } else {
                QL_ASSERT(false);
            }

            precedence = prev_precedence;
            if (precedence > op_inf->second.precedence) {
                ss << ")";
            }
        }
        precedence = prev_precedence;
    }

};

/**
 * Gives a one-line description of a node.
 */
void describe(const Node &node, std::ostream &ss) {
    DescribingVisitor visitor(ss);

    // See fix-me note at top of DescribingVisitor. tl;dr visitors are defined
    // to always be mutable by tree-gen, but there's no reason for that here.
    const_cast<Node&>(node).visit(visitor);
}

/**
 * Gives a one-line description of a node.
 */
void describe(const utils::One<Node> &node, std::ostream &ss) {
    describe(*node, ss);
}

/**
 * Gives a one-line description of a node.
 */
utils::Str describe(const Node &node) {
    utils::StrStrm ss;
    describe(node, ss);
    return ss.str();
}

/**
 * Gives a one-line description of a node.
 */
utils::Str describe(const utils::One<Node> &node) {
    utils::StrStrm ss;
    describe(node, ss);
    return ss.str();
}

} // namespace ir
} // namespace ql
