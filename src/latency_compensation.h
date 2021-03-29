/** \file
 * Latency compensation?
 */

#pragma once

#include "ql/utils/str.h"
#include "ql/plat/platform.h"
#include "ql/ir/ir.h"

namespace ql {

// buffer_delay_insertion pass
void latency_compensation(
    const ir::ProgramRef &program,
    const plat::PlatformRef &platform,
    const utils::Str &passname
);

} // namespace ql
