/** \file
 * cQASM 1.2 writer logic as human-readable complement of the IR.
 */

#include "ql/ir/cqasm/write.h"

#include "ql/ir/describe.h"
#include "ql/ir/ir_gen_ex.h"
#include "ql/ir/operator_info.h"
#include "ql/ir/ops.h"
#include "ql/pass/ana/statistics/report.h"
#include "ql/version.h"

namespace ql {
namespace ir {
namespace cqasm {

/**
 * cQASM 1.2 writer implemented (more or less) using the visitor pattern.
 */
class Writer : public Visitor<void> {
protected:

    /**
     * Reference to the root node.
     */
    const Ref &ir;

    /**
     * The stream that we're writing to.
     */
    std::ostream &os;

    /**
     * Line prefix.
     */
    utils::Str line_prefix;

    /**
     * Additional options for the way in which the cQASM file is written.
     */
    const WriteOptions &options;

    /**
     * The current indentation level.
     */
    utils::Int indent = 0;

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

    /**
     * Starts a Line, after updating the indentation level by adding
     * `indent_delta` to it.
     *
     * WARNING: don't use sl() and el() with nonzero indent_delta in the same
     * line of <<. The order in which indent is updated is basically undefined
     * behavior!
     */
    std::string sl(utils::Int indent_delta = 0) {
        utils::StrStrm ss;
        indent += indent_delta;
        if (indent < 0) indent = 0;
        auto indent_remain = indent;
        while (indent_remain-- > 0) {
            ss << "    ";
        }
        return ss.str();
    }

    /**
     * Ends a Line, leaving `blank` blank lines and updating the indentation
     * level by adding `indent_delta` to it.
     *
     * WARNING: don't use sl() and el() with nonzero indent_delta in the same
     * line of <<. The order in which indent is updated is basically undefined
     * behavior!
     */
    std::string el(utils::UInt blank = 0, utils::Int indent_delta = 0) {
        utils::StrStrm ss;
        indent += indent_delta;
        if (indent < 0) indent = 0;
        do {
            ss << "\n" << line_prefix;
        } while (blank--);
        return ss.str();
    }

    /**
     * The set of all names currently in use or reserved.
     */
    utils::Set<utils::Str> names;

    /**
     * Map from tree node to uniquified name.
     */
    utils::Map<const void*, utils::Str> unique_names;

    /**
     * Generates a unique, valid identifier for the given node based on the
     * given desired name. Calling this multiple times for the same non-empty
     * node is guaranteed to return the same identifier. Calling this multiple
     * times for empty nodes will yield unique identifiers.
     */
    utils::Str uniquify(
        const utils::One<Node> &node,
        const utils::Str &desired_name
    ) {

        // See if we've uniquified the name for this node before.
        const void *key = node.get_ptr().get();
        if (key != nullptr) {
            auto it = unique_names.find(key);
            if (it != unique_names.end()) {
                return it->second;
            }
        }

        // Make a unique, valid identifier based on the desired name.
        auto name = std::regex_replace(desired_name, std::regex("[^a-zA-Z0-9_]"), "_");
        if (!std::regex_match(name, IDENTIFIER_RE)) name = "_" + name;
        auto unique_name = name;
        utils::UInt unique_idx = 1;
        while (!names.insert(unique_name).second) {
            unique_name = name + "_" + utils::to_string(unique_idx++);
        }
        QL_ASSERT(std::regex_match(unique_name, IDENTIFIER_RE));

        // Store the uniquified name in the map.
        if (key != nullptr) {
            unique_names.insert({key, unique_name});
        }

        return unique_name;
    }

    /**
     * Generates a unique, valid identifier.
     */
    utils::Str uniquify(const utils::Str &desired_name) {
        return uniquify({}, desired_name);
    }

    /**
     * Returns whether the target cQASM version is at least the given version.
     */
    utils::Bool version_at_least(const utils::Vec<utils::UInt> &version) {
        for (utils::UInt i = 0; i < utils::min<utils::UInt>(version.size(), options.version.size()); i++) {
            if (options.version[i] > version[i]) return true;
            if (options.version[i] < version[i]) return false;
        }
        if (version.size() > options.version.size()) return false;
        return true;
    }

public:

    /**
     * Constructs a writer for the given stream.
     */
    Writer(
        const Ref &ir,
        const WriteOptions &options,
        std::ostream &os = std::cout,
        const utils::Str &line_prefix = ""
    ) :
        ir(ir),
        os(os),
        line_prefix(line_prefix),
        options(options),
        names({

            // cQASM 1.2 keywords:
            "break", "cond", "continue", "else", "for", "foreach", "if",
            "map", "repeat", "set", "qubits", "until", "var", "while",

            // Default mappings that we probably shouldn't touch:
            "q", "b", "pi", "eu", "im", "true", "false",

            // Maybe don't auto-generate an identifier that is just an
            // underscore.
            "_"

        })
    {}

    /**
     * Fallback function.
     */
    void visit_node(Node &node) override {
        node.dump(std::cerr);
        QL_ICE("unexpected node type encountered while writing cQASM");
    }

    /**
     * Visitor function for `Root` nodes.
     */
    void visit_root(Root &node) override {

        // Write initial line prefix (if any).
        os << line_prefix;

        // Generate header.
        os << sl() << "# Generated by OpenQL " << OPENQL_VERSION_STRING;
        if (node.program.empty()) {    // NB: normal situation for io.cqasm.Read
            os << " for EMPTY program" << el();
        } else {
            os << " for program " << node.program->name << el();
        }
        os << sl() << "version " << options.version.to_string("", ".", "");
        os << el(1);

        // Generate body.
        node.platform->visit(*this);
        if (node.program.empty()) {
            QL_IOUT("empty program");
        } else {
            node.program->visit(*this);

            // Print program-wide statistics as comments at the end if requested.
            if (options.include_statistics) {
                os << el();
                pass::ana::statistics::report::dump(ir, node.program, os, line_prefix + "# ");
            }
        }

    }

    /**
     * Prints a variable.
     */
    void print_variable(const utils::One<Object> &obj) {

        // Check version.
        if (!version_at_least({1, 1})) {
            QL_USER_ERROR(
                "cannot print variable for object with name \"" << obj->name << "\"; "
                "minimum version is cQASM 1.1"
            );
        }

        // Write the variable name(s).
        auto name = uniquify(obj, obj->name);
        os << sl() << "var ";
        if (obj->shape.empty()) {
            os << name;
        } else {
            auto first = true;
            utils::Vec<utils::UInt> index(obj->shape.size(), 0);
            while (true) {
                if (!first) os << ", ";
                first = false;
                auto full_name = name + index.to_string("_", "_", "");
                if (!names.insert(full_name).second) {
                    QL_ICE("unrecoverable name conflict for indexed non-scalar object " << full_name);
                }
                os << full_name;
                auto carry_done = false;
                for (utils::UInt dim = obj->shape.size(); dim--; ) {
                    index[dim]++;
                    if (index[dim] >= obj->shape[dim]) {
                        index[dim] = 0;
                        continue;
                    }
                    carry_done = true;
                    break;
                }
                if (!carry_done) {
                    break;
                }
            }
        }
        os << ": ";

        // Write the type.
        if (obj->data_type->as_qubit_type()) {
            os << "qubit";
        } else if (obj->data_type->as_bit_type()) {
            os << "bit";
        } else if (obj->data_type->as_int_type()) {
            os << "int";
        } else if (obj->data_type->as_real_type()) {
            os << "real";
        } else if (obj->data_type->as_complex_type()) {
            os << "complex";
        } else if (obj->data_type->as_real_matrix_type()) {
            QL_USER_ERROR(
                "data type " + obj->data_type->name +
                " not supported for variables"
            );
        }

        // Add additional type information as annotations if
        // enabled.
        if (options.include_metadata) {

            // Annotate with the platform type name.
            if (
                obj->data_type != ir->platform->default_bit_type &&
                obj->data_type != ir->platform->default_int_type &&
                obj->data_type != ir->platform->qubits->data_type
            ) {
                os << " @ql.type(\"" << obj->data_type->name << "\")";
            }

            // Annotate the object type/name if necessary.
            if (obj->as_temporary_object()) {
                os << " @ql.temp()";
            } else if (name != obj->name) {
                os << " @ql.name(\"" << obj->name << "\")";
            }

        }

        os << el();
    }

    /**
     * Visitor function for `Platform` nodes.
     */
    void visit_platform(Platform &node) override {

        // Reserve names for the builtin instructions, functions, and objects
        // described in the platform.
        for (const auto &insn : node.instructions) {
            names.insert(insn->cqasm_name);
        }
        for (const auto &func : node.functions) {
            names.insert(func->name);
        }
        if (!options.registers_as_variables) {
            for (const auto &obj : node.objects) {
                names.insert(obj->name);
            }
        }

        // Print the size of the main qubit register for cQASM 1.0 or when
        // registers are to be made explicit.
        if (!version_at_least({1, 1}) || options.registers_as_variables) {
            QL_ASSERT(ir->platform->qubits->shape.size() == 1);
            os << sl() << "qubits " << ir->platform->qubits->shape[0];
            os << el(1);
        }

        // Print variables for the registers when requested.
        if (options.registers_as_variables) {
            for (const auto &obj : node.objects) {

                // The main qubit register is created using the qubits
                // statement; it gets special treatment in cQASM.
                if (node.qubits.links_to(obj)) {
                    continue;
                }

                // Print as a variable.
                try {
                    print_variable(obj);
                } catch (utils::Exception &e) {
                    if (!obj->name.empty()) {
                        e.add_context("while writing variable for register " + obj->name);
                    }
                    throw;
                }

            }
        }

        // Add a pragma with the platform JSON data.
        if (options.include_platform) {
            auto s = node.data->dump(2);
            QL_ASSERT(utils::starts_with(s, "{"));
            QL_ASSERT(utils::ends_with(s, "}"));
            s = std::regex_replace(s.substr(1, s.size() - 2), std::regex("\n"), "\n" + line_prefix);
            os << sl() << "pragma @ql.platform({|" << s.substr() << "|})";
            os << el(1);
        }

    }

    /**
     * Visitor function for `Program` nodes.
     */
    void visit_program(Program &node) override {

        // Add a pragma with the program name.
        if (options.include_metadata) {
            os << sl() << "pragma @ql.name(\"" << node.name << "\")";
            os << el(1);
        }

        // Dump variables.
        if (!node.objects.empty()) {
            for (const auto &obj : node.objects) {
                try {
                    print_variable(obj);
                } catch (utils::Exception &e) {
                    if (!obj->name.empty()) {
                        e.add_context("while writing variable " + obj->name);
                    }
                    throw;
                }
            }
        }

        // Handle subcircuit header indentation.
        indent++;

        // If the first block is not the entry point, write a goto instruction
        // in a special entry point subcircuit.
        if (!node.entry_point.links_to(node.blocks[0])) {
            if (!version_at_least({1, 2})) {
                QL_USER_ERROR("control-flow is not supported until cQASM 1.2");
            }
            os << el();
            os << sl(-1) << "." << uniquify("entry");
            if (options.include_metadata) {
                os << " @ql.entry()";
            }
            os << el(0, 1);
            os << sl() << "goto " << uniquify(node.entry_point.as_mut(), node.entry_point->name) << el();
        }

        // Print the blocks.
        utils::Str exit_name;
        for (utils::UInt idx = 0; idx < node.blocks.size(); idx++) {
            const auto &block = node.blocks[idx];

            // Write the block header.
            auto name = uniquify(block, block->name);
            os << el();
            os << sl(-1) << "." << name;
            if (options.include_metadata && name != block->name) {
                os << " @ql.name(\"" << block->name << "\")";
            }
            os << el(0, 1);

            // Write the statements.
            block->visit(*this);

            // Write the goto statement for the next block if needed.
            if (block->next.empty() && idx != node.blocks.size() - 1) {
                if (!version_at_least({1, 2})) {
                    QL_USER_ERROR("control-flow is not supported until cQASM 1.2");
                }
                if (exit_name.empty()) {
                    exit_name = uniquify("exit");
                }
                os << sl() << "goto " << exit_name << el();
            } else {
                utils::Link<Block> seq_next;
                if (idx < node.blocks.size() - 1) {
                    seq_next = node.blocks[idx + 1];
                }
                if (block->next != seq_next) {
                    if (!version_at_least({1, 2})) {
                        QL_USER_ERROR("control-flow is not supported until cQASM 1.2");
                    }
                    os << sl() << "goto " << uniquify(block->next.as_mut(), block->next->name) << el();
                }
            }

            // Print block-wide statistics as comments at the end if requested.
            if (options.include_statistics) {
                os << el();
                pass::ana::statistics::report::dump(ir, block, os, line_prefix + "    # ");
            }

        }

        // Print the exit label if needed.
        if (!exit_name.empty()) {
            os << el();
            os << sl(-1) << "." << exit_name;
            if (options.include_metadata) {
                os << " @ql.exit()";
            }
            os << el();
        }

    }

    /**
     * Prints a bundle of simultaneously-issued (w.r.t. the quantum time domain)
     * instructions.
     */
    void flush_bundle(utils::Any<Instruction> &bundle, utils::Int &cycle) {
        if (bundle.size() == 1) {
            bundle[0]->visit(*this);
            cycle++;
        } else if (!bundle.empty()) {
            os << sl() << "{ # start at cycle " << cycle;
            os << el(0, 1);
            for (const auto &pending_stmt : bundle) {
                pending_stmt->visit(*this);
            }
            os << sl(-1) << "}";
            os << el();
            cycle++;
        }
        bundle.reset();
    }

    /**
     * Visitor function for `BlockBase` nodes.
     */
    void visit_block_base(BlockBase &node) override {

        // Gather bundles before printing them, so we can format them a bit more
        // nicely.
        utils::Int cycle = 0;
        utils::Any<Instruction> bundle;

        // Loop over all the statements.
        for (const auto &stmt : node.statements) {
            auto insn = stmt.as<Instruction>();

            // If stmt/insn cannot be added to the current bundle because it's
            // scheduled in a different cycle or isn't a schedulable
            // instruction, flush it. Also, always flush when the include_timing
            // option is disabled, to prevent multiple instructions from being
            // bundled together.
            if (!options.include_timing || insn.empty() || insn->cycle != cycle) {
                flush_bundle(bundle, cycle);
                if (!insn.empty()) {

                    // Add a skip before the next bundle if necessary and if
                    // include_timing is enabled.
                    if (options.include_timing && insn->cycle > cycle) {
                        os << sl() << "skip " << (insn->cycle - cycle) << el();
                    }

                    cycle = insn->cycle;
                }
            }

            // Now insn (if stmt is an instruction) can be added to the bundle.
            if (!insn.empty()) {
                bundle.add(insn);
                continue;
            }

            // Print statements outside of bundles.
            stmt->visit(*this);

        }

        // Print any remaining bundles.
        flush_bundle(bundle, cycle);

        // cQASM readers have no awareness of the duration of instructions, but
        // semantically a block can only start when all instructions in the
        // previous block have completed. Therefore, we have to add a skip at
        // the end, to skip to the first cycle when all instructions have
        // completed.
        if (options.include_timing) {
            utils::UInt last = get_duration_of_block(node.copy());
            QL_ASSERT(cycle >= 0);
            if (last > (utils::UInt)cycle) {
                os << sl() << "skip " << (last - cycle) << el();
            }
        }

    }

    /**
     * Visitor function for `ConditionalInstruction` nodes.
     */
    void visit_conditional_instruction(ConditionalInstruction &node) override {

        // Don't print condition prefix if this is a trivial condition.
        auto blit = node.condition->as_bit_literal();
        if (blit && blit->value) return;

        // Print the condition.
        os << "cond (";
        node.condition.visit(*this);
        os << ") ";

    }

    /**
     * Visitor function for `CustomInstruction` nodes.
     */
    void visit_custom_instruction(CustomInstruction &node) override {
        os << sl();
        visit_conditional_instruction(node);
        os << node.instruction_type->cqasm_name;
        auto first = true;
        for (const auto &op : node.instruction_type->template_operands) {
            if (!first) {
                os << ",";
            }
            os << " ";
            op->visit(*this);
            first = false;
        }
        for (const auto &op : node.operands) {
            if (!first) {
                os << ",";
            }
            os << " ";
            op->visit(*this);
            first = false;
        }
        os << el();
    }

    /**
     * Visitor function for `SetInstruction` nodes.
     */
    void visit_set_instruction(SetInstruction &node) override {
        os << sl();
        visit_conditional_instruction(node);
        os << "set ";
        node.lhs->visit(*this);
        os << " = ";
        node.rhs->visit(*this);
        os << el();
    }

    /**
     * Visitor function for `GotoInstruction` nodes.
     */
    void visit_goto_instruction(GotoInstruction &node) override {
        if (!version_at_least({1, 2})) {
            QL_USER_ERROR("control-flow is not supported until cQASM 1.2");
        }
        os << sl();
        visit_conditional_instruction(node);
        os << "goto " << uniquify(node.target.as_mut(), node.target->name) << el();
    }

    /**
     * Visitor function for `WaitInstruction` nodes.
     */
    void visit_wait_instruction(WaitInstruction &node) override {
        switch (options.include_wait_instructions) {
            case WaitStyle::DISABLED: {
                // Ignore.
                break;
            }
            case WaitStyle::SIMPLE: {
                os << sl();
                if (node.objects.empty()) {
                    os << "wait " << node.duration;
                } else if (node.duration == 0) {
                    utils::Set<utils::UInt> qubits;
                    for (const auto &op : node.objects) {
                        if (
                            op->target == ir->platform->qubits &&
                            op->data_type == ir->platform->qubits->data_type &&
                            op->indices.size() == 1 &&
                            op->indices[0]->as_int_literal()
                        ) {
                            qubits.insert(op->indices[0]->as_int_literal()->value);
                        } else {
                            QL_USER_ERROR(
                                describe(op) << " cannot be represented as "
                                "target for a barrier using simple wait style"
                            );
                        }
                    }
                    os << "barrier q[";
                    auto it = qubits.begin();
                    QL_ASSERT(it != qubits.end());
                    os << *it;
                    ++it;
                    auto postponed = false;
                    while (it != qubits.end()) {
                        if (*it == *std::prev(it) + 1) {
                            postponed = true;
                            continue;
                        } else if (postponed) {
                            os << ":" << *std::prev(it);
                            postponed = false;
                        }
                        os << ", " << *it;
                        ++it;
                    }
                    if (postponed) {
                        os << ":" << *std::prev(it);
                    }
                    os << "]";
                } else {
                    QL_USER_ERROR(
                        "simple wait style lacks a barrier with nonzero duration"
                    );
                }
                os << el();
                break;
            }
            case WaitStyle::EXTENDED: {
                os << sl();
                auto first = true;
                if (node.duration == 0) {
                    os << "barrier";
                } else {
                    os << "wait " << node.duration;
                    first = false;
                }
                for (const auto &op : node.objects) {
                    if (!first) {
                        os << ",";
                    }
                    os << " ";
                    op->visit(*this);
                    first = false;
                }
                os << el();
                break;
            }
        }

    }

    /**
     * Visitor function for `IfElse` nodes.
     */
    void visit_if_else(IfElse &node) override {
        if (!version_at_least({1, 2})) {
            QL_USER_ERROR("control-flow is not supported until cQASM 1.2");
        }
        os << sl();
        for (utils::UInt idx = 0; idx < node.branches.size(); idx++) {
            os << "if (";
            node.branches[idx]->condition->visit(*this);
            os << ") {" << el(0, 1);
            node.branches[idx]->body->visit(*this);
            os << sl(-1) << "}";
            if (idx < node.branches.size() - 1 || !node.otherwise.empty()) {
                os << " else ";
            }
        }
        if (!node.otherwise.empty()) {
            os << "{" << el(0, 1);
            node.otherwise->visit(*this);
            os << sl(-1) << "}";
        }
        os << el();
    }

    /**
     * Visitor function for `StaticLoop` nodes.
     */
    void visit_static_loop(StaticLoop &node) override {
        if (!version_at_least({1, 2})) {
            QL_USER_ERROR("control-flow is not supported until cQASM 1.2");
        }
        os << sl() << "foreach (";
        node.lhs->visit(*this);
        os << " = ";
        node.frm->visit(*this);
        os << "..";
        node.to->visit(*this);
        os << ") {" << el(0, 1);
        node.body->visit(*this);
        os << sl(-1) << "}" << el();
    }

    /**
     * Visitor function for `ForLoop` nodes.
     */
    void visit_for_loop(ForLoop &node) override {
        if (!version_at_least({1, 2})) {
            QL_USER_ERROR("control-flow is not supported until cQASM 1.2");
        }
        if (node.initialize.empty() && node.update.empty()) {
            os << sl() << "while (";
            node.condition->visit(*this);
        } else {
            os << sl() << "for (";
            if (!node.initialize.empty()) {
                node.initialize->lhs->visit(*this);
                os << " = ";
                node.initialize->rhs->visit(*this);
            }
            os << "; ";
            node.condition->visit(*this);
            os << "; ";
            if (!node.update.empty()) {
                node.update->lhs->visit(*this);
                os << " = ";
                node.update->rhs->visit(*this);
            }
        }
        os << ") {" << el(0, 1);
        node.body->visit(*this);
        os << sl(-1) << "}" << el();
    }

    /**
     * Visitor function for `RepeatUntilLoop` nodes.
     */
    void visit_repeat_until_loop(RepeatUntilLoop &node) override {
        if (!version_at_least({1, 2})) {
            QL_USER_ERROR("control-flow is not supported until cQASM 1.2");
        }
        os << sl() << "repeat {" << el(0, 1);
        node.body->visit(*this);
        os << sl(-1) << "} until (";
        node.condition->visit(*this);
        os << ")" << el();
    }

    /**
     * Visitor function for `BreakStatement` nodes.
     */
    void visit_break_statement(BreakStatement &) override {
        if (!version_at_least({1, 2})) {
            QL_USER_ERROR("control-flow is not supported until cQASM 1.2");
        }
        os << sl() << "break" << el();
    }

    /**
     * Visitor function for `ContinueStatement` nodes.
     */
    void visit_continue_statement(ContinueStatement &) override {
        if (!version_at_least({1, 2})) {
            QL_USER_ERROR("control-flow is not supported until cQASM 1.2");
        }
        os << sl() << "break" << el();
    }

    /**
     * Visitor function for `BitLiteral` nodes.
     */
    void visit_bit_literal(BitLiteral &node) override {
        if (node.value) {
            os << "true";
        } else {
            os << "false";
        }
    }

    /**
     * Visitor function for `IntLiteral` nodes.
     */
    void visit_int_literal(IntLiteral &node) override {
        os << node.value;
    }

    /**
     * Prints a real number.
     */
    void print_real(utils::Real r) {

        // Accurately printing floating-point values is hard. Half the JSON
        // library is dedicated to it. So why not abuse it for printing
        // literals?
        utils::Json j{r};
        os << j[0];

    }

    /**
     * Visitor function for `RealLiteral` nodes.
     */
    void visit_real_literal(RealLiteral &node) override {
        print_real(node.value);
    }

    /**
     * Visitor function for `ComplexLiteral` nodes.
     */
    void visit_complex_literal(ComplexLiteral &node) override {
        os << "(";
        print_real(node.value.real());
        os << "+";
        print_real(node.value.imag());
        os << "*im)";
    }

    /**
     * Visitor function for `RealMatrixLiteral` nodes.
     */
    void visit_real_matrix_literal(RealMatrixLiteral &node) override {
        os << "[";
        for (utils::UInt row = 1; row <= node.value.size_rows(); row++) {
            if (row == 1) {
                os << "; ";
            }
            for (utils::UInt col = 1; col <= node.value.size_rows(); col++) {
                if (col == 1) {
                    os << ", ";
                }
                auto value = node.value.at(row, col);
                print_real(value);
            }
        }
    }

    /**
     * Visitor function for `ComplexMatrixLiteral` nodes.
     */
    void visit_complex_matrix_literal(ComplexMatrixLiteral &node) override {
        os << "[";
        for (utils::UInt row = 1; row <= node.value.size_rows(); row++) {
            if (row == 1) {
                os << "; ";
            }
            for (utils::UInt col = 1; col <= node.value.size_rows(); col++) {
                if (col == 1) {
                    os << ", ";
                }
                auto value = node.value.at(row, col);
                print_real(value.real());
                os << "+";
                print_real(value.imag());
                os << "*im";
            }
        }
    }

    /**
     * Visitor function for `StringLiteral` nodes.
     */
    void visit_string_literal(StringLiteral &node) override {
        os << '"';
        for (auto c : node.value) {
            if (c == '"') {
                os << "\\\"";
            } else if (c == '\n') {
                os << "\\n";
            } else if (c == '\r') {
                os << "\\r";
            } else if (c == '\\') {
                os << "\\\\";
            } else {
                os << c;
            }
        }
        os << '"';
    }

    /**
     * Visitor function for `JsonLiteral` nodes.
     */
    void visit_json_literal(JsonLiteral &node) override {
        auto s = node.value->dump();
        QL_ASSERT(utils::starts_with(s, "{"));
        QL_ASSERT(utils::ends_with(s, "}"));
        os << "{|" << s.substr(1, s.size() - 2) << "|}";
    }

    /**
     * Visitor function for `Reference` nodes.
     */
    void visit_reference(Reference &node) override {

        // Figure out the name and the way to print.
        utils::Str name;
        auto typecast = node.data_type != node.target->data_type;
        if (node.target == ir->platform->qubits) {
            if (node.data_type->as_bit_type()) {
                typecast = false;
                name = "b";
            } else {
                name = "q";
            }
        } else if (node.target->as_physical_object()) {
            name = node.target->name;
        } else {
            name = uniquify(node.target.as_mut(), node.target->name);
        }

        // Print the typecast function if needed.
        if (typecast) {
            os << node.data_type->name << "(";
        }

        // Print the name.
        os << name;

        // Handle indices.
        if (!node.indices.empty()) {

            if (node.target == ir->platform->qubits) {

                // For the main qubit register (and implicit bit register),
                // index using [].
                os << "[";
                node.indices[0]->visit(*this);
                os << "]";

            } else if (node.target->as_physical_object() || !options.registers_as_variables) {

                // cQASM doesn't natively support indexing for things other than
                // the main qubit register. But we can model the index operation
                // as a function call, that can be evaluated into an appropriate
                // reference when the cQASM file is parsed via libqasm's
                // constant propagation system.
                os << "(";
                auto first = true;
                for (const auto &index : node.indices) {
                    if (!first) {
                        os << ", ";
                    }
                    index->visit(*this);
                    first = false;
                }
                os << ")";

            } else {

                // We can support literal indices for variables by embedding
                // them in the variable name, i.e., defining a (scalar) cQASM
                // variable for every element in the non-scalar OpenQL variable.
                // Dynamic indexing is obviously not supported this way, though.
                for (const auto &index : node.indices) {
                    if (auto ilit = index->as_int_literal()) {
                        os << "_" << ilit->value;
                    } else {
                        QL_USER_ERROR(
                            "dynamic indexation for variables is not "
                            "supported by cQASM"
                        );
                    }
                }

            }
        }

        // Terminate the typecast function.
        if (typecast) {
            os << ")";
        }

    }

    /**
     * Visitor function for `FunctionCall` nodes.
     */
    void visit_function_call(FunctionCall &node) override {
        auto prev_precedence = precedence;
        auto op_inf = OPERATOR_INFO.find({
            node.function_type->name,
            node.operands.size()
        });
        if (op_inf == OPERATOR_INFO.end()) {

            // Reset precedence for the function operands.
            precedence = 0;
            os << node.function_type->name << "(";
            auto first = true;
            for (auto &operand : node.operands) {
                if (!first) os << ", ";
                first = false;
                operand.visit(*this);
            }
            os << ")";

        } else {
            if (precedence > op_inf->second.precedence) {
                os << "(";
            }

            os << op_inf->second.prefix;
            if (node.operands.size() == 1) {

                // Print the only operand with this precedence level.
                // Associativity doesn't matter for unary operators because we
                // don't have postfix operators.
                precedence = op_inf->second.precedence;
                node.operands.front().visit(*this);

            } else if (node.operands.size() > 1) {

                // Print the first operand with this precedence level if
                // left-associative, or with one level higher precedence if
                // right-associative to force parentheses for equal precedence
                // in that case.
                precedence = op_inf->second.precedence;
                if (op_inf->second.associativity == OperatorAssociativity::RIGHT) {
                    precedence++;
                }
                node.operands.front().visit(*this);
                os << op_inf->second.infix;

                // If this is a ternary operator, print the middle operand.
                // Always place parentheses around it in case it's another
                // operator with the same precedence; I don't think this is
                // actually necessary, but more readable in my opinion.
                if (node.operands.size() > 2) {
                    QL_ASSERT(node.operands.size() <= 3);
                    precedence = op_inf->second.precedence + 1;
                    node.operands[1].visit(*this);
                    os << op_inf->second.infix2;
                }

                // Print the second operand with this precedence level if
                // right-associative, or with one level higher precedence if
                // left-associative to force parentheses for equal precedence
                // in that case.
                precedence = op_inf->second.precedence;
                if (op_inf->second.associativity == OperatorAssociativity::LEFT) {
                    precedence++;
                }
                node.operands.back().visit(*this);

            } else {
                QL_ASSERT(false);
            }
        }

        precedence = prev_precedence;
        if (precedence > op_inf->second.precedence) {
            os << ")";
        }
    }

};

/**
 * Writes a cQASM representation of the IR to the given stream with the given
 * line prefix.
 */
void write(
    const Ref &ir,
    const WriteOptions &options,
    std::ostream &os,
    const utils::Str &line_prefix
) {
    write(ir, ir, options, os, line_prefix);
}

/**
 * Writes the (partial) cQASM representation of the given node in the IR to the
 * given stream with the given line prefix.
 */
void write(
    const Ref &ir,
    const utils::One<ir::Node> &node,
    const WriteOptions &options,
    std::ostream &os,
    const utils::Str &line_prefix
) {
    Writer w{ir, options, os, line_prefix};
    node->visit(w);
}

/**
 * Shorthand for getting a cQASM string representation of the given node.
 */
utils::Str to_string(
    const Ref &ir,
    const utils::One<ir::Node> &node,
    const WriteOptions &options
) {
    utils::StrStrm ss;
    write(ir, node, options, ss);
    return ss.str();
}

} // namespace cqasm
} // namespace ir
} // namespace ql
