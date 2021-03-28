/** \file
 * Buffer insertion pass.
 *
 * \see buffer_insertion.cc
 */

#pragma once

#include "ql/utils/str.h"
#include "program.h"
#include "platform.h"

namespace ql {

// buffer_delay_insertion pass
void insert_buffer_delays(
    ir::Program &program,
    const quantum_platform &platform,
    const utils::Str &passname
);

} // namespace ql
