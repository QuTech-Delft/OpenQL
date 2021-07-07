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
void write_dot(
    const ir::BlockBaseRef &block,
    std::ostream &dot,
    std::ostream &key
);

} // namespace ddg
} // namespace com
} // namespace ql
