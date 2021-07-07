/** \file
 * Defines a consistency check for a DDG, useful when debugging.
 */

#include "ql/com/ddg/ops.h"

#include "ql/ir/describe.h"

namespace ql {
namespace com {
namespace ddg {

/**
 * Adds the given statement and its outgoing edges to the statements and edges
 * maps.
 */
static void add_node(
    const ir::StatementRef &statement,
    utils::Int order_offset,
    utils::Map<utils::UInt, ir::StatementRef> &statements,
    utils::Map<ir::StatementRef, utils::UInt> &statement_indices,
    utils::Map<utils::UInt, EdgeCRef> &edges
) {
    auto node = get_node(statement);
    QL_ASSERT(statements.insert({node->order + order_offset, statement}).second);
    QL_ASSERT(statement_indices.insert({statement, node->order + order_offset}).second);
    for (const auto &endpoint : node->successors) {
        QL_ASSERT(edges.insert({edges.size(), endpoint.second.as_const()}).second);
    }
}

/**
 * Dumps a dot file and accompanying key.
 */
void write_dot(
    const ir::BlockBaseRef &block,
    std::ostream &dot,
    std::ostream &key
) {

    // Construct maps of unique numbers to nodes and edges.
    utils::Map<utils::UInt, ir::StatementRef> statements;
    utils::Map<ir::StatementRef, utils::UInt> statement_indices;
    utils::Map<utils::UInt, EdgeCRef> edges;
    utils::Int order_offset = -get_source_node(block)->order;
    add_node(get_source(block), order_offset, statements, statement_indices, edges);
    for (const auto &statement : block->statements) {
        add_node(statement, order_offset, statements, statement_indices, edges);
    }
    add_node(get_sink(block), order_offset, statements, statement_indices, edges);

    // Write the dot graph.
    dot << "digraph ddg {\n\n";
    for (const auto &it : statements) {
        dot << "  n" << it.first;
        dot << " [ label=\"n" << it.first << ": ";
        dot << utils::replace_all(ir::describe(it.second), "\"", "'");
        dot << "\" shape=box ]\n";
    }
    dot << "\n";
    for (const auto &it : edges) {
        dot << "  n" << statement_indices.at(it.second->predecessor);
        dot << " -> n" << statement_indices.at(it.second->successor);
        dot << " [ label=\"e" << it.first << " (" << it.second->weight << ")\" ]\n";
    }
    dot << "\n}\n";
    dot.flush();

    // Write the key.
    for (const auto &it : edges) {
        key << "e" << it.first << ":\n";
        key << "  from " << ir::describe(it.second->predecessor) << "\n";
        key << "  to " << ir::describe(it.second->successor) << "\n";
        key << "  weight " << it.second->weight << "\n";
        key << "  because:\n";
        for (const auto &cause : it.second->causes) {
            key << "    " << cause << "\n";
        }
        key << "\n";
    }
    key.flush();

}

} // namespace ddg
} // namespace com
} // namespace ql
