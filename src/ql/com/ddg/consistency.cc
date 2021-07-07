/** \file
 * Defines a consistency check for a DDG, useful when debugging.
 */

#include "ql/com/ddg/consistency.h"

#include "ql/com/ddg/ops.h"
#include "ql/ir/describe.h"

namespace ql {
namespace com {
namespace ddg {

/**
 * Adds all nodes forward-reachable from the given node to the map, using the
 * set of all preceding nodes as value.
 */
static void pathfind(
    const NodeCRef &node,
    utils::Set<NodeCRef> &nodes,
    utils::Set<NodeCRef> &path
) {

    // Try to insert the given node into the node list. If insertion is
    // successful, we hadn't visited this node before. So do that now.
    if (nodes.insert(node).second) {

        // If we've already seen this node in our path, we have a cycle!
        if (!path.insert(node).second) {
            QL_ICE("found cycle");
        }

        // Continue pathfinding.
        for (const auto &successor : node->successors) {
            pathfind(get_node(successor.first), nodes, path);
        }

        // Restore the path set to how we found it.
        path.erase(node);

    }

}

/**
 * Checks consistency of the data dependency graph associated with the given
 * block. Throws an ICE or assertion failure if an inconsistency was found.
 */
void check_consistency(const ir::BlockBaseRef &block) {
    try {

        // Check the graph annotation.
        auto graph = block->get_annotation_ptr<Graph>();
        if (!graph) QL_ICE("missing Graph annotation on block");
        if (graph->source.empty()) QL_ICE("missing source statement");
        if (!graph->source->has_annotation<NodeRef>()) QL_ICE("missing source node");
        if (graph->sink.empty()) QL_ICE("missing source statement");
        if (!graph->sink->has_annotation<NodeRef>()) QL_ICE("missing source node");
        if (graph->direction != 1 && graph->direction != -1) QL_ICE("invalid graph direction");

        // Sanity-check the source node.
        auto source = get_source_node(block);
        if (!source->predecessors.empty()) QL_ICE("source node has incoming edges");
        if (source->successors.empty()) QL_ICE("source node has no outgoing edges");
        if (source->order > 0) QL_ICE("source node does not have order <= 0");

        // Sanity-check the sink node.
        auto sink = get_sink_node(block);
        if (sink->predecessors.empty()) QL_ICE("sink node has no incoming edges");
        if (!sink->successors.empty()) QL_ICE("sink node has outgoing edges");
        if (sink->order < 0) QL_ICE("sink node does not have order >= 0");

        // Look for all nodes reachable from the source node. This also checks
        // for cycles.
        utils::Set<NodeCRef> reachable_nodes;
        utils::Set<NodeCRef> path;
        pathfind(source, reachable_nodes, path);

        // Make sure all statements in the block have nodes reachable from the
        // source, and that no two statements have the same node.
        utils::Set<NodeCRef> statement_nodes;
        QL_ASSERT(statement_nodes.insert(source).second);
        for (const auto &statement : block->statements) {
            const auto &node = get_node(statement);
            if (reachable_nodes.find(node) == reachable_nodes.end()) {
                QL_ICE(
                    "node for " << ir::describe(statement) << " statement "
                    "is not reachable from the source node"
                );
            }
            if (!statement_nodes.insert(node).second) {
                QL_ICE("node is used for more than one statement");
            }
        }
        if (reachable_nodes.find(sink) == reachable_nodes.end()) {
            QL_ICE("sink node is not reachable from the source node");
        }
        if (!statement_nodes.insert(sink).second) {
            QL_ICE("node is used for more than one statement");
        }

        // The number of nodes found from the statement list must match the
        // number of nodes reachable from the source node. Together with what
        // we've already checked, this ensures a one-to-one relationship between
        // the two.
        if (statement_nodes.size() != reachable_nodes.size()) {
            QL_ICE("node-statement relationship is not one-to-one");
        }

        // Make sure that all nodes that aren't the source or sink have at least
        // one incoming and outgoing edge.
        for (const auto &statement : block->statements) {
            auto node = get_node(statement);
            if (node->successors.empty()) {
                QL_ICE("non-sentinel statement node is missing successors");
            }
            if (node->predecessors.empty()) {
                QL_ICE("non-sentinel statement node is missing predecessors");
            }
        }

        // Find all edges, and ensure that no edge is reused.
        utils::Set<EdgeRef> edges;
        for (const auto &node : statement_nodes) {
            for (const auto &endpoint : node->successors) {
                if (!edges.insert(endpoint.second).second) {
                    QL_ICE("edge is used more than once");
                }
            }
        }

        // Check the endpoints of all the nodes.
        for (const auto &node : statement_nodes) {
            for (const auto &endpoint : node->successors) {
                if (get_node(endpoint.second->predecessor) != node) {
                    QL_ICE("outgoing edge of node does not have that node as predecessor");
                }
            }
            for (const auto &endpoint : node->predecessors) {
                if (get_node(endpoint.second->successor) != node) {
                    QL_ICE("incoming edge of node does not have that node as successor");
                }
                if (edges.find(endpoint.second) == edges.end()) {
                    QL_ICE("incoming edge was not found as outgoing edge of any node");
                }
            }
        }

        // Check the edge weights.
        for (const auto &edge : edges) {
            if (edge->weight != 0) {
                if (utils::sign_of(edge->weight) != utils::sign_of(graph->direction)) {
                    QL_ICE("sign of edge weight does not correspond to graph direction");
                }
            }
        }

    } catch (utils::Exception &e) {
        e.add_context("data dependency graph consistency check failed");
        throw;
    }
}

} // namespace ddg
} // namespace com
} // namespace ql
