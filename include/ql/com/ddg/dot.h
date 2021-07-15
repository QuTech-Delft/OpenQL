/** \file
 * Defines ways to visualize the data dependency graph using a graphviz dot
 * file, useful when debugging.
 */

#pragma once

#include "ql/com/ddg/ops.h"

namespace ql {
namespace com {
namespace ddg {

/**
 * Dumps a dot representation of the data dependency graph for the given block,
 * including the current cycle numbers.
 */
void dump_dot(
    const ir::BlockBaseRef &block,
    std::ostream &os = std::cout,
    const utils::Str &line_prefix = ""
);

} // namespace ddg
} // namespace com
} // namespace ql
