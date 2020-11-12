/**
 * @file   decompose_toffoli.h
 * @date   11/2016
 * @author Nader Khammassi
 */

#pragma once

#include "utils/str.h"
#include "program.h"
#include "platform.h"

namespace ql {

void decompose_toffoli(
    quantum_program *programp,
    const quantum_platform &platform,
    const utils::Str &passname
);

} // namespace ql
