/** \file
 * Implementation of pass that writes sweep points.
 */

#pragma once

#include "utils/str.h"
#include "program.h"
#include "platform.h"

namespace ql {

void write_sweep_points(
    quantum_program *programp,
    const quantum_platform &platform,
    const utils::Str &passname
);

} // namespace ql
