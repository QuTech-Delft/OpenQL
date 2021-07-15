/** \file
 * Defines functions for operating on an existing control-flow graph.
 */

#include "ql/com/cfg/ops.h"

namespace ql {
namespace com {
namespace cfg {

/**
 * Returns the CFG node associated with the given block, if any.
 */
NodeCRef get_node(const ir::BlockRef &block) {
    if (block.empty()) {
        return {};
    }
    if (auto node = block->get_annotation_ptr<NodeRef>()) {
        return node->as_const();
    } else {
        return {};
    }
}

/**
 * Returns the source block associated with the given program, if any.
 */
ir::BlockRef get_source(const ir::ProgramRef &program) {
    if (auto data = program->get_annotation_ptr<Graph>()) {
        return data->source;
    } else {
        return {};
    }
}

/**
 * Shorthand for getting the source node.
 */
NodeCRef get_source_node(const ir::ProgramRef &program) {
    return get_node(get_source(program));
}

/**
 * Returns the sink block associated with the given program, if any.
 */
ir::BlockRef get_sink(const ir::ProgramRef &program) {
    if (auto data = program->get_annotation_ptr<Graph>()) {
        return data->sink;
    } else {
        return {};
    }
}

/**
 * Shorthand for getting the sink node.
 */
NodeCRef get_sink_node(const ir::ProgramRef &program) {
    return get_node(get_sink(program));
}

/**
 * Returns the CFG edge between the two given blocks, or returns an empty
 * edge reference if there is no edge between the blocks. Note that this is
 * directional.
 */
EdgeCRef get_edge(const ir::BlockRef &from, const ir::BlockRef &to) {
    const auto &successors = get_node(from)->successors;
    auto it = successors.find(to);
    if (it == successors.end()) {
        return {};
    } else {
        return it->second.as_const();
    }
}

/**
 * Removes the control-flow graph annotations from the given program.
 */
void clear(const ir::ProgramRef &program) {
    program->erase_annotation<Graph>();
    for (const auto &block : program->blocks) {
        block->erase_annotation<NodeRef>();
    }
}

} // namespace cfg
} // namespace com
} // namespace ql
