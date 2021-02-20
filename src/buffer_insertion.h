/** \file
 * Buffer insertion pass.
 *
 * \see buffer_insertion.cc
 */

#pragma once

#include "utils/str.h"
#include "program.h"
#include "platform.h"

namespace ql {

// buffer_delay_insertion pass
void insert_buffer_delays(
    quantum_program *programp,
    const quantum_platform &platform,
    const utils::Str &passname
);

} // namespace ql
