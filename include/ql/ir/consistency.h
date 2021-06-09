/** \file
 * Defines a consistency check function for the IR.
 */

#pragma once

#include "ql/ir/ir.h"

namespace ql {
namespace ir {

/**
 * Performs a consistency check of the IR. An exception is thrown if a problem
 * is found. The constraints checked by this must be met on any interface that
 * passes an IR reference, although actually checking it on every interface
 * might be detrimental for performance.
 */
void check_consistency(const Ref &ir);

} // namespace ir
} // namespace ql
