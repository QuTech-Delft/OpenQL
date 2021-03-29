/** \file
 * Rotation optimizer pass implementation.
 */

#pragma once

#include "ql/utils/str.h"
#include "ql/ir/ir.h"
#include "ql/plat/platform.h"

namespace ql {

void rotation_optimize(
    const ir::ProgramRef &program,
    const plat::PlatformRef &platform,
    const utils::Str &passname
);

} // namespace ql
