/** \file
 * Implementation of pass that writes sweep points.
 */

#pragma once

#include "ql/utils/str.h"
#include "ql/plat/platform.h"
#include "ql/ir/ir.h"

namespace ql {

void write_sweep_points(
    const ir::ProgramRef &program,
    const plat::PlatformRef &platform,
    const utils::Str &passname
);

} // namespace ql
