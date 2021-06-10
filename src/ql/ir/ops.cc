/** \file
 * Defines basic access operations on the IR.
 */

#include "ql/ir/ops.h"

namespace ql {
namespace ir {

/**
 * Returns the data type with the given name, or returns an empty link if the
 * type does not exist.
 */
DataTypeLink find_type(const Ref &ir, const utils::Str &name) {
    auto begin = ir->platform->data_types.get_vec().begin();
    auto end = ir->platform->data_types.get_vec().end();
    auto pos = std::lower_bound(
        begin, end,
        utils::make<QubitType>(name),
        compare_by_name<DataType>
    );
    if (pos == end) {
        return {};
    } else {
        return *pos;
    }
}

/**
 * Returns the data type of/returned by an expression.
 */
DataTypeLink get_type_of(const ExpressionRef &expr) {
    if (auto lit = expr->as_literal()) {
        return lit->data_type;
    } else if (auto ref = expr->as_reference()) {
        return ref->target->data_type;
    } else if (auto cast = expr->as_typecast()) {
        return cast->data_type;
    } else if (auto fnc = expr->as_function_call()) {
        return fnc->function_type->return_type;
    } else {
        throw utils::Exception("unknown expression node type encountered");
    }
}

/**
 * Returns the maximum value that an integer of the given type may have.
 */
utils::Int get_max_int_for(const IntType &ityp) {
    auto bits = ityp.bits;
    if (ityp.is_signed) bits--;
    return (utils::Int)((1ull << bits) - 1);
}

/**
 * Returns the minimum value that an integer of the given type may have.
 */
utils::Int get_min_int_for(const IntType &ityp) {
    if (!ityp.is_signed) return 0;
    return (utils::Int)(-(1ull << (ityp.bits - 1)));
}

/**
 * Adds a physical object to the platform.
 */
ObjectLink add_physical_object(const Ref &ir, const utils::One<PhysicalObject> &obj) {

    // Check its name.
    if (!std::regex_match(obj->name, IDENTIFIER_RE)) {
        throw utils::Exception(
            "invalid name for new register: \"" + obj->name + "\" is not a valid identifier"
        );
    }

    // Insert it in the right position to maintain list order by name, while
    // doing a name uniqueness test at the same time.
    auto begin = ir->platform->objects.get_vec().begin();
    auto end = ir->platform->objects.get_vec().end();
    auto pos = std::lower_bound(begin, end, obj, compare_by_name<PhysicalObject>);
    if (pos != end && (*pos)->name == obj->name) {
        throw utils::Exception(
            "invalid name for new register: \"" + obj->name + "\" is already in use"
        );
    }
    ir->platform->objects.get_vec().insert(pos, obj);

    return obj;
}

/**
 * Returns the physical object with the given name, or returns an empty link if
 * the object does not exist.
 */
ObjectLink find_physical_object(const Ref &ir, const utils::Str &name) {
    auto begin = ir->platform->objects.get_vec().begin();
    auto end = ir->platform->objects.get_vec().end();
    auto pos = std::lower_bound(
        begin, end,
        utils::make<PhysicalObject>(name),
        compare_by_name<PhysicalObject>
    );
    if (pos == end) {
        return {};
    } else {
        return *pos;
    }
}

/**
 * Adds an instruction type to the platform. The instruction_type object should
 * be fully generalized; template operands can be attached with the optional
 * additional argument (in which case the instruction specialization tree will
 * be generated appropriately).
 */
InstructionTypeLink add_instruction_type(
    const Ref &ir,
    const utils::One<InstructionType> &instruction_type,
    const utils::Any<Expression> &template_operands
) {
    if (!instruction_type->specializations.empty()) {
        instruction_type->dump();
    }
    QL_ASSERT(instruction_type->specializations.empty());
    QL_ASSERT(instruction_type->template_operands.empty());
    QL_ASSERT(instruction_type->generalization.empty());


    // Check its name.
    if (!std::regex_match(instruction_type->name, IDENTIFIER_RE)) {
        throw utils::Exception(
            "invalid name for new instruction type: \"" + instruction_type->name + "\" is not a valid identifier"
        );
    }

    // Search for an existing matching instruction.
    auto begin = ir->platform->instructions.get_vec().begin();
    auto end = ir->platform->instructions.get_vec().end();
    auto pos = std::lower_bound(begin, end, instruction_type, compare_by_name<InstructionType>);
    auto already_exists = false;
    for (; pos != end && (*pos)->name == instruction_type->name; ++pos) {
        if ((*pos)->operand_types.size() != instruction_type->operand_types.size()) {
            continue;
        }
        auto match = true;
        for (utils::UInt i = 0; i < (*pos)->operand_types.size(); i++) {
            if ((*pos)->operand_types[i]->data_type != instruction_type->operand_types[i]->data_type) {
                match = false;
                break;
            }
        }
        if (match) {
            already_exists = true;
            break;
        }
    }

    // If the instruction already exists and this is not a specialization of it,
    // that's an error.
    if (already_exists && template_operands.empty()) {
        // TODO: this exception should include the whole prototype, not just the
        //  name.
        throw utils::Exception(
            "duplicate instruction type: \"" + instruction_type->name + "\""
        );
    }

    // If the instruction doesn't already exist, add it.
    if (!already_exists) {
        pos = ir->platform->instructions.get_vec().insert(pos, instruction_type);
    }

    // Now create and add specializations as appropriate.
    auto added_anything = !already_exists;
    auto ityp = *pos;
    for (utils::UInt i = 0; i < template_operands.size(); i++) {
        auto op = template_operands[i];

        // See if the specialization already exists, and if so, recurse into it.
        auto already_exists = false;
        for (const auto &spec : ityp->specializations) {
            if (spec->template_operands.back().equals(op)) {
                already_exists = true;
                ityp = spec;
                break;
            }
        }
        if (already_exists) {
            continue;
        }

        // The specialization doesn't exist yet, so we need to create it.
        auto spec = instruction_type.clone();
        spec->copy_annotations(*instruction_type);

        // Move from operand types into template operands.
        for (utils::UInt j = 0; j <= i; j++) {
            QL_ASSERT(spec->operand_types[0]->data_type == get_type_of(template_operands[i]));
            spec->operand_types.remove(0);
            auto op_clone = template_operands[i].clone();
            op_clone->copy_annotations(*template_operands[i]);
            spec->template_operands.add(op_clone);
        }

        // Link the specialization up.
        ityp->specializations.add(spec);
        spec->generalization = ityp;
        added_anything = true;

        // Advance to next.
        ityp = spec;

    }
    if (!added_anything) {
        // TODO: this exception should include the whole prototype, not just the
        //  name.
        throw utils::Exception(
            "duplicate instruction type: \"" + instruction_type->name + "\""
        );
    }

    return ityp;
}

/**
 * Finds an instruction type based on its name and operand types. This returns
 * the most specialized instruction available. If generate_overload_if_needed is
 * set, and no instruction with the given name and operand type set exists, then
 * an overload is generated for the first instruction type for which only the
 * name matches, and that overload is returned. If no matching instruction type
 * is found or was created, an empty link is returned.
 */
InstructionTypeLink find_instruction_type(
    const Ref &ir,
    const utils::Str &name,
    const utils::Vec<DataTypeLink> &types,
    utils::Bool generate_overload_if_needed
) {

    // Search for a matching instruction.
    auto begin = ir->platform->instructions.get_vec().begin();
    auto end = ir->platform->instructions.get_vec().end();
    auto first = std::lower_bound(
        begin, end,
        utils::make<InstructionType>(name),
        compare_by_name<InstructionType>
    );
    auto pos = first;
    for (; pos != end && (*pos)->name == name; ++pos) {
        if ((*pos)->operand_types.size() != types.size()) {
            continue;
        }
        auto match = true;
        for (utils::UInt i = 0; i < (*pos)->operand_types.size(); i++) {
            if ((*pos)->operand_types[i]->data_type != types[i]) {
                match = false;
                break;
            }
        }
        if (match) {
            // TODO: find specialization!
            return *pos;
        }
    }

    // pos equalling first implies that (*pos)->name != name, i.e. there is no
    // instruction by this name.
    if (pos == first) {
        return {};
    }

    // If we shouldn't generate an overload if only the name matches, stop now.
    if (!generate_overload_if_needed) {
        return {};
    }

    // Generate an overload for this instruction with the given set of
    // parameters, conservatively assuming write access mode. This is based on
    // the first instruction we encountered with this name.
    auto ityp = first->clone();
    ityp->copy_annotations(**first);
    ityp->operand_types.reset();
    for (const auto &typ : types) {
        ityp->operand_types.emplace(prim::AccessMode::WRITE, typ);
    }

    // Insert the instruction just after all the other instructions with this
    // name, i.e. at pos, to maintain sort order.
    ir->platform->instructions.get_vec().insert(pos, ityp);

    return ityp;
}

/**
 * Returns whether the given expression can be assigned or is a qubit (i.e.,
 * whether it can appear on the left-hand side of an assignment, or can be used
 * as an operand in classical write or qubit access mode).
 */
utils::Bool is_assignable_or_qubit(const ExpressionRef &expr) {
    if (expr->as_literal()) {
        return false;
    } else if (expr->as_reference()) {
        return true;
    } else if (auto cast = expr->as_typecast()) {
        return is_assignable_or_qubit(cast->expression);
    } else if (expr->as_function_call()) {
        return false;
    } else {
        throw utils::Exception("unknown expression node type encountered");
    }
}

/**
 * Returns the duration of an instruction in quantum cycles. Note that this will
 * be zero for non-quantum instructions.
 */
utils::UInt get_duration_of(const InstructionRef &insn) {
    if (auto custom = insn->as_custom_instruction()) {
        return custom->instruction_type->duration;
    } else if (auto wait = insn->as_wait_instruction()) {
        return wait->duration;
    } else {
        return 0;
    }
}

/**
 * Returns whether an instruction is a quantum gate, by returning the number of
 * qubits in its operand list.
 */
utils::UInt is_quantum_gate(const InstructionRef &insn) {
    utils::UInt num_qubits = 0;
    if (auto custom = insn->as_custom_instruction()) {
        for (auto &otyp : custom->instruction_type->operand_types) {
            if (otyp->data_type->as_qubit_type()) {
                num_qubits++;
            }
        }
    }
    return num_qubits;
}

/**
 * Less-than operator to allow this to be used as a key to a map.
 */
utils::Bool ObjectAccesses::Reference::operator<(const Reference &rhs) const {
    if (target > rhs.target) return false;
    if (target < rhs.target) return true;
    if (implicit_bit > rhs.implicit_bit) return false;
    if (implicit_bit < rhs.implicit_bit) return true;
    utils::UInt i = 0;
    while (true) {
        if (i >= rhs.indices.size()) return false;
        if (i >= indices.size()) return true;
        if (indices[i] > rhs.indices[i]) return false;
        if (indices[i] < rhs.indices[i]) return true;
        i++;
    }
}

/**
 * Returns the contained dependency list.
 */
const ObjectAccesses::Accesses &ObjectAccesses::get() const {
    return accesses;
}

/**
 * Adds a single object access. Literal access mode is upgraded to read
 * mode, as it makes no sense to access an object in literal mode (this
 * should never happen for consistent IRs though, unless this is explicitly
 * called this way). Measure access mode is upgraded to a write access to
 * both the qubit and the implicit bit associated with it. If there was
 * already an access for the object, the access mode is combined: if they
 * match the mode is maintained, otherwise the mode is changed to write.
 */
void ObjectAccesses::add_access(prim::AccessMode mode, const Reference &reference) {
    if (mode == prim::AccessMode::LITERAL) {
        mode = prim::AccessMode::READ;
    } else if (mode == prim::AccessMode::MEASURE) {
        Reference copy = reference;
        copy.implicit_bit = !copy.implicit_bit;
        add_access(prim::AccessMode::WRITE, copy);
        mode = prim::AccessMode::WRITE;
    }
    auto it = accesses.find(reference);
    if (it == accesses.end()) {
        accesses.insert(it, {reference, mode});
    } else if (it->second != mode) {
        it->second = prim::AccessMode::WRITE;
    }
}

/**
 * Adds dependencies on whatever is used by a complete expression.
 */
void ObjectAccesses::add_expression(
    prim::AccessMode mode,
    const ExpressionRef &expr,
    utils::Bool implicit_bit
) {
    if (auto ref = expr->as_reference()) {
        Reference r;
        r.target = ref->target;
        r.implicit_bit = implicit_bit;
        for (const auto &idx : ref->indices) {
            if (auto ilit = idx->as_int_literal()) {
                r.indices.push_back(ilit->value);
            } else {
                break;
            }
        }
        add_access(mode, r);
    } else if (auto cast = expr->as_typecast()) {
        if (
            cast->data_type->as_bit_type() &&
            get_type_of(cast->expression)->as_qubit_type()
        ) {
            add_expression(mode, cast->expression, true);
        } else {
            add_expression(mode, cast->expression);
        }
    } else if (auto call = expr->as_function_call()) {
        add_operands(call->function_type->operand_types, call->operands);
    }
}

/**
 * Adds dependencies on the operands of a function or instruction.
 */
void ObjectAccesses::add_operands(
    const utils::Any<OperandType> &prototype,
    const utils::Any<Expression> &operands
) {
    utils::UInt num_qubits = 0;
    for (auto &otyp : prototype) {
        if (otyp->data_type->as_qubit_type()) {
            num_qubits++;
        }
    }
    auto disable_qubit_commutation = (
        (num_qubits == 1 && disable_single_qubit_commutation) ||
        (num_qubits > 1 && disable_multi_qubit_commutation)
    );
    for (utils::UInt i = 0; i < prototype.size(); i++) {
        auto mode = prototype[i]->mode;
        if (disable_qubit_commutation) {
            switch (mode) {
                case prim::AccessMode::COMMUTE_X:
                case prim::AccessMode::COMMUTE_Y:
                case prim::AccessMode::COMMUTE_Z:
                    mode = prim::AccessMode::WRITE;
                default:
                    break;
            }
        }
        add_expression(mode, operands[i]);
    }
}

/**
 * Adds dependencies for a complete statement.
 */
void ObjectAccesses::add_statement(const StatementRef &stmt) {
    if (auto cond = stmt->as_conditional_instruction()) {
        add_expression(prim::AccessMode::READ, cond->condition);
        if (auto custom = stmt->as_custom_instruction()) {
            add_operands(custom->instruction_type->operand_types, custom->operands);
            if (!custom->instruction_type->template_operands.empty()) {
                auto gen = custom->instruction_type;
                while (!gen->generalization.empty()) gen = gen->generalization;
                for (utils::UInt i = 0; i < custom->instruction_type->template_operands.size(); i++) {
                    add_expression(
                        gen->operand_types[i]->mode,
                        custom->instruction_type->template_operands[i]
                    );
                }
            }
        } else if (auto set = stmt->as_set_instruction()) {
            add_expression(prim::AccessMode::WRITE, set->lhs);
            add_expression(prim::AccessMode::READ, set->rhs);
        } else if (stmt->as_goto_instruction()) {
            // no dependencies
        } else {
            QL_ASSERT(false);
        }
    } else if (stmt->as_wait_instruction()) {
        // no dependencies
    } else if (stmt->as_dummy_instruction()) {
        // no dependencies
    } else if (auto if_else = stmt->as_if_else()) {
        for (const auto &branch : if_else->branches) {
            add_expression(prim::AccessMode::READ, branch->condition);
            add_block(branch->body);
        }
        if (!if_else->otherwise.empty()) {
            add_block(if_else->otherwise);
        }
    } else if (auto loop = stmt->as_loop()) {
        add_block(loop->body);
        if (auto stat = stmt->as_static_loop()) {
            add_expression(prim::AccessMode::WRITE, stat->lhs);
        } else if (auto dyn = stmt->as_dynamic_loop()) {
            add_expression(prim::AccessMode::READ, dyn->condition);
            if (auto forl = stmt->as_for_loop()) {
                add_statement(forl->initialize);
                add_statement(forl->update);
            } else if (stmt->as_repeat_until_loop()) {
                // no further dependencies
            } else {
                QL_ASSERT(false);
            }
        } else {
            QL_ASSERT(false);
        }
    } else if (stmt->as_loop_control_statement()) {
        // no dependencies
    } else {
        QL_ASSERT(false);
    }
}

/**
 * Adds dependencies for a whole (sub)block of statements.
 */
void ObjectAccesses::add_block(const SubBlockRef &block) {
    for (const auto &stmt : block->statements) {
        add_statement(stmt);
    }
}

/**
 * Clears the dependency list, allowing the object to be reused.
 */
void ObjectAccesses::reset() {
    accesses.clear();
}

/**
 * Constructs a remapper.
 */
ReferenceRemapper::ReferenceRemapper(Map &&map) : map(std::move(map)) {
}

/**
 * Constructs a remapper.
 */
ReferenceRemapper::ReferenceRemapper(const Map &map) : map(map) {
}

/**
 * The visit function that actually implements the remapping.
 */
void ReferenceRemapper::visit_reference(Reference &node) {
    auto it = map.find(node.target);
    if (it != map.end()) {
        node.target = it->second;
    }
}

} // namespace ir
} // namespace ql
