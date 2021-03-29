/** \file
 * Clifford sequence optimizer.
 */

#pragma once

#include "ql/utils/str.h"
#include "ql/plat/platform.h"
#include "ql/ir/ir.h"

namespace ql {

/**
 * Clifford sequence optimizer.
 */
void clifford_optimize(
    const ir::ProgramRef &programp,
    const plat::PlatformRef &platform,
    const utils::Str &passname
);

} // namespace ql
