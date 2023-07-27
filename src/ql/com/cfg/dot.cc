/** \file
 * Defines ways to visualize the control-flow graph using a graphviz dot file,
 * useful when debugging.
 */

#include "ql/com/cfg/dot.h"

#include "ql/ir/cqasm/write.h"

namespace ql {
namespace com {
namespace cfg {

/**
 * Dumps a dot file representing the control-flow graph attached to the given
 * program.
 */
void dump_dot(
    const ir::Ref &ir,
    std::ostream &os,
    const utils::Str &line_prefix
) {

    // Write the header.
    os << line_prefix << "digraph ddg {\n";
    os << line_prefix << "\n";
    os << line_prefix << "  graph [ rankdir=TD ]\n";
    os << line_prefix << "  edge [ fontsize=16, arrowhead=vee, arrowsize=0.5 ]\n";
    os << line_prefix << "  node [ shape=box, fontcolor=black, style=filled, fontsize=12, fontname=Courier ]\n";
    os << line_prefix << "\n";

    // Get a list of all the blocks (including source and sink) for convenience.
    utils::List<ir::BlockRef> blocks;
    auto source = get_source(ir->program);
    blocks.push_back(source);
    for (const auto &block : ir->program->blocks) {
        blocks.push_back(block);
    }
    auto sink = get_sink(ir->program);
    blocks.push_back(sink);

    // Write the graph nodes.
    for (const auto &block : blocks) {
        os << line_prefix << "  \"" << block->name << "\"";
        os << " [ label=\"";
        utils::Str desc;
        if (block == source) {
            desc = "entry (source)";
        } else if (block == sink) {
            desc = "exit (sink)";
        } else {
            desc = "." + block->name + "\n";
            desc += ir::cqasm::to_string(ir, block);
            if (block->next.empty()) {
                desc += "exit\n";
            } else {
                desc += "goto " + block->next->name + "\n";
            }
        }
        desc = utils::replace_all(desc, "\\", "\\\\");
        desc = utils::replace_all(desc, "\n", "\\l");
        os << desc << "\"";
        if (block == source || block == sink) {
            os << ", shape=oval";
        }
        os << " ]\n";
    }
    os << line_prefix << "\n";

    // Write the edges.
    for (const auto &block : blocks) {
        auto node = get_node(block);
        for (const auto &endpoint : node->successors) {
            auto edge = endpoint.second;
            os << line_prefix << "  ";
            os << "\"" << edge->predecessor->name << "\"";
            os << " -> ";
            os << "\"" << edge->successor->name << "\"";
            os << "\n";
        }
    }
    os << line_prefix << "\n";

    // Write the footer.
    os << line_prefix << "}" << std::endl;

}

} // namespace cfg
} // namespace com
} // namespace ql
