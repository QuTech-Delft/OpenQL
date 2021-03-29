/** \file
 * Buffer insertion pass.
 *
 * \see buffer_insertion.cc
 */

#pragma once

#include "ql/utils/str.h"
#include "ql/plat/platform.h"
#include "ql/ir/ir.h"

namespace ql {

// buffer_delay_insertion pass
void insert_buffer_delays(
    const ir::ProgramRef &program,
    const plat::PlatformRef &platform,
    const utils::Str &passname
);

} // namespace ql
