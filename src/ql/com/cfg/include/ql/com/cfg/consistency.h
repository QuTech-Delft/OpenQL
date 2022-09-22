/** \file
 * Defines a consistency check for a CFG, useful when debugging.
 */

#pragma once

#include "ql/ir/ir.h"

namespace ql {
namespace com {
namespace cfg {

/**
 * Checks consistency of the control-flow graph associated with the given
 * program. Throws an ICE or assertion failure if an inconsistency was found.
 */
void check_consistency(const ir::ProgramRef &program);

} // namespace cfg
} // namespace com
} // namespace ql
