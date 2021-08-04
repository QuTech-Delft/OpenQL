/** \file
 * Defines the structures and functions used to construct the data dependency
 * graph for a block.
 */

#include "ql/com/ddg/build.h"

#include "ql/ir/ops.h"
#include "ql/ir/describe.h"
#include "ql/com/ddg/ops.h"

namespace ql {
namespace com {
namespace ddg {

/**
 * Constructs an object reference gatherer.
 */
EventGatherer::EventGatherer(const ir::Ref &ir) : ir(ir) {}

/**
 * Returns the contained dependency list.
 */
const Events &EventGatherer::get() const {
    return events;
}

/**
 * Adds a single reference. Literal access mode is upgraded to read
 * mode, as it makes no sense to access an object in literal mode (this
 * should never happen for consistent IRs though, unless this is explicitly
 * called this way). Measure access mode is upgraded to a write access to
 * both the qubit and the implicit bit associated with it. If there was
 * already an access for the object, the access mode is combined: if they
 * match the mode is maintained, otherwise the mode is changed to write.
 */
void EventGatherer::add_reference(
    ir::prim::OperandMode mode,
    const utils::One<ir::Reference> &reference
) {
    switch (mode) {
        case ir::prim::OperandMode::BARRIER:
        case ir::prim::OperandMode::WRITE:
        case ir::prim::OperandMode::UPDATE:
            mode = ir::prim::OperandMode::WRITE;
            break;
        case ir::prim::OperandMode::READ:
        case ir::prim::OperandMode::LITERAL:
            mode = ir::prim::OperandMode::READ;
            break;
        case ir::prim::OperandMode::COMMUTE_X:
        case ir::prim::OperandMode::COMMUTE_Y:
        case ir::prim::OperandMode::COMMUTE_Z:
            break;
        case ir::prim::OperandMode::MEASURE: {
            QL_ASSERT(reference->data_type->as_qubit_type());
            auto copy = reference->copy().as<ir::Reference>();
            copy->data_type = ir->platform->implicit_bit_type;
            add_reference(ir::prim::OperandMode::WRITE, copy);
            mode = ir::prim::OperandMode::WRITE;
            break;
        }
        case ir::prim::OperandMode::IGNORE:
            return;
    }
    AccessMode amode{mode};
    auto sref = Reference(reference);
    auto it = events.find(reference);
    if (it == events.end()) {
        events.insert(it, {sref, amode});
    } else {
        it->second = it->second.combine_with(amode);
    }
}

/**
 * Adds dependencies on whatever is used by a complete expression.
 */
void EventGatherer::add_expression(
    ir::prim::OperandMode mode,
    const ir::ExpressionRef &expr
) {
    if (expr->as_reference()) {
        add_reference(mode, expr.as<ir::Reference>());
    } else if (auto call = expr->as_function_call()) {
        add_operands(call->function_type->operand_types, call->operands);
    }
}

/**
 * Adds dependencies on the operands of a function or instruction.
 */
void EventGatherer::add_operands(
    const utils::Any<ir::OperandType> &prototype,
    const utils::Any<ir::Expression> &operands
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
                case ir::prim::OperandMode::COMMUTE_X:
                case ir::prim::OperandMode::COMMUTE_Y:
                case ir::prim::OperandMode::COMMUTE_Z:
                    mode = ir::prim::OperandMode::UPDATE;
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
void EventGatherer::add_statement(const ir::StatementRef &stmt) {
    auto barrier = false;
    if (auto cond = stmt->as_conditional_instruction()) {
        add_expression(ir::prim::OperandMode::READ, cond->condition);
        if (auto custom = stmt->as_custom_instruction()) {
            add_operands(
                ir::get_generalization(custom->instruction_type)->operand_types,
                ir::get_operands(stmt.as<ir::CustomInstruction>())
            );
            barrier = custom->instruction_type->barrier;
        } else if (auto set = stmt->as_set_instruction()) {
            add_expression(ir::prim::OperandMode::WRITE, set->lhs);
            add_expression(ir::prim::OperandMode::READ, set->rhs);
        } else if (stmt->as_goto_instruction()) {
            barrier = true;
        } else {
            QL_ASSERT(false);
        }
    } else if (auto wait = stmt->as_wait_instruction()) {
        if (wait->objects.empty()) {
            barrier = true;
        } else {
            for (const auto &ref : wait->objects) {
                add_expression(ir::prim::OperandMode::BARRIER, ref);
            }
        }
    } else if (auto if_else = stmt->as_if_else()) {
        for (const auto &branch : if_else->branches) {
            add_expression(ir::prim::OperandMode::READ, branch->condition);
            add_block(branch->body);
        }
        if (!if_else->otherwise.empty()) {
            add_block(if_else->otherwise);
        }
    } else if (auto loop = stmt->as_loop()) {
        add_block(loop->body);
        if (auto stat = stmt->as_static_loop()) {
            add_expression(ir::prim::OperandMode::WRITE, stat->lhs);
        } else if (auto dyn = stmt->as_dynamic_loop()) {
            add_expression(ir::prim::OperandMode::READ, dyn->condition);
            if (auto forl = stmt->as_for_loop()) {
#if 0   // original
                add_statement(forl->initialize);
                add_statement(forl->update);
#else   // FIXME: honour 'Maybe'-ness
                if(!forl->initialize.empty()) add_statement(forl->initialize);
                if(!forl->update.empty()) add_statement(forl->update);
#endif
            } else if (stmt->as_repeat_until_loop()) {
                // no further dependencies
            } else {
                QL_ASSERT(false);
            }
        } else {
            QL_ASSERT(false);
        }
    } else if (stmt->as_loop_control_statement()) {
        barrier = true;
    } else if (stmt->as_sentinel_statement()) {
        barrier = true;
    } else {
        QL_ASSERT(false);
    }

    // Generate data dependencies for barrier-like statements that operate
    // on everything, including stuff we don't know about.
    if (barrier) {
        add_reference(ir::prim::OperandMode::BARRIER, {});
    }

}

/**
 * Adds dependencies for a whole (sub)block of statements.
 */
void EventGatherer::add_block(const ir::SubBlockRef &block) {
    for (const auto &stmt : block->statements) {
        add_statement(stmt);
    }
}

/**
 * Clears the dependency list, allowing the object to be reused.
 */
void EventGatherer::reset() {
    events.clear();
}

/**
 * Data dependency graph builder class. Implements the build() function.
 */
class Builder {
private:

    /**
     * IR root node.
     */
    const ir::Ref &ir;

    /**
     * The block that we're building for.
     */
    const ir::BlockBaseRef &block;

    /**
     * The event gatherer object that we're using to get the events for the
     * statements in the block.
     */
    EventGatherer gatherer;

    /**
     * The source statement, serving as a sentinel that precedes all other
     * statements.
     */
    utils::One<ir::SentinelStatement> source;

    /**
     * The sink statement, serving as a sentinel that follows all other
     * statements.
     */
    utils::One<ir::SentinelStatement> sink;

    /**
     * Pair of an event and the corresponding node in the DDG that caused it.
     */
    struct EventNodePair {

        /**
         * The event (object access).
         */
        Event event;

        /**
         * Reference to the data dependency graph node whose corresponding
         * statement caused this event.
         */
        NodeRef node;

        /**
         * The statement that the node belongs to.
         */
        ir::StatementRef statement;

        /**
         * Returns whether this event commutes with the given event. Also
         * returns true when the events are caused by the same node.
         */
        utils::Bool commutes_with(const EventNodePair &enp) const {

            // Events belonging to the same statement commute with each other!
            // This is a necessary detail to make the DDG builder state machine
            // work.
            if (node == enp.node) {
                return true;
            }

            return event.commutes_with(enp.event);
        }

    };

    /**
     * List of events and corresponding DDG nodes.
     */
    using EventNodePairs = utils::List<EventNodePair>;

    /**
     * List of events/nodes that commute with each other. That is, all events
     * in this list commute with all other events in this list. Incoming events
     * will always be pushed into this set, evicting any entries that don't
     * commute with the incoming event to the non_commuting list. Whenever an
     * event is evicted from commuting to non_commuting, any entries previously
     * in non_commuting that operate on the same object or a subset thereof that
     * don't commute with the evicted event are pruned, to avoid redundant edges
     * in the DDG as much as possible.
     */
    EventNodePairs commuting;

    /**
     * List of events and associated DDG nodes in the past, that can't possibly
     * commute with any future events anymore. When a new event is pushed into
     * the commuting list, a data dependency must be added between all events
     * in this list that may (partially) operate on the same object, regardless
     * of whether the incoming event would commute with that event (because
     * something in commuting is already preventing this).
     */
    EventNodePairs non_commuting;

    /**
     * Accumulator for the order field of the DDG nodes.
     */
    utils::Int order_accumulator;

    /**
     * Adds a data dependency edge between the nodes of the given two event-node
     * pairs, using the duration of the "from" statement as weight.
     */
    void add_edge(const EventNodePair &from, const EventNodePair &to) {
        QL_ASSERT(from.node != to.node);

        // Create an edge, or fetch the existing edge if there already was one.
        auto result = from.node->successors.insert({to.statement, {}});
        auto &edge_ref = result.first->second;
        if (result.second) {

            // No edge existed yet, make one.
            QL_DOUT(
                "    add edge from " << ir::describe(from.statement) <<
                " to " << ir::describe(to.statement)
            );
            edge_ref.emplace();
            edge_ref->predecessor = from.statement;
            edge_ref->successor = to.statement;
            edge_ref->weight = 0;
            QL_ASSERT(to.node->predecessors.insert({from.statement, edge_ref}).second);

        }

        // Ensure that the edge weight is high enough.
        edge_ref->weight = utils::max<utils::Int>(
            edge_ref->weight,
            (utils::Int)ir::get_duration_of_statement(from.statement)
        );

        // Add a cause to the edge.
        Reference reference = from.event.reference.intersect_with(to.event.reference);
        DependencyType dependency_type{from.event.mode, to.event.mode};
        Cause cause{reference, dependency_type};
        QL_DOUT(
            "    add cause " << cause <<
            " to edge from " << ir::describe(from.statement) <<
            " to " << ir::describe(to.statement)
        );
        edge_ref->causes.push_back(cause);

    }

    /**
     * Evicts an event-node pair from the commuting list into the non_commuting
     * list, and prunes the non_commuting list accordingly. it must be an
     * iterator of the commuting list. It is advanced to the next event-node
     * pair after eviction.
     */
    void evict_from_commuting(EventNodePairs::Iter &it) {
        QL_DOUT("    evict: " << it->event << " for " << ir::describe(it->statement));

        // Remove any event-node pairs in non_commuting of which the event is
        // fully shadowed by the incoming event. The shadowing implies that the
        // events don't commute, and thus that there is already a DDG edge
        // between them. Because anything that would get an edge from *nc_it
        // would also get an edge to *it in this case, and because dependency
        // relations are transitive, we can safely forget about nc_it, and thus
        // optimize the graph and the generation thereof.
        auto nc_it = non_commuting.begin();
        while (nc_it != non_commuting.end()) {
            if (nc_it->event.is_shadowed_by(it->event)) {
                nc_it = non_commuting.erase(nc_it);
            } else {
                ++nc_it;
            }
        }

        // Move the event-node pair from commuting to non_commuting.
        non_commuting.push_back(*it);
        it = commuting.erase(it);

    }

    /**
     * Processes an incoming event by adding it to the commuting list, first
     * evicting anything from the list that doesn't commute with it.
     */
    void process_event(const EventNodePair &incoming) {
        QL_DOUT("  process event: " << incoming.event << " for " << ir::describe(incoming.statement));

        // Evict any event-node pairs that don't commute with the incoming pair
        // from the commuting list.
        auto it = commuting.begin();
        while (it != commuting.end()) {
            if (!it->commutes_with(incoming)) {
                evict_from_commuting(it);
            } else {
                ++it;
            }
        }

        // Add DDG edges from nodes in non_commuting that hit the same object as
        // incoming to the node corresponding to incoming. As a special case,
        // don't make edges to global state writes if we find any other node we
        // need an edge with, because said node necessarily will already have an
        // edge to this global state write.
        utils::Bool any_edge = false;
        utils::List<std::reference_wrapper<const EventNodePair>> global_write;
        for (const auto &nc : non_commuting) {
            if (nc.event.reference.is_global_state()) {
                global_write.push_back(nc);
                continue;
            }
            if (!nc.event.reference.is_provably_distinct_from(incoming.event.reference)) {
                add_edge(nc, incoming);
                any_edge = true;
            }
        }
        if (!any_edge) {
            for (const auto &nc : global_write) {
#if 0
                QL_ASSERT(!nc.get().commutes_with(incoming));
#else   // FIXME
                if(nc.get().commutes_with(incoming)) {
                    QL_ICE(
                        "DDG build: event '" + ir::describe(incoming.statement)
                        + "' commutes with '" + ir::describe(nc.get().statement) + "'"
                    );
                }
#endif
                add_edge(nc, incoming);
            }
        }

        // Add the incoming pair to the commuting list.
        commuting.push_back(incoming);

    }

    /**
     * Processes an incoming statement.
     */
    void process_statement(const ir::StatementRef &statement) {
        QL_DOUT("process statement: " << ir::describe(statement));
        QL_DOUT("  currently " << commuting.size() << " commuting entries");
        QL_DOUT("  currently " << non_commuting.size() << " non-commuting entries");

        // Make a node for the statement and add it.
        NodeRef node;
        node.emplace();
        node->order = order_accumulator++;
        statement->set_annotation<NodeRef>(node);

        // Gather the object access events for this statement.
        gatherer.reset();
        gatherer.add_statement(statement);

        // Process the events.
        for (const auto &event : gatherer.get()) {
            process_event({event, node, statement});
        }

    }

public:

    /**
     * Creates a new builder.
     */
    Builder(
        const ir::Ref &ir,
        const ir::BlockBaseRef &block,
        utils::Bool commute_multi_qubit,
        utils::Bool commute_single_qubit
    ) :
        ir(ir),
        block(block),
        gatherer(ir),
        order_accumulator(0)
    {
        gatherer.disable_multi_qubit_commutation = !commute_multi_qubit;
        gatherer.disable_single_qubit_commutation = !commute_single_qubit;
    }

    /**
     * Actually does the building.
     */
    void build() {

        // Remove any existing DDG annotations.
        clear(block);

        // Make the source and sink nodes and attach them to the block via the
        // Graph annotation.
        source.emplace();
        sink.emplace();
        block->set_annotation<Graph>({source, sink, 1});

        // Process the statements.
        process_statement(source);
        for (const auto &statement : block->statements) {
            process_statement(statement);
        }
        process_statement(sink);

    }

};

/**
 * Builds a forward data dependency graph for the given block.
 * commute_multi_qubit and commute_single_qubit allow the COMMUTE_* operand
 * access modes to be disabled for single- and/or multi-qubit gates.
 *
 * The nodes of the graph are represented by the statements in the block and
 * two sentinel statements, known as the source and the sink. The edges are
 * formed by dependencies from one instruction to another. Edges are weighted
 * such that the absolute value of the weight indicates the minimum number of
 * cycles that must be between the start cycle of the source and destination
 * node in the final schedule, and such that the sign indicates the direction
 */
void build(
    const ir::Ref &ir,
    const ir::BlockBaseRef &block,
    utils::Bool commute_multi_qubit,
    utils::Bool commute_single_qubit
) {
    Builder(ir, block, commute_multi_qubit, commute_single_qubit).build();
}

} // namespace ddg
} // namespace com
} // namespace ql
