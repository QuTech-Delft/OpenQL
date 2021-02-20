/** \file
 * Rotation optimizer pass implementation.
 */

#pragma once

#include "utils/str.h"
#include "program.h"
#include "platform.h"

namespace ql {

void rotation_optimize(
    quantum_program *programp,
    const quantum_platform &platform,
    const utils::Str &passname
);

} // namespace ql
