/** \file
 * Latency compensation?
 */

#pragma once

#include "utils/str.h"
#include "program.h"
#include "platform.h"

namespace ql {

// buffer_delay_insertion pass
void latency_compensation(
    quantum_program *programp,
    const quantum_platform &platform,
    const utils::Str &passname
);

}
