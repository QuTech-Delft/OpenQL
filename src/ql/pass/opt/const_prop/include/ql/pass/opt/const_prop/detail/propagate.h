/** \file
 * Recursively perform constant propagation on an IR node
 */

#pragma once

#include "ql/ir/ir.h"

namespace ql {
namespace pass {
namespace opt {
namespace const_prop {
namespace detail {

/**
 * Recursively perform constant propagation on an IR node.
 */
void propagate(
    const ir::Ref &ir,
    ir::Node &node
);

} // namespace detail
} // namespace const_prop
} // namespace opt
} // namespace pass
} // namespace ql
