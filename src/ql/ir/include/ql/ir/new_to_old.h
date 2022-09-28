/** \file
 * Provides the conversion from the new one to the old one for compatibility and
 * testing purposes.
 */

#pragma once

#include "ql/ir/compat/compat.h"
#include "ql/ir/ir.h"

namespace ql {
namespace ir {

/**
 * Converts the new IR to the old one. This requires that the platform was
 * constructed using convert_old_to_new(), and (obviously) that no features of
 * the new IR are used that are not supported by the old IR.
 */
compat::ProgramRef convert_new_to_old(const Ref &ir);

} // namespace ir
} // namespace ql
