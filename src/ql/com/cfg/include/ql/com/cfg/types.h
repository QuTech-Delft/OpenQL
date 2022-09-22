/** \file
 * Defines types for representing the control-flow graph.
 */

#pragma once

#include "ql/utils/map.h"
#include "ql/ir/ir.h"

namespace ql {
namespace com {
namespace cfg {

/**
 * Represents an edge in the control-flow graph.
 */
struct Edge {

    /**
     * Reference to the block that the edge originates from.
     */
    ir::BlockRef predecessor = {};

    /**
     * Reference to the block that the edge targets.
     */
    ir::BlockRef successor = {};

};

/**
 * Reference to a CFG edge.
 */
using EdgeRef = utils::Ptr<Edge>;

/**
 * Const reference to a CFG edge.
 */
using EdgeCRef = utils::Ptr<const Edge>;

/**
 * Shorthand for a list of endpoints for a block.
 */
using Endpoints = utils::Map<ir::BlockRef, EdgeRef>;

/**
 * A node in the CFG.
 */
struct Node {

    /**
     * The endpoints of the incoming edges for this node.
     */
    Endpoints predecessors;

    /**
     * The endpoints of the outgoing edges for this node.
     */
    Endpoints successors;

};

/**
 * A reference to a CFG node. This is attached to blocks via an annotation.
 */
using NodeRef = utils::Ptr<Node>;

/**
 * A const reference to a CFG node.
 */
using NodeCRef = utils::Ptr<const Node>;

/**
 * Annotation structure placed on a program when the CFG is constructed,
 * containing things that need to be tracked for the CFG as a whole.
 */
struct Graph {

    /**
     * The source block, serving as a sentinel that precedes the entry point.
     */
    ir::BlockRef source;

    /**
     * The sink block, serving as a sentinel that executes after program
     * termination.
     */
    ir::BlockRef sink;

};

} // namespace cfg
} // namespace com
} // namespace ql
