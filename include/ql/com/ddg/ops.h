/** \file
 * Defines functions for operating on an existing data dependency graph.
 */

#pragma once

#include "ql/com/ddg/types.h"

namespace ql {
namespace com {
namespace ddg {

/**
 * Returns the DDG node associated with the given statement, if any.
 */
NodeCRef get_node(const ir::StatementRef &statement);

/**
 * Returns the source statement associated with the given block, if any.
 */
utils::One<ir::SentinelStatement> get_source(const ir::BlockBaseRef &block);

/**
 * Shorthand for getting the source node.
 */
NodeCRef get_source_node(const ir::BlockBaseRef &block);

/**
 * Returns the sink statement associated with the given block, if any.
 */
utils::One<ir::SentinelStatement> get_sink(const ir::BlockBaseRef &block);

/**
 * Shorthand for getting the sink node.
 */
NodeCRef get_sink_node(const ir::BlockBaseRef &block);

/**
 * Returns the DDG edge between the two given statements, or returns an empty
 * edge reference if there is no edge between the statements. Note that this is
 * directional.
 */
EdgeCRef get_edge(const ir::StatementRef &from, const ir::StatementRef &to);

/**
 * Returns the effective scheduling direction when scheduling using this DDG.
 */
utils::Int get_direction(const ir::BlockBaseRef &block);

/**
 * Removes the data dependency graph annotations from the given block.
 */
void clear(const ir::BlockBaseRef &block);

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
void reverse(const ir::BlockBaseRef &block);

/**
 * Add the Remaining annotation to nodes in the graph.
 * Remaining gives the remaining length of the critical path.
 * Can be used to e.g. compare which gate is most critical.
 */
void add_remaining(const ir::BlockBaseRef &block);

} // namespace ddg
} // namespace com
} // namespace ql
