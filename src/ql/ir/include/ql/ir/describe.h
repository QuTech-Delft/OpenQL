/** \file
 * Defines a serializer for generating single-line descriptions of certain IR
 * nodes, useful within error messages and debug messages.
 */

#pragma once

#include "ql/utils/str.h"
#include "ql/ir/ir.h"

namespace ql {
namespace ir {

/**
 * Gives a one-line description of a node.
 */
void describe(const Node &node, std::ostream &ss);

/**
 * Gives a one-line description of a node.
 */
void describe(const utils::One<Node> &node, std::ostream &ss);

/**
 * Gives a one-line description of a node.
 */
utils::Str describe(const Node &node);

/**
 * Gives a one-line description of a node.
 */
utils::Str describe(const utils::One<Node> &node);

} // namespace ir
} // namespace ql
