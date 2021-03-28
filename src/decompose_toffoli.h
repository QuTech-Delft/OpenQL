/** \file
 * Toffoli gate decomposer pass implementation.
 */

#pragma once

#include "ql/utils/str.h"
#include "program.h"
#include "platform.h"

namespace ql {

void decompose_toffoli(
    ir::Program &program,
    const quantum_platform &platform,
    const utils::Str &passname
);

} // namespace ql
