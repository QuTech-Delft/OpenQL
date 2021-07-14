/** \file
 * Defines a consistency check for a CFG, useful when debugging.
 */

#include "ql/com/cfg/consistency.h"

#include "ql/com/cfg/ops.h"
#include "ql/ir/describe.h"

namespace ql {
namespace com {
namespace cfg {

/**
 * Checks consistency of the control-flow graph associated with the given
 * program. Throws an ICE or assertion failure if an inconsistency was found.
 */
void check_consistency(const ir::ProgramRef &program) {
    try {

        // Check the graph annotation.
        auto graph = program->get_annotation_ptr<Graph>();
        if (!graph) QL_ICE("missing Graph annotation on program");
        if (graph->source.empty()) QL_ICE("missing source block");
        if (!graph->source->has_annotation<NodeRef>()) QL_ICE("missing source node");
        if (graph->sink.empty()) QL_ICE("missing source block");
        if (!graph->sink->has_annotation<NodeRef>()) QL_ICE("missing source node");

        // Sanity-check the source node.
        auto source = get_source_node(program);
        if (!source->predecessors.empty()) QL_ICE("source node has incoming edges");
        if (source->successors.empty()) QL_ICE("source node has no outgoing edges");

        // Sanity-check the sink node. Note that the sink is not necessarily
        // reachable (it's fine if the program never terminates).
        auto sink = get_sink_node(program);
        if (!sink->successors.empty()) QL_ICE("sink node has outgoing edges");

        // Make sure all blocks in the program have nodes, that no two blocks
        // have the same node, and that all blocks have at least one successor.
        utils::Set<NodeCRef> block_nodes;
        QL_ASSERT(block_nodes.insert(source).second);
        for (const auto &block : program->blocks) {
            const auto &node = get_node(block);
            if (!node.has_value()) {
                QL_ICE(ir::describe(block) << " is missing a node");
            }
            if (!block_nodes.insert(node).second) {
                QL_ICE("node is used for more than one block");
            }
            if (node->successors.empty()) {
                QL_ICE(ir::describe(block) << " is missing successors");
            }
        }
        if (!block_nodes.insert(sink).second) {
            QL_ICE("node is used for more than one block");
        }

        // Find all edges, and ensure that no edge is reused.
        utils::Set<EdgeRef> edges;
        for (const auto &node : block_nodes) {
            for (const auto &endpoint : node->successors) {
                if (!edges.insert(endpoint.second).second) {
                    QL_ICE("edge is used more than once");
                }
            }
        }

        // Check the endpoints of all the nodes.
        for (const auto &node : block_nodes) {
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

    } catch (utils::Exception &e) {
        e.add_context("data dependency graph consistency check failed");
        throw;
    }
}

} // namespace cfg
} // namespace com
} // namespace ql
