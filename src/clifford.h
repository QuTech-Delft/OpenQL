/** \file
 * Clifford sequence optimizer.
 */

#pragma once

#include "ql/utils/str.h"
#include "program.h"
#include "platform.h"

namespace ql {

/**
 * Clifford sequence optimizer.
 */
void clifford_optimize(
    ir::Program &programp,
    const quantum_platform &platform,
    const utils::Str &passname
);

} // namespace ql
