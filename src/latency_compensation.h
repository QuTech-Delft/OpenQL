/** \file
 * Latency compensation?
 */

#pragma once

#include "ql/utils/str.h"
#include "program.h"
#include "platform.h"

namespace ql {

// buffer_delay_insertion pass
void latency_compensation(
    ir::Program &program,
    const quantum_platform &platform,
    const utils::Str &passname
);

}
