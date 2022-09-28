/** \file
 * Defines the structures and functions used to construct the data dependency
 * graph for a block.
 */

#pragma once

#include "ql/com/ddg/types.h"

namespace ql {
namespace com {
namespace ddg {

/**
 * Container for gathering and representing the list of object accesses for
 * instructions and expressions.
 */
class EventGatherer {
private:

    /**
     * Reference to the root of the IR.
     */
    ir::Ref ir;

    /**
     * The actual event list.
     */
    Events events;

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
     * Constructs an object reference gatherer.
     */
    explicit EventGatherer(const ir::Ref &ir);

    /**
     * Returns the contained list of object accesses.
     */
    const Events &get() const;

    /**
     * Adds a single reference. Literal access mode is upgraded to read
     * mode, as it makes no sense to access an object in literal mode (this
     * should never happen for consistent IRs though, unless this is explicitly
     * called this way). Measure access mode is upgraded to a write access to
     * both the qubit and the implicit bit associated with it. If there was
     * already an access for the object, the access mode is combined: if they
     * match the mode is maintained, otherwise the mode is changed to write.
     */
    void add_reference(
        ir::prim::OperandMode mode,
        const utils::One<ir::Reference> &reference
    );

    /**
     * Adds dependencies on whatever is used by a complete expression.
     */
    void add_expression(
        ir::prim::OperandMode mode,
        const ir::ExpressionRef &expr
    );

    /**
     * Adds dependencies on the operands of a function or instruction.
     */
    void add_operands(
        const utils::Any<ir::OperandType> &prototype,
        const utils::Any<ir::Expression> &operands
    );

    /**
     * Adds dependencies for a complete statement.
     */
    void add_statement(const ir::StatementRef &stmt);

    /**
     * Adds dependencies for a whole (sub)block of statements.
     */
    void add_block(const ir::SubBlockRef &block);

    /**
     * Clears the dependency list, allowing the object to be reused.
     */
    void reset();

};

/**
 * Builds a forward data dependency graph for the given block.
 * commute_multi_qubit and commute_single_qubit allow the COMMUTE_* operand
 * access modes to be disabled for single- and/or multi-qubit gates.
 *
 * The nodes of the graph are represented by the statements in the block and
 * two sentinel statements, known as the source and the sink. The edges are
 * formed by dependencies from one statements to another. Edges are weighted
 * such that the absolute value of the weight indicates the minimum number of
 * cycles that must be between the start cycle of the source and destination
 * node in the final schedule, and such that the sign indicates the direction
 */
void build(
    const ir::Ref &ir,
    const ir::BlockBaseRef &block,
    utils::Bool commute_multi_qubit = true,
    utils::Bool commute_single_qubit = true
);

} // namespace ddg
} // namespace com
} // namespace ql
