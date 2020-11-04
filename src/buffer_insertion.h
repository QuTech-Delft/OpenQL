/**
 * @file   buffer_insertion.h
 * @date   11/2016
 * @author Nader Khammassi
 * @author Hans van Someren
 */

#pragma once

#include "utils/strings.h"
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
