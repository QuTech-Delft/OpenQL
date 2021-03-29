/** \file
 * Toffoli gate decomposer pass implementation.
 */

#pragma once

#include "ql/utils/str.h"
#include "ql/plat/platform.h"
#include "ql/ir/ir.h"

namespace ql {

void decompose_toffoli(
    const ir::ProgramRef &program,
    const plat::PlatformRef &platform,
    const utils::Str &passname
);

} // namespace ql
