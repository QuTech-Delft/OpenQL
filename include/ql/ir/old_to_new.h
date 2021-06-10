/** \file
 * Provides the conversion from the old IR (still used for the API for backward
 * compatibility) to the new one.
 */

#pragma once

#include "ql/ir/compat/compat.h"
#include "ql/ir/ir.h"

namespace ql {
namespace ir {

/**
 * Converts the old IR (program and platform) to the new one.
 */
Ref convert_old_to_new(const compat::ProgramRef &old);

} // namespace ir
} // namespace ql
