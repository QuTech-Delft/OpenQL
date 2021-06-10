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
    utils::Bool generate_overload_if_needed = false
);

/**
 * Returns whether the given expression can be assigned or is a qubit (i.e.,
 * whether it can appear on the left-hand side of an assignment, or can be used
 * as an operand in classical write or qubit access mode).
 */
utils::Bool is_assignable_or_qubit(const ExpressionRef &expr);

/**
 * Returns the duration of an instruction in quantum cycles. Note that this will
 * be zero for non-quantum instructions.
 */
utils::UInt get_duration_of(const InstructionRef &insn);

/**
 * Returns whether an instruction is a quantum gate, by returning the number of
 * qubits in its operand list.
 */
utils::UInt is_quantum_gate(const InstructionRef &insn);

/**
 * Container for gathering and representing the list of object accesses for
 * instructions and expressions.
 */
class ObjectAccesses {
public:

    /**
     * A reference to an object, including index.
     */
    class Reference {
    public:

        /**
         * The object being used.
         */
        ObjectLink target;

        /**
         * Whether the object itself or its implicit bit register is used (this
         * can only be true for qubit registers).
         */
        utils::Bool implicit_bit = false;

        /**
         * The indices being accessed, for as far as they are statically known.
         */
        utils::Vec<utils::UInt> indices;

        /**
         * Less-than operator to allow this to be used as a key to a map.
         */
        utils::Bool operator<(const Reference &rhs) const;

    };

    using Access = utils::Pair<Reference, prim::AccessMode>;

    /**
     * Shorthand for the data dependency list container.
     */
    using Accesses = utils::Map<Reference, prim::AccessMode>;

private:

    /**
     * The actual dependency list.
     */
    Accesses accesses;

public:

    /**
     * Configuration tweak that disables X/Y/Z commutation for single-qubit
     * gates (i.e., instructions with a single-qubit operand). Modifying this
     * only affects the behavior of subsequent add_*() calls; it doesn't affect
     * previously added dependencies.
     */
    utils::Bool disable_single_qubit_commutation = false;

    /**
     * Configuration tweak that disables X/Y/Z commutation for multi-qubit
     * gates (i.e., an instruction with a multi-qubit operand). Modifying this
     * only affects the behavior of subsequent add_*() calls; it doesn't affect
     * previously added dependencies.
     */
    utils::Bool disable_multi_qubit_commutation = false;

    /**
     * Returns the contained list of object accesses.
     */
    const Accesses &get() const;

    /**
     * Adds a single object access. Literal access mode is upgraded to read
     * mode, as it makes no sense to access an object in literal mode (this
     * should never happen for consistent IRs though, unless this is explicitly
     * called this way). Measure access mode is upgraded to a write access to
     * both the qubit and the implicit bit associated with it. If there was
     * already an access for the object, the access mode is combined: if they
     * match the mode is maintained, otherwise the mode is changed to write.
     */
    void add_access(prim::AccessMode mode, const Reference &reference);

    /**
     * Adds dependencies on whatever is used by a complete expression.
     */
    void add_expression(
        prim::AccessMode mode,
        const ExpressionRef &expr,
        utils::Bool implicit_bit = false
    );

    /**
     * Adds dependencies on the operands of a function or instruction.
     */
    void add_operands(
        const utils::Any<OperandType> &prototype,
        const utils::Any<Expression> &operands
    );

    /**
     * Adds dependencies for a complete statement.
     */
    void add_statement(const StatementRef &stmt);

    /**
     * Adds dependencies for a whole (sub)block of statements.
     */
    void add_block(const SubBlockRef &block);

    /**
     * Clears the dependency list, allowing the object to be reused.
     */
    void reset();

};

/**
 * Visitor that rewrites object references to implement (re)mapping.
 *
 * FIXME: this fundamentally can't handle remapping elements of non-scalar
 *  stuff. So it's probably not good enough.
 */
class ReferenceRemapper : RecursiveVisitor {
public:

    /**
     * Shorthand for the object link map type.
     */
    using Map = utils::Map<ObjectLink, ObjectLink>;

    /**
     * The object link map.
     */
    Map map;

    /**
     * Constructs a remapper.
     */
    explicit ReferenceRemapper(Map &&map = {});

    /**
     * Constructs a remapper.
     */
    explicit ReferenceRemapper(const Map &map);

    /**
     * The visit function that actually implements the remapping.
     */
    void visit_reference(Reference &node) override;

};

} // namespace ir
} // namespace ql
