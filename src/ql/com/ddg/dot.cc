/** \file
 * Defines a consistency check for a DDG, useful when debugging.
 */

#include "ql/com/ddg/dot.h"

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
    utils::Map<utils::UInt, ir::StatementRef> &statements,
    utils::Map<ir::StatementRef, utils::UInt> &statement_indices,
    utils::Map<utils::UInt, EdgeCRef> &edges
) {
    auto node = get_node(statement);
    QL_ASSERT(statements.insert({utils::abs(node->order), statement}).second);
    QL_ASSERT(statement_indices.insert({statement, utils::abs(node->order)}).second);
    for (const auto &endpoint : node->successors) {
        QL_ASSERT(edges.insert({edges.size(), endpoint.second.as_const()}).second);
    }
}

/**
 * Dumps a dot representation of the data dependency graph for the given block,
 * including the current cycle numbers.
 */
void dump_dot(
    const ir::BlockBaseRef &block,
    std::ostream &os,
    const utils::Str &line_prefix
) {

    // Construct maps of unique numbers to nodes and edges.
    utils::Map<utils::UInt, ir::StatementRef> statements;
    utils::Map<ir::StatementRef, utils::UInt> statement_indices;
    utils::Map<utils::UInt, EdgeCRef> edges;
    add_node(get_source(block), statements, statement_indices, edges);
    for (const auto &statement : block->statements) {
        add_node(statement, statements, statement_indices, edges);
    }
    add_node(get_sink(block), statements, statement_indices, edges);

    // Write the dot graph.
    os << line_prefix << "digraph ddg {\n";
    os << line_prefix << "\n";
    for (const auto &it : statements) {
        os << line_prefix << "  n" << it.first;
        os << " [ label=<n" << it.first << "<br/>";
        auto desc = ir::describe(it.second);
        desc = utils::replace_all(desc, "<", "&lt;");
        desc = utils::replace_all(desc, ">", "&gt;");
        os << desc << "<br/>";
        os << "cycle " << it.second->cycle;
        os << "> shape=box ]\n";
    }
    os << line_prefix << "\n";
    for (const auto &it : edges) {
        os << line_prefix << "  n" << statement_indices.at(it.second->predecessor);
        os << " -> n" << statement_indices.at(it.second->successor);
        os << " [ label=\"e" << it.first << " (" << it.second->weight << "): ";
        auto cause = utils::to_string(*it.second->causes.begin());
        cause = utils::replace_all(cause, "<", "&lt;");
        cause = utils::replace_all(cause, ">", "&gt;");
        os << cause;
        if (it.second->causes.size() > 1) {
            os << ", ...";
        }
        os << "\" ]\n";
    }
    os << line_prefix << "\n";
    os << line_prefix << "  label=<";
    for (const auto &it : edges) {
        if (it.second->causes.size() > 2) {
            os << "e" << it.first << ":<br/>";
            for (const auto &cause : it.second->causes) {
                auto cause_text = utils::to_string(cause);
                cause_text = utils::replace_all(cause_text, "<", "&lt;");
                cause_text = utils::replace_all(cause_text, ">", "&gt;");
                os << cause_text << "<br/>";
            }
            os << "<br/>";
        }
    }
    os << ">\n";
    os << line_prefix << "  labelloc=b\n";
    os << line_prefix << "}" << std::endl;

}

} // namespace ddg
} // namespace com
} // namespace ql
