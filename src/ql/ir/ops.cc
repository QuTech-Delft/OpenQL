/** \file
 * Defines basic access operations on the IR.
 */

#include "ql/ir/ops.h"

#include "ql/ir/describe.h"
#include "ql/ir/old_to_new.h"

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
    if (pos == end || (*pos)->name != name) {
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
        return ref->data_type;
    } else if (auto fnc = expr->as_function_call()) {
        return fnc->function_type->return_type;
    } else {
        QL_ICE("unknown expression node type encountered");
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
        QL_USER_ERROR(
            "invalid name for new register: \"" <<
            obj->name << "\" is not a valid identifier"
        );
    }

    // Insert it in the right position to maintain list order by name, while
    // doing a name uniqueness test at the same time.
    auto begin = ir->platform->objects.get_vec().begin();
    auto end = ir->platform->objects.get_vec().end();
    auto pos = std::lower_bound(begin, end, obj, compare_by_name<PhysicalObject>);
    if (pos != end && (*pos)->name == obj->name) {
        QL_USER_ERROR(
            "invalid name for new register: \"" <<
            obj->name << "\" is already in use"
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
    if (pos == end || (*pos)->name != name) {
        return {};
    } else {
        return *pos;
    }
}

/**
 * Adds an instruction type to the platform, or return the matching instruction
 * type specialization without changing anything in the IR if one already
 * existed. The boolean in the return value indicates what happened: if true, a
 * new instruction type was added. The incoming instruction_type object should
 * be fully generalized; template operands can be attached with the optional
 * additional argument (in which case the instruction specialization tree will
 * be generated appropriately).
 */
static utils::Pair<InstructionTypeLink, utils::Bool> add_or_find_instruction_type(
    const Ref &ir,
    const utils::One<InstructionType> &instruction_type,
    const utils::Any<Expression> &template_operands = {}
) {
    QL_ASSERT(instruction_type->specializations.empty());
    QL_ASSERT(instruction_type->template_operands.empty());
    QL_ASSERT(instruction_type->generalization.empty());

    // Check its name.
    if (!std::regex_match(instruction_type->name, IDENTIFIER_RE)) {
        QL_USER_ERROR(
            "invalid name for new instruction type: \"" <<
            instruction_type->name << "\" is not a valid identifier"
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

    // If the generalized instruction doesn't already exist, add it.
    auto added_anything = false;
    if (!already_exists) {
        auto clone = instruction_type.clone();
        clone->copy_annotations(*instruction_type);

        // The decompositions can't be cloned, because the links to
        // parameters and objects won't be updated properly (at least at the
        // time of writing clone() isn't smart enough for that). But we only
        // want them in the final, most specialized node anyway. So we add
        // the original from instruction_type at the end.
        clone->decompositions.reset();

        pos = ir->platform->instructions.get_vec().insert(pos, clone);
        added_anything = true;
    } else {

        // If it did already exist, copy the operand access modes from the
        // existing instruction. The assumption here is that the first time the
        // instruction is added is the "best" instruction in terms of
        // descriptiveness, so we need to copy anything that must be the same
        // across specializations to the incoming instruction type in case it's
        // added.
        for (utils::UInt i = 0; i < (*pos)->operand_types.size(); i++) {
            instruction_type->operand_types[i]->mode = (*pos)->operand_types[i]->mode;
        }

    }

    // Now create/add/look for specializations as appropriate.
    auto ityp = *pos;
    for (utils::UInt i = 0; i < template_operands.size(); i++) {
        auto op = template_operands[i];

        // See if the specialization already exists, and if so, recurse into
        // it.
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

        // The specialization doesn't exist yet, so we need to create it. We use
        // the generalization as a base except for the deepest specialization.
        utils::One<ir::InstructionType> spec;
        if (i == template_operands.size() - 1) {
            spec = instruction_type.clone();
            spec->copy_annotations(*instruction_type);
        } else {
            spec = ityp.clone();
            spec->copy_annotations(*ityp);
            spec->specializations.reset();
            spec->generalization.reset();
        }
        spec->decompositions.reset();

        // Move from operand types into template operands.
        for (utils::UInt j = 0; j <= i; j++) {
            QL_ASSERT(spec->operand_types[0]->data_type == get_type_of(template_operands[j]));
            spec->operand_types.remove(0);
            auto op_clone = template_operands[j].clone();
            op_clone->copy_annotations(*template_operands[j]);
            spec->template_operands.add(op_clone);
        }

        // Link the specialization up.
        ityp->specializations.add(spec);
        spec->generalization = ityp;
        added_anything = true;

        // Advance to next.
        ityp = spec;

    }

    // If we added an instruction type, make sure to add the decomposition rules
    // to the specialization.
    if (added_anything) {
        ityp->decompositions = instruction_type->decompositions;
    }

    return {ityp, added_anything};
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

    // Defer to add_or_find_instruction_type().
    auto result = add_or_find_instruction_type(ir, instruction_type, template_operands);

    // If we didn't add anything because a matching specialization of a matching
    // instruction already existed, either throw an error or return the existing
    // instruction
    if (!result.second) {
        QL_USER_ERROR(
            "duplicate instruction type: " << describe(instruction_type)
        );
    }

    return result.first;
}

/**
 * Finds an instruction type based on its name, operand types, and writability
 * of each operand. If generate_overload_if_needed is set, and no instruction
 * with the given name and operand type set exists, then an overload is
 * generated for the first instruction type for which only the name matches iff
 * that instruction type has the PrototypeInferred annotation, and that overload
 * is returned. If no matching instruction type is found or was created, an
 * empty link is returned.
 */
InstructionTypeLink find_instruction_type(
    const Ref &ir,
    const utils::Str &name,
    const utils::Vec<DataTypeLink> &types,
    const utils::Vec<utils::Bool> &writable,
    utils::Bool generate_overload_if_needed
) {
    QL_ASSERT(types.size() == writable.size());

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
            if (!writable[i]) {
                switch ((*pos)->operand_types[i]->mode) {
                    case prim::OperandMode::BARRIER:
                    case prim::OperandMode::WRITE:
                    case prim::OperandMode::UPDATE:
                    case prim::OperandMode::COMMUTE_X:
                    case prim::OperandMode::COMMUTE_Y:
                    case prim::OperandMode::COMMUTE_Z:
                    case prim::OperandMode::MEASURE:
                        match = false;
                        break;
                    case prim::OperandMode::READ:
                    case prim::OperandMode::LITERAL:
                    case prim::OperandMode::IGNORE:
                        break;
                }
                if (!match) break;
            }
        }
        if (match) {
            return *pos;
        }
    }

    // pos equalling first implies that (*pos)->name != name, i.e. there is no
    // instruction by this name.
    if (pos == first) {
        return {};
    }

    // If we shouldn't generate an overload if only the name matches, stop now.
    if (!generate_overload_if_needed || !(*first)->has_annotation<PrototypeInferred>()) {
        QL_DOUT("not generating overload for instruction '" + name + "'");  // NB: key '"prototype"' may be missing in instruction definition
        return {};
    }

    // Generate an overload for this instruction with the given set of
    // parameters, conservatively assuming write access mode for references and
    // read for everything else. This is based on the first instruction we
    // encounter with this name.
    auto ityp = first->clone();
    ityp->copy_annotations(**first);
    ityp->operand_types.reset();
    for (utils::UInt i = 0; i < types.size(); i++) {
        ityp->operand_types.emplace(
            writable[i] ? prim::OperandMode::UPDATE : prim::OperandMode::READ,
            types[i]
        );
    }

    // Insert the instruction just after all the other instructions with this
    // name, i.e. at pos, to maintain sort order.
    ir->platform->instructions.get_vec().insert(pos, ityp);

    return ityp;
}

/**
 * Builds a new instruction node based on the given name and operand list. Its
 * behavior depends on name.
 *
 *  - If "set", a set instruction is created. Exactly two operands must be
 *    specified, of which the first is the LHS and the second is the RHS. The
 *    LHS must be a reference, and have a classical data type. The RHS must have
 *    exactly the same data type as the LHS.
 *  - If "wait", a wait instruction is created. The first operand must be a
 *    non-negative integer literal, representing the duration. The remainder of
 *    the operands are what's waited on, and must be references. If there is
 *    only one operand, the instruction is a full barrier (i.e. it effectively
 *    waits on all objects).
 *  - If "barrier", a zero-duration wait instruction is created. The operands
 *    are what's waited on, and must be references. If there are no operands,
 *    the instruction is a full barrier (i.e. it effectively waits on all
 *    objects).
 *  - Any other name is treated as a custom instruction, resolved via
 *    find_instruction_type(). The most specialized instruction type is used.
 *
 * If no condition is specified, the instruction will be unconditional (a
 * literal true node is generated for it). For wait instructions, the specified
 * condition *must* be null, as wait instructions are always unconditional.
 *
 * Note that goto and dummy instructions cannot be created via this interface.
 *
 * return_empty_on_failure disables the exception that would otherwise be thrown
 * if no matching instruction type is found, instead returning {}.
 *
 * The generate_overload_if_needed flag is a hack for the conversion process
 * from the old to new IR. See find_instruction_type().
 */
InstructionRef make_instruction(
    const Ref &ir,
    const utils::Str &name,
    const utils::Any<Expression> &operands,
    const ExpressionRef &condition,
    utils::Bool return_empty_on_failure,
    utils::Bool generate_overload_if_needed
) {
    QL_IOUT("make_instruction: name=" + name + ", condition=" + (condition.empty() ? "<empty>" : ir::describe(condition)));   // FIXME
    InstructionRef insn;
    if (name == "set") {

        // Build a set instruction.
        if (operands.size() != 2) {
            QL_USER_ERROR(
                "set instructions must have exactly two operands"
            );
        }
        if (!operands[0]->as_reference()) {
            QL_USER_ERROR(
                "the left-hand side of a set instructions must be a reference"
            );
        }
        auto type = get_type_of(operands[0]);
        if (!type->as_classical_type()) {
            QL_USER_ERROR(
                "set instructions only support classical data types"
            );
        }
        if (type != get_type_of(operands[1])) {
            QL_USER_ERROR(
                "the left-hand side and right-hand side of a set "
                "instruction must have the same type"
            );
        }
        insn = utils::make<SetInstruction>(operands[0], operands[1]);

    } else if (name == "wait") {

        // Build a wait instruction.
        auto wait_insn = utils::make<WaitInstruction>();
        if (operands.empty()) {
            QL_USER_ERROR(
                "wait instructions must have at least one "
                "operand (the duration)"
            );
        }
        if (auto ilit = operands[0]->as_int_literal()) {
            if (ilit->value < 0) {
                QL_USER_ERROR(
                    "the duration of a wait instruction cannot be negative"
                );
            }
            wait_insn->duration = (utils::UInt)ilit->value;
        } else {
            QL_USER_ERROR(
                "the duration of a wait instruction must be an integer literal"
            );
        }
        for (utils::UInt i = 1; i < operands.size(); i++) {
            auto ref = operands[i].as<Reference>();
            if (ref.empty()) {
                QL_USER_ERROR(
                    "the operands of a wait instruction after the first "
                    "must be references"
                );
            }
            wait_insn->objects.add(ref);
        }
        insn = wait_insn;

    } else if (name == "barrier") {

        // Build a barrier instruction.
        auto barrier_insn = utils::make<WaitInstruction>();
        for (const auto &operand : operands) {
            auto ref = operand.as<Reference>();
            if (ref.empty()) {
                QL_USER_ERROR(
                    "the operands of a wait instruction after the first "
                    "must be references"
                );
            }
            barrier_insn->objects.add(ref);
        }
        insn = barrier_insn;

    } else {

        // Build a custom instruction.
        auto custom_insn = utils::make<CustomInstruction>();
        custom_insn->operands = operands;

        // Find the type for the custom instruction.
        utils::Vec<DataTypeLink> types;
        utils::Vec<utils::Bool> writable;
        for (const auto &operand : operands) {
            types.push_back(get_type_of(operand));
            writable.push_back(operand->as_reference() != nullptr);
        }
        custom_insn->instruction_type = find_instruction_type(
            ir,
            name,
            types,
            writable,
            generate_overload_if_needed
        );
        if (custom_insn->instruction_type.empty()) {
            if (return_empty_on_failure) {
                return {};
            }
            utils::StrStrm ss;
            ss << "unknown instruction: " << name;
            auto first = true;
            for (const auto &type : types) {
                if (first) {
                    first = false;
                } else {
                    ss << ",";
                }
                ss << " " << type->name;
            }
            QL_USER_ERROR(ss.str());
        }

        // Specialize the instruction type and operands as much as possible.
        specialize_instruction(custom_insn);

        insn = custom_insn;
    }

    // Set the condition, if applicable.
    if (auto cond_insn = insn->as_conditional_instruction()) {
        if (condition.empty()) {
            cond_insn->condition = make_bit_lit(ir, true);
        } else {
            cond_insn->condition = condition;
        }
    } else if (!condition.empty()) {
        QL_USER_ERROR(
            "condition specified for instruction that "
            "cannot be made conditional"
        );
    }

    // Return the constructed condition.
    return insn;
}

/**
 * Shorthand for making a set instruction.
 */
InstructionRef make_set_instruction(
    const Ref &ir,
    const ExpressionRef &lhs,
    const ExpressionRef &rhs,
    const ExpressionRef &condition
) {
    return make_instruction(ir, "set", {lhs, rhs}, condition);
}

/**
 * Updates the given instruction node to use the most specialized instruction
 * type available. If the instruction is not a custom instruction or the
 * instruction is already fully specialized, this is no-op.
 */
void specialize_instruction(
    const InstructionRef &instruction
) {
    if (auto custom_insn = instruction->as_custom_instruction()) {
        utils::Bool specialization_found;
        do {
            specialization_found = false;
            for (const auto &spec : custom_insn->instruction_type->specializations) {
                if (spec->template_operands.back().equals(custom_insn->operands.front())) {
                    custom_insn->operands.remove(0);
                    custom_insn->instruction_type = spec;
                    specialization_found = true;
                    break;
                }
            }
        } while (specialization_found);
    }
}

/**
 * Updates the given instruction node to use the most generalized instruction
 * type available. If the instruction is not a custom instruction or the
 * instruction is already fully generalized, this is no-op.
 *
 * This is useful in particular for changing instruction operands when mapping:
 * first generalize to get all the operands in the instruction node, then modify
 * the operands, and finally specialize the instruction again according to the
 * changed operands using specialize_instruction().
 */
void generalize_instruction(
    const InstructionRef &instruction
) {
    if (auto custom_insn = instruction->as_custom_instruction()) {
        while (!custom_insn->instruction_type->generalization.empty()) {
            custom_insn->operands.add(
                custom_insn->instruction_type->template_operands.back().clone(),
                0
            );
            custom_insn->instruction_type = custom_insn->instruction_type->generalization;
        }
    }
}

/**
 * Returns the most generalized variant of the given instruction type.
 */
InstructionTypeLink get_generalization(const InstructionTypeLink &spec) {
    InstructionTypeLink gen = spec;
    while (!gen->generalization.empty()) {
        gen = gen->generalization;
    }
    return gen;
}

/**
 * Returns the complete list of operands of an instruction. For custom
 * instructions this includes the template operands, and for set instructions
 * this returns the LHS and RHS as two operands. Other instruction types return
 * no operands. The condition (if any) is also not returned.
 */
Any<Expression> get_operands(const InstructionRef &instruction) {
    Any<Expression> operands;
    if (auto custom = instruction->as_custom_instruction()) {
        operands.extend(custom->instruction_type->template_operands);
        operands.extend(custom->operands);
    } else if (auto set = instruction->as_set_instruction()) {
        operands.add(set->lhs);
        operands.add(set->rhs);
    }
    return operands;
}

/**
 * Adds a decomposition rule. An instruction is generated for the decomposition
 * rule based on instruction_type and template_operands if one didn't already
 * exist. If one did already exist, only the decompositions field of
 * instruction_type is used to extend the decomposition rule list of the
 * existing instruction type.
 */
InstructionTypeLink add_decomposition_rule(
    const Ref &ir,
    const utils::One<InstructionType> &instruction_type,
    const utils::Any<Expression> &template_operands
) {

    // Defer to add_or_find_instruction_type().
    auto result = add_or_find_instruction_type(ir, instruction_type, template_operands);

    // If we didn't add anything because a matching specialization of a matching
    // instruction already existed, just add the incoming decomposition rules to
    // it.
    if (!result.second) {
        result.first->decompositions.extend(instruction_type->decompositions);
    }

    return result.first;
}

/**
 * Adds a function type to the platform.
 */
FunctionTypeLink add_function_type(
    const Ref &ir,
    const utils::One<FunctionType> &function_type
) {

    // Check its name.
    if (
        !std::regex_match(function_type->name, IDENTIFIER_RE) &&
        !utils::starts_with(function_type->name, "operator")
    ) {
        QL_USER_ERROR(
            "invalid name for new function type: \"" <<
            function_type->name << "\" is not a valid identifier or operator"
        );
    }

    // Search for an existing matching function.
    auto begin = ir->platform->functions.get_vec().begin();
    auto end = ir->platform->functions.get_vec().end();
    auto pos = std::lower_bound(begin, end, function_type, compare_by_name<FunctionType>);
    for (; pos != end && (*pos)->name == function_type->name; ++pos) {
        if ((*pos)->operand_types.size() != function_type->operand_types.size()) {
            continue;
        }
        auto match = true;
        for (utils::UInt i = 0; i < (*pos)->operand_types.size(); i++) {
            if ((*pos)->operand_types[i]->data_type != function_type->operand_types[i]->data_type) {
                match = false;
                break;
            }
        }
        if (match) {
            QL_USER_ERROR(
                "duplicate function type: " << describe(function_type)
            );
        }
    }

    // Add the function type in the right place.
    ir->platform->functions.get_vec().insert(pos, function_type);

    return function_type;
}

/**
 * Finds a function type based on its name and operand types. If no matching
 * function type is found, an empty link is returned.
 */
FunctionTypeLink find_function_type(
    const Ref &ir,
    const utils::Str &name,
    const utils::Vec<DataTypeLink> &types
) {
    auto begin = ir->platform->functions.get_vec().begin();
    auto end = ir->platform->functions.get_vec().end();
    auto pos = std::lower_bound(
        begin,
        end,
        utils::make<FunctionType>(name),
        compare_by_name<FunctionType>
    );
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
            return *pos;
        }
    }
    return {};
}

/**
 * Builds a new function call node based on the given name and operand list.
 */
utils::One<FunctionCall> make_function_call(
    const Ref &ir,
    const utils::Str &name,
    const utils::Any<Expression> &operands
) {

    // Build a function call node.
    auto function_call = utils::make<FunctionCall>();
    function_call->operands = operands;

    // Find the type for the custom function.
    utils::Vec<DataTypeLink> types;
    for (const auto &operand : operands) {
        types.push_back(get_type_of(operand));
    }
    function_call->function_type = find_function_type(ir, name, types);
    if (function_call->function_type.empty()) {
        utils::StrStrm ss;
        ss << "unknown function: " << name << "(";
        auto first = true;
        for (const auto &type : types) {
            if (first) {
                first = false;
            } else {
                ss << " ,";
            }
            ss << type->name;
        }
        ss << ")";
        QL_USER_ERROR(ss.str());
    }

    return function_call;
}

/**
 * Returns the number of qubits in the main qubit register.
 */
utils::UInt get_num_qubits(const Ref &ir) {
    QL_ASSERT(ir->platform->qubits->shape.size() == 1);
    return ir->platform->qubits->shape[0];
}

/**
 * Makes an integer literal using the given or default integer type.
 */
utils::One<IntLiteral> make_int_lit(
    const Ref &ir,
    utils::Int i,
    const DataTypeLink &type
) {
    auto typ = type;
    if (typ.empty()) {
        typ = ir->platform->default_int_type;
    }
    auto int_type = typ.as<IntType>();
    if (int_type.empty()) {
        QL_USER_ERROR(
            "type " + typ->name + " is not integer-like"
        );
    }
    if (i > get_max_int_for(*int_type) || i < get_min_int_for(*int_type)) {
        QL_USER_ERROR(
            "integer literal value out of range for default integer type"
        );
    }
    return utils::make<IntLiteral>(i, typ);
}

/**
 * Makes an integer literal using the given or default integer type.
 */
utils::One<IntLiteral> make_uint_lit(
    const Ref &ir,
    utils::UInt i,
    const DataTypeLink &type
) {
    auto typ = type;
    if (typ.empty()) {
        typ = ir->platform->default_int_type;
    }
    auto int_type = typ.as<IntType>();
    if (int_type.empty()) {
        QL_USER_ERROR(
            "type " << typ->name << " is not integer-like"
        );
    }
    if (i > (utils::UInt)get_max_int_for(*int_type)) {
        QL_USER_ERROR(
            "integer literal value out of range for default integer type"
        );
    }
    return utils::make<IntLiteral>((utils::UInt)i, typ);
}

/**
 * Makes an bit literal using the given or default bit type.
 */
utils::One<BitLiteral> make_bit_lit(
    const Ref &ir,
    utils::Bool b,
    const DataTypeLink &type
) {
    auto typ = type;
    if (typ.empty()) {
        typ = ir->platform->default_bit_type;
    }
    auto bit_type = typ.as<BitType>();
    if (bit_type.empty()) {
        QL_USER_ERROR(
            "type " + typ->name + " is not bit-like"
        );
    }
    return utils::make<BitLiteral>(b, typ);
}

/**
 * Makes a qubit reference to the main qubit register.
 */
utils::One<Reference> make_qubit_ref(const Ref &ir, utils::UInt idx) {
    return make_reference(ir, ir->platform->qubits, {idx});
}

/**
 * Makes a reference to the implicit measurement bit associated with a qubit in
 * the main qubit register.
 */
utils::One<Reference> make_bit_ref(const Ref &ir, utils::UInt idx) {
    if (ir->platform->implicit_bit_type.empty()) {
        QL_USER_ERROR(
            "platform does not support implicit measurement bits for qubits"
        );
    }
    auto ref = make_qubit_ref(ir, idx);
    ref->data_type = ir->platform->implicit_bit_type;
    return ref;
}

/**
 * Makes a reference to the specified object using literal indices.
 */
utils::One<Reference> make_reference(
    const Ref &ir,
    const ObjectLink &obj,
    utils::Vec<utils::UInt> indices
) {
    if (indices.size() > obj->shape.size()) {
        QL_USER_ERROR(
            "too many indices specified to make reference to '" + obj->name + "'"
        );
    } else if (indices.size() < obj->shape.size()) {
        QL_USER_ERROR(
            "not enough indices specified to make reference to '" + obj->name + "' "
            "(only individual elements can be referenced at this time)"
        );
    }
    auto ref = utils::make<Reference>(obj, obj->data_type);
    for (utils::UInt i = 0; i < indices.size(); i++) {
        if (indices[i] >= obj->shape[i]) {
            QL_USER_ERROR(
                "index out of range making reference to '" + obj->name + "'"
            );
        }
        ref->indices.add(make_uint_lit(ir, indices[i]));
    }
    return ref;
}

/**
 * Makes a temporary object with the given type.
 */
ObjectLink make_temporary(
    const Ref &ir,
    const DataTypeLink &data_type,
    const utils::Vec<utils::UInt> &shape
) {
    auto obj = utils::make<TemporaryObject>("", data_type, shape);
    ir->program->objects.add(obj);
    return std::move(obj);
}

/**
 * Returns the duration of an instruction in quantum cycles. Note that this will
 * be zero for non-quantum instructions.
 */
utils::UInt get_duration_of_instruction(const InstructionRef &insn) {
    if (auto custom = insn->as_custom_instruction()) {
        return custom->instruction_type->duration;
    } else if (auto wait = insn->as_wait_instruction()) {
        return wait->duration;
    } else {
        return 0;
    }
}

/**
 * Returns the duration of a statement in quantum cycles. Note that this will
 * be zero for non-quantum instructions. It will also be zero for structured
 * control-flow sub-blocks.
 */
utils::UInt get_duration_of_statement(const StatementRef &stmt) {
    auto insn = stmt.as<Instruction>();
    if (!insn.empty()) {
        return get_duration_of_instruction(insn);
    } else {
        return 0;
    }
}

/**
 * Returns the duration of a block in quantum cycles. If the block contains
 * structured control-flow sub-blocks, these are counted as zero cycles.
 */
utils::UInt get_duration_of_block(const BlockBaseRef &block) {

    // It is always necessary to iterate over the entire block, because the
    // first instruction might have a duration longer than the entire rest of
    // the block.
    utils::UInt duration = 0;
    for (const auto &stmt : block->statements) {
        duration = utils::max(
            duration,
            stmt->cycle + get_duration_of_statement(stmt)
        );
    }

    return duration;
}

/**
 * Returns whether an instruction is a quantum gate, by returning the number of
 * qubits in its operand list.
 */
utils::UInt get_number_of_qubits_involved(const InstructionRef &insn) {
    utils::UInt num_qubits = 0;
    if (auto custom = insn->as_custom_instruction()) {
        for (auto &otyp : get_generalization(custom->instruction_type)->operand_types) {
            if (otyp->data_type->as_qubit_type()) {
                num_qubits++;
            }
        }
    }
    return num_qubits;
}

} // namespace ir
} // namespace ql
