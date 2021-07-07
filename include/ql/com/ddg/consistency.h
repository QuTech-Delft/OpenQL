/** \file
 * Defines a consistency check for a DDG, useful when debugging.
 */

#pragma once

#include "ql/ir/ir.h"

namespace ql {
namespace com {
namespace ddg {

/**
 * Checks consistency of the data dependency graph associated with the given
 * block. Throws an ICE or assertion failure if an inconsistency was found.
 */
void check_consistency(const ir::BlockBaseRef &block);

} // namespace ddg
} // namespace com
} // namespace ql
