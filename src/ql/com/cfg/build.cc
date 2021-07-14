/** \file
 * Defines the structures and functions used to construct the control-flow
 * graph for a program.
 */

#include "ql/com/cfg/build.h"

#include "ql/ir/describe.h"
#include "ql/com/cfg/ops.h"

namespace ql {
namespace com {
namespace cfg {

/**
 * Ensures that a node exists for the given block, and returns that node.
 */
static NodeRef ensure_node(const ir::BlockRef &block) {
    if (auto node = block->get_annotation_ptr<NodeRef>()) {
        return *node;
    }
    NodeRef node;
    node.emplace();
    block->set_annotation<NodeRef>(node);
    return node;
}

/**
 * Creates a CFG edge between the two given blocks.
 */
static void add_edge(const ir::BlockRef &from, const ir::BlockRef &to, const ir::BlockRef &sink) {
    auto target = to.empty() ? sink : to;
    QL_ASSERT(from != target);
    auto from_node = ensure_node(from);
    auto target_node = ensure_node(target);
    auto result = from_node->successors.insert({target, {}});
    if (!result.second) {
        return;
    }
    auto &edge_ref = result.first->second;
    edge_ref.emplace();
    edge_ref->predecessor = from;
    edge_ref->successor = target;
    QL_ASSERT(target_node->predecessors.insert({from, edge_ref}).second);
}

/**
 * Processes a block, ensuring that there is a CFG node for it and creating its
 * outgoing edges.
 */
static void process_block(const ir::BlockRef &block, const ir::BlockRef &sink) {
    ensure_node(block);
    for (const auto &statement : block->statements) {
        if (!statement->as_instruction()) {
            QL_ICE(
                "found non-instruction in program; cannot construct CFG: " <<
                ir::describe(statement)
            );
        }
        if (auto gi = statement->as_goto_instruction()) {
            add_edge(block, gi->target.as_mut(), sink);
        }
    }
    add_edge(block, block->next.as_mut(), sink);
}

/**
 * Builds a control-flow graph for the given program.
 *
 * It's not possible to construct a CFG for a program that still contains
 * structured control-flow. It must in that case be converted to basic-block
 * form first.
 *
 * The nodes of the graph are represented by the blocks in the program and
 * two sentinel blocks, known as the source and the sink. The edges are formed
 * by dependencies from one block to another.
 */
void build(const ir::ProgramRef &program) {

    // Remove any existing CFG annotations.
    clear(program);

    // Make the source and sink nodes and attach them to the block via the
    // Graph annotation.
    auto source = utils::make<ir::Block>("@SOURCE");
    source->next = program->entry_point;
    auto sink = utils::make<ir::Block>("@SINK");
    program->set_annotation<Graph>({source, sink});

    // Process all blocks.
    process_block(source, sink);
    for (const auto &block : program->blocks) {
        process_block(block, sink);
    }
    ensure_node(sink);

}

} // namespace cfg
} // namespace com
} // namespace ql
