/** \file
 * Defines basic access operations on the IR.
 */

#pragma once

#include "ql/utils/map.h"
#include "ql/ir/ir.h"

namespace ql {
namespace ir {

// Private template stuff.
namespace {

/**
 * Compares two named nodes by name.
 */
template <class T>
utils::Bool compare_by_name(const utils::One<T> &lhs, const utils::One<T> &rhs) {
    return lhs->name < rhs->name;
}

} // anonymous namespace

/**
 * Registers a data type.
 */
template <class T, typename... Args>
DataTypeLink add_type(const Ref &ir, Args... args) {

    // Construct a new data type object as requested.
    auto dtyp = utils::make<T>(std::forward<Args>(args)...).template as<DataType>();

    // Check its name. Note: some types may have additional parameters that are
    // not consistency-checked here.
    if (!std::regex_match(dtyp->name, IDENTIFIER_RE)) {
        throw utils::Exception(
            "invalid name for new data type: \"" + dtyp->name + "\" is not a valid identifier"
        );
    }

    // Insert it in the right position to maintain list order by name, while
    // doing a name uniqueness test at the same time.
    auto begin = ir->platform->data_types.get_vec().begin();
    auto end = ir->platform->data_types.get_vec().end();
    auto pos = std::lower_bound(begin, end, dtyp, compare_by_name<DataType>);
    if (pos != end && (*pos)->name == dtyp->name) {
        throw utils::Exception(
            "invalid name for new data type: \"" + dtyp->name + "\" is already in use"
        );
    }
    ir->platform->data_types.get_vec().insert(pos, dtyp);

    return dtyp;
}

/**
 * Returns the data type with the given name, or returns an empty link if the
 * type does not exist.
 */
DataTypeLink find_type(const Ref &ir, const utils::Str &name);

/**
 * Returns the data type of/returned by an expression.
 */
DataTypeLink get_type_of(const ExpressionRef &expr);

/**
 * Returns the maximum value that an integer of the given type may have.
 */
utils::Int get_max_int_for(const IntType &ityp);

/**
 * Returns the minimum value that an integer of the given type may have.
 */
utils::Int get_min_int_for(const IntType &ityp);

/**
 * Adds a physical object to the platform.
 */
ObjectLink add_physical_object(const Ref &ir, const utils::One<PhysicalObject> &obj);

/**
 * Returns the physical object with the given name, or returns an empty link if
 * the object does not exist.
 */
ObjectLink find_physical_object(const Ref &ir, const utils::Str &name);

/**
 * Adds an instruction type to the platform. The instruction_type object should
 * be fully generalized; template operands can be attached with the optional
 * additional argument (in which case the instruction specialization tree will
 * be generated appropriately).
 */
InstructionTypeLink add_instruction_type(
    const Ref &ir,
    const utils::One<InstructionType> &instruction_type,
    const utils::Any<Expression> &template_operands = {}
);

/**
 * Finds an instruction type based on its name, operand types, and writability
 * of each operand. If generate_overload_if_needed is set, and no instruction
 * with the given name and operand type set exists, then an overload is
 * generated for the first instruction type for which only the name matches, and
 * that overload is returned. If no matching instruction type is found or was
 * created, an empty link is returned.
 */
InstructionTypeLink find_instruction_type(
    const PlatformRef &platform,
    const utils::Str &name,
    const utils::Vec<DataTypeLink> &types,
    const utils::Vec<utils::Bool> &writable,
    utils::Bool generate_overload_if_needed = false
);

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
    const PlatformRef &platform,
    const utils::Str &name,
    const utils::Any<Expression> &operands,
    const ExpressionRef &condition = {},
    utils::Bool return_empty_on_failure = false,
    utils::Bool generate_overload_if_needed = false
);

/**
 * Shorthand for making a set instruction.
 */
InstructionRef make_set_instruction(
    const Ref &ir,
    const ExpressionRef &lhs,
    const ExpressionRef &rhs,
    const ExpressionRef &condition = {}
);

/**
 * Updates the given instruction node to use the most specialized instruction
 * type available. If the instruction is not a custom instruction or the
 * instruction is already fully specialized, this is no-op.
 */
void specialize_instruction(
    const InstructionRef &instruction
);

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
);

/**
 * Returns the most generalized variant of the given instruction type.
 */
InstructionTypeLink get_generalization(const InstructionTypeLink &spec);

/**
 * Returns the complete list of operands of an instruction. For custom
 * instructions this includes the template operands, and for set instructions
 * this returns the LHS and RHS as two operands. Other instruction types return
 * no operands. The condition (if any) is also not returned.
 */
Any<Expression> get_operands(const InstructionRef &instruction);

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
);

/**
 * Adds a function type to the platform.
 */
FunctionTypeLink add_function_type(
    const Ref &ir,
    const utils::One<FunctionType> &function_type
);

/**
 * Finds a function type based on its name and operand types. If no matching
 * function type is found, an empty link is returned.
 */
FunctionTypeLink find_function_type(
    const Ref &ir,
    const utils::Str &name,
    const utils::Vec<DataTypeLink> &types
);

/**
 * Builds a new function call node based on the given name and operand list.
 */
utils::One<FunctionCall> make_function_call(
    const Ref &ir,
    const utils::Str &name,
    const utils::Any<Expression> &operands
);

/**
 * Returns the number of qubits in the main qubit register.
 */
utils::UInt get_num_qubits(const PlatformRef &platform);

/**
 * Makes an integer literal using the given or default integer type.
 */
utils::One<IntLiteral> make_int_lit(const Ref &ir, utils::Int i, const DataTypeLink &type = {});

/**
 * Makes an integer literal using the given or default integer type.
 */
utils::One<IntLiteral> make_uint_lit(const PlatformRef &platform, utils::UInt i, const DataTypeLink &type = {});

/**
 * Makes an bit literal using the given or default bit type.
 */
utils::One<BitLiteral> make_bit_lit(const PlatformRef &platform, utils::Bool b, const DataTypeLink &type = {});

/**
 * Makes a qubit reference to the main qubit register.
 */
utils::One<Reference> make_qubit_ref(const PlatformRef &platform, utils::UInt idx);

/**
 * Makes a reference to the implicit measurement bit associated with a qubit in
 * the main qubit register.
 */
utils::One<Reference> make_bit_ref(const Ref &ir, utils::UInt idx);

/**
 * Makes a reference to the specified object using literal indices.
 */
utils::One<Reference> make_reference(
    const PlatformRef &platform,
    const ObjectLink &obj,
    utils::Vec<utils::UInt> indices = {}
);

/**
 * Makes a temporary object with the given type.
 */
ObjectLink make_temporary(
    const Ref &ir,
    const DataTypeLink &data_type,
    const utils::Vec<utils::UInt> &shape = {}
);

/**
 * Returns the duration of an instruction in quantum cycles. Note that this will
 * be zero for non-quantum instructions.
 */
utils::UInt get_duration_of_instruction(const InstructionRef &insn);

/**
 * Returns the duration of a statement in quantum cycles. Note that this will
 * be zero for non-quantum instructions. It will also be zero for structured
 * control-flow sub-blocks.
 */
utils::UInt get_duration_of_statement(const StatementRef &stmt);

/**
 * Returns the duration of a block in quantum cycles. If the block contains
 * structured control-flow sub-blocks, these are counted as zero cycles.
 */
utils::UInt get_duration_of_block(const BlockBaseRef &block);

/**
 * Returns whether an instruction is a quantum gate, by returning the number of
 * qubits in its operand list.
 */
utils::UInt get_number_of_qubits_involved(const InstructionRef &insn);

class OperandsHelper {
public:
    OperandsHelper(const PlatformRef p, const CustomInstruction &instruction) : platform(p), instr(instruction) {};

    utils::UInt getQubit(utils::UInt operandIndex) {
        const auto& op = getOperand(operandIndex);

        const auto& ref = op.as_reference();
        if (!ref) {
            QL_FATAL("Operand #" << operandIndex << " of instruction " << instr.instruction_type->name << " is not a reference.");
        }

        if (ref->target != platform->qubits) {
            QL_FATAL("Operand #" << operandIndex << " of instruction " << instr.instruction_type->name << " is not a qubit.");
        }

        return ref->indices[0].as<IntLiteral>()->value;
    }

    utils::UInt getFloat(utils::UInt operandIndex) {
        const auto& op = getOperand(operandIndex);

        const auto real = op.as_real_literal();

        if (!real) {
            QL_FATAL("Operand #" << operandIndex << " of instruction " << instr.instruction_type->name << " is not a float.");
        }

        return real->value;
    }

    utils::UInt getInt(utils::UInt operandIndex) {
        const auto& op = getOperand(operandIndex);

        const auto integer = op.as_int_literal();

        if (!integer) {
            QL_FATAL("Operand #" << operandIndex << " of instruction " << instr.instruction_type->name << " is not an integer.");
        }

        return integer->value;
    }

    utils::UInt numberOfQubitOperands() {
        auto& instr_type = *instr.instruction_type;
        while (!instr_type.generalization.empty()) {
            instr_type = *instr_type.generalization;
        }

        utils::UInt nQubitOperands = 0;
        for (const auto& op: instr_type.operand_types) {
            if (op->data_type->type() == NodeType::QubitType) {
                ++nQubitOperands;
            }
        }

        return nQubitOperands;
    }

    std::pair<utils::UInt, utils::UInt> get2QGateOperands() {
        QL_ASSERT(numberOfQubitOperands() == 2);

        utils::UInt q1 = utils::MAX;
        utils::UInt q2 = utils::MAX;

        for (utils::UInt i = 0; i < totalNumberOfOperands(); ++i) {
            const auto& op = getOperand(i);

            const auto ref = op.as_reference();
            if (ref && ref->target == platform->qubits) {
                if (q1 == utils::MAX) {
                    q1 = ref->indices[0].as<IntLiteral>()->value;
                } else if (q2 == utils::MAX) {
                    q2 = ref->indices[0].as<IntLiteral>()->value;
                    QL_ASSERT(q1 != q2);
                } else {
                    QL_FATAL("Gate has more than 2 qubit operands!");
                }
            }
        }
        return std::make_pair(q1, q2);
    }

    bool isNN2QGate(std::function<utils::UInt(utils::UInt)> v2r) {
        auto qubits = get2QGateOperands();

        return platform->topology->get_min_hops(v2r(qubits.first), v2r(qubits.second)) == 1;
    }

private:
    const utils::UInt totalNumberOfOperands() {
        return instr.instruction_type->template_operands.size() + instr.operands.size();
    }

    const Expression& getOperand(utils::UInt operandIndex) {
        const auto& templateOperands = instr.instruction_type->template_operands;
        const auto nTemplateOperands = templateOperands.size();

        if (operandIndex < nTemplateOperands) {
            return *templateOperands[operandIndex];
        }

        const auto nTotalOperands = nTemplateOperands + instr.operands.size();
        if (operandIndex >= nTotalOperands) {
            QL_FATAL("Tried to access operand #" << operandIndex << " of instruction " << instr.instruction_type->name << " which has only " << nTotalOperands << " operands.");
        }

        return *instr.operands[operandIndex - nTemplateOperands];
    }

    const PlatformRef platform;
    const CustomInstruction &instr;
};

} // namespace ir
} // namespace ql
