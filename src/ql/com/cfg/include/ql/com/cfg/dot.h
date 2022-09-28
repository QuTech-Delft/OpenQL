/** \file
 * Defines ways to visualize the control-flow graph using a graphviz dot file,
 * useful when debugging.
 */

#pragma once

#include "ql/com/cfg/ops.h"

namespace ql {
namespace com {
namespace cfg {

/**
 * Dumps a dot file representing the control-flow graph attached to the given
 * program.
 */
void dump_dot(
    const ir::Ref &ir,
    std::ostream &os = std::cout,
    const utils::Str &line_prefix = ""
);

} // namespace cfg
} // namespace com
} // namespace ql
