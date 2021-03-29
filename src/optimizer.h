/** \file
 * Rotation optimizer pass implementation.
 */

#pragma once

#include "ql/utils/str.h"
#include "ql/ir/ir.h"
#include "platform.h"

namespace ql {

void rotation_optimize(
    ir::Program &program,
    const quantum_platform &platform,
    const utils::Str &passname
);

} // namespace ql
