/** \file
 * Defines ways to visualize the dependency graph using dot and console output,
 * useful when debugging.
 */

#pragma once

#include "ql/com/ddg/ops.h"

namespace ql {
namespace com {
namespace ddg {

/**
 * Dumps a dot file and accompanying key.
 */
void dump_dot(
    const ir::BlockBaseRef &block,
    std::ostream &os = std::cout,
    const utils::Str &line_prefix = ""
);

} // namespace ddg
} // namespace com
} // namespace ql
