/** \file
 * Defines functions for operating on an existing control-flow graph.
 */

#pragma once

#include "ql/com/cfg/types.h"

namespace ql {
namespace com {
namespace cfg {

/**
 * Returns the CFG node associated with the given block, if any.
 */
NodeCRef get_node(const ir::BlockRef &block);

/**
 * Returns the source block associated with the given program, if any.
 */
ir::BlockRef get_source(const ir::ProgramRef &program);

/**
 * Shorthand for getting the source node.
 */
NodeCRef get_source_node(const ir::ProgramRef &program);

/**
 * Returns the sink block associated with the given program, if any.
 */
ir::BlockRef get_sink(const ir::ProgramRef &program);

/**
 * Shorthand for getting the sink node.
 */
NodeCRef get_sink_node(const ir::ProgramRef &program);

/**
 * Returns the CFG edge between the two given blocks, or returns an empty
 * edge reference if there is no edge between the blocks. Note that this is
 * directional.
 */
EdgeCRef get_edge(const ir::BlockRef &from, const ir::BlockRef &to);

/**
 * Removes the control-flow graph annotations from the given program.
 */
void clear(const ir::ProgramRef &program);

} // namespace cfg
} // namespace com
} // namespace ql
