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

    // Write the header.
    os << line_prefix << "digraph ddg {\n";
    os << line_prefix << "\n";
    os << line_prefix << "  graph [ rankdir=TD ]\n";
    os << line_prefix << "  edge [ fontsize=16, arrowhead=vee, arrowsize=0.5 ]\n";
    os << line_prefix << "\n";

    // Write the graph nodes.
    for (const auto &it : statements) {
        os << line_prefix << "  n" << it.first;
        os << " [ label=<n" << it.first << ", cycle " << it.second->cycle << "<br/>";
        auto desc = ir::describe(it.second);
        desc = utils::replace_all(desc, "<", "&lt;");
        desc = utils::replace_all(desc, ">", "&gt;");
        os << desc << ">, shape=box, fontcolor=black, style=filled, fontsize=16 ]\n";
    }
    os << line_prefix << "\n";

    // Write a separate graph with the cycle numbers to help dot place the nodes
    // and help make the graph more readable, but only do so when we have at
    // least two cycle numbers present in the graph; existence of a DDG doesn't
    // imply the code is actually scheduled.
    utils::Set<utils::Int> cycles_in_use;
    for (const auto &it : statements) {
        cycles_in_use.insert(it.second->cycle);
    }
    if (cycles_in_use.size() > 1) {
        os << line_prefix << "  {\n";
        os << line_prefix << "    node [ shape=plaintext, fontsize=16, fontcolor=blue ]\n";
        if (get_direction(block) > 0) {
            os << line_prefix << "    Source";
        } else {
            os << line_prefix << "    Sink";
        }
        utils::UInt gaps = 0;
        utils::UInt line = 0;
        utils::Int prev = utils::MAX;
        for (auto cycle : cycles_in_use) {
            if (cycle - 1 > prev) {
                os << " -> Gap" << gaps++;
                line++;
            }
            os << " -> Cycle" << cycle;
            line++;
            if (line >= 10) {
                os << "\n";
                os << line_prefix << "    Cycle" << cycle;
                line = 0;
            }
            prev = cycle;
        }
        if (get_direction(block) > 0) {
            os << " -> Sink\n";
        } else {
            os << " -> Source\n";
        }
        for (utils::UInt gap = 0; gap < gaps; gap++) {
            os << line_prefix << "    Gap" << gap << " [ label=\"...\" ]\n";
        }
        os << line_prefix << "  }\n";
        os << line_prefix << "\n";
        for (const auto &it : statements) {
            auto node = get_node(it.second);
            os << line_prefix << "  { rank=same; ";
            if (node->predecessors.empty()) {
                os << "Source";
            } else if (node->successors.empty()) {
                os << "Sink";
            } else {
                os << "Cycle" << it.second->cycle;
            }
            os << "; n" << statement_indices.at(it.second) << "; }\n";
        }
        os << line_prefix << "\n";
    }

    // Write the edges.
    for (const auto &it : edges) {
        os << line_prefix << "  n" << statement_indices.at(it.second->predecessor);
        os << " -> n" << statement_indices.at(it.second->successor);
        os << " [ label=\"" << it.second->weight << " (e" << it.first << "=";
        if (it.second->causes.size() > 1) {
            os << "...";
        } else {
            auto cause = utils::to_string(*it.second->causes.begin());
            cause = utils::replace_all(cause, "<", "&lt;");
            cause = utils::replace_all(cause, ">", "&gt;");
            os << cause;
        }
        os << ")\" ]\n";
    }
    os << line_prefix << "\n";

    // Edges can potentially have a lot of causes, which we probably don't all
    // want to print as edge labels. Therefore, edges with lots of causes just
    // get an ellipsis in the graph and a footnote at the bottom with the
    // complete list.
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
    os << line_prefix << "\n";

    // Write the footer.
    os << line_prefix << "}" << std::endl;

}

} // namespace ddg
} // namespace com
} // namespace ql
