/**
 * @file   write_sweep_points.h
 * @date   11/2016
 * @author Nader Khammassi
 */

#pragma once

#include "program.h"
#include "platform.h"

namespace ql {

void write_sweep_points(
    ql::quantum_program *programp,
    const ql::quantum_platform &platform,
    const std::string &passname
);

} // namespace ql
