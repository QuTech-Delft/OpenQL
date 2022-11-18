/** \file
 * Defines functions for operating on an existing data dependency graph.
 */

#include "ql/com/ddg/ops.h"

namespace ql {
namespace com {
namespace ddg {

/**
 * Returns the DDG node associated with the given statement, if any.
 */
NodeCRef get_node(const ir::StatementRef &statement) {
    if (statement.empty()) {
        return {};
    }
    if (auto node = statement->get_annotation_ptr<NodeRef>()) {
        return node->as_const();
    } else {
        return {};
    }
}

/**
 * Returns the source statement associated with the given block, if any.
 */
utils::One<ir::SentinelStatement> get_source(const ir::BlockBaseRef &block) {
    if (auto data = block->get_annotation_ptr<Graph>()) {
        return data->source;
    } else {
        return {};
    }
}

/**
 * Shorthand for getting the source node.
 */
NodeCRef get_source_node(const ir::BlockBaseRef &block) {
    return get_node(get_source(block));
}

/**
 * Returns the sink statement associated with the given block, if any.
 */
utils::One<ir::SentinelStatement> get_sink(const ir::BlockBaseRef &block) {
    if (auto data = block->get_annotation_ptr<Graph>()) {
        return data->sink;
    } else {
        return {};
    }
}

/**
 * Shorthand for getting the sink node.
 */
NodeCRef get_sink_node(const ir::BlockBaseRef &block) {
    return get_node(get_sink(block));
}

/**
 * Returns the DDG edge between the two given statements, or returns an empty
 * edge reference if there is no edge between the statements. Note that this is
 * directional.
 */
EdgeCRef get_edge(const ir::StatementRef &from, const ir::StatementRef &to) {
    const auto &successors = get_node(from)->successors;
    auto it = std::find_if(successors.begin(), successors.end(),
        [&to](const std::pair<ir::StatementRef, EdgeRef> &x) { return x.first == to; });
    if (it == successors.end()) {
        return {};
    } else {
        return it->second.as_const();
    }
}

/**
 * Returns the effective scheduling direction when scheduling using this DDG.
 */
utils::Int get_direction(const ir::BlockBaseRef &block) {
    if (auto data = block->get_annotation_ptr<Graph>()) {
        return data->direction;
    } else {
        return 0;
    }
}

/**
 * Removes the data dependency graph annotations from the given block.
 */
void clear(const ir::BlockBaseRef &block) {
    block->erase_annotation<Graph>();
    for (const auto &statement : block->statements) {
        statement->erase_annotation<NodeRef>();
    }
}

/**
 * Helper function for reverse() that reverses the node and successor edges of
 * the given statement.
 */
static void reverse_statement(const ir::StatementRef &statement) {
    auto node = statement->get_annotation<NodeRef>();
    std::swap(node->successors, node->predecessors);
    node->order = -node->order;
    for (const auto &it : node->successors) {
        const auto &edge = it.second;
        std::swap(edge->successor, edge->predecessor);
        edge->weight = -edge->weight;
    }
}

/**
 * Reverses the direction of the data dependency graph associated with the given
 * block. This does the following things:
 *
 *  - swap source and sink;
 *  - swap successors and predecessors;
 *  - negate instruction order (for tie-breaking scheduling heuristics);
 *  - negate the weight of the edges; and
 *  - reverse the effective scheduling direction.
 *
 * A reversed DDG effectively turns an ASAP scheduler into ALAP and vice versa,
 * because the weights are then non-positive so cycles decrease, and the
 * dependencies are reversed.
 */
void reverse(const ir::BlockBaseRef &block) {
    auto &graph = block->get_annotation<Graph>();
    std::swap(graph.source, graph.sink);
    graph.direction = -graph.direction;
    reverse_statement(graph.source);
    for (const auto &statement : block->statements) {
        reverse_statement(statement);
    }
    reverse_statement(graph.sink);
}

void add_remaining(const ir::BlockBaseRef &block) {
    get_sink(block)->set_annotation<Remaining>({ 0 });

    std::set<ir::StatementRef> toVisit;
    toVisit.insert(get_sink(block));

    while(!toVisit.empty()) {
        auto current = *toVisit.begin();
        toVisit.erase(toVisit.begin());

        auto currentRemaining = current->get_annotation<Remaining>().remaining;

        for (const auto &node_edge: get_node(current)->predecessors) {
            QL_ASSERT(node_edge.second->weight >= 0 && "Cannot compute remaining on reversed DDG");
            auto remaining = (utils::UInt) node_edge.second->weight + currentRemaining;

            if (node_edge.first->has_annotation<Remaining>()) {
                auto& annot = node_edge.first->get_annotation<Remaining>().remaining;
                annot = std::max(remaining, annot);
            } else {
                node_edge.first->set_annotation<Remaining>({ remaining });
            }

            toVisit.insert(node_edge.first);
        }
    }
}

} // namespace ddg
} // namespace com
} // namespace ql
