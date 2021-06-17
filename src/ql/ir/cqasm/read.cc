/** \file
 * cQASM 1.2 reader logic as human-readable complement of the IR.
 */

#include "ql/ir/cqasm/read.h"

#include "ql/ir/compat/program.h"
#include "ql/ir/ops.h"
#include "ql/ir/consistency.h"
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
        case prim::AccessMode::READ:
        case prim::AccessMode::LITERAL:
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
                    throw utils::UserError(
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
        throw utils::UserError(
            "@ql.name must have a single string argument"
        );
    }
    if (auto str = annot->operands[0]->as_const_string()) {
        if (identifier && !std::regex_match(str->value, IDENTIFIER_RE)) {
            throw utils::UserError(
                "name specified via @ql.name must be an identifier"
            );
        }
        annot->set_annotation<Used>({});
        return str->value;
    } else {
        throw utils::UserError(
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
        throw utils::UserError(
            "@ql.type must have a single string argument"
        );
    }
    if (auto str = annot->operands[0]->as_const_string()) {
        auto typ = find_type(ir, str->value);
        if (typ.empty()) {
            throw utils::UserError(
                "type specified via @ql.type does not exist in platform"
            );
        }
        annot->set_annotation<Used>({});
        return typ;
    } else {
        throw utils::UserError(
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
    } else if (cq_type->as_real()) {
        for (const auto &ql_type : ir->platform->data_types) {
            if (ql_type->as_real_type()) {
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
    throw utils::UserError(
        "failed to infer OpenQL type for " + utils::to_string(cq_type) + "; "
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
 * Converts the contents of a cQASM block to an OpenQL block.
 */
static void convert_block(
    const Ref &ir,
    const cqt::One<cqs::Block> &cq_block,
    const utils::One<BlockBase> &ql_block
) {

}

/**
 * Reads a cQASM 1.2 file into the IR. If reading is successful, ir->program is
 * completely replaced. data represents the cQASM file contents, fname specifies
 * the filename if one exists for the purpose of generating better error
 * messages.
 */
void read(const Ref &ir, const utils::Str &data, const utils::Str &fname) {

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
                throw utils::Exception("main qubit register must be one-dimensional");
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

    // Add builtin functions.
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
        throw utils::UserError(errors.str());
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
                    throw utils::UserError("@ql.temp does not take any arguments");
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
                throw utils::UserError(
                    "subcircuit marked @ql.entry must consist of exactly "
                    "one unconditional goto instruction"
                );
            }
            cq_entry = x.first->target;
            if (cq_entry.links_to(cq_program->subcircuits[0])) {
                throw utils::UserError(
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
                throw utils::UserError(
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
                    cq_subc->body->statements[0].as<cqs::BundleExt>()->items.remove(-1);

                }

                // Convert the rest of the block.
                convert_block(ir, cq_subc->body, ql_block);

                // Make sure no unused @ql.* annotations remain.
                check_all_annotations_used(cq_program->subcircuits.back());

            }

        }

    }

    // Looks like conversion was successful.
    ir->program = ql_program;
    ir->dump_seq();
    check_consistency(ir);

}

} // namespace cqasm
} // namespace ir
} // namespace ql
