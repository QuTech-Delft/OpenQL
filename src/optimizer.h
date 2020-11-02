/**
 * @file   optimizer.h
 * @date   11/2016
 * @author Nader Khammassi
 * @brief  optimizer interface and its implementation
 * @todo   implementations should be in separate files for better readability
 */

#pragma once

#include "program.h"
#include "platform.h"

namespace ql {

void rotation_optimize(
    ql::quantum_program *programp,
    const ql::quantum_platform &platform,
    const std::string &passname
);

} // namespace ql
