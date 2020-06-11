/**
 * @file   optimizer.h
 * @date   11/2016
 * @author Nader Khammassi
 * @brief  optimizer interface and its implementation
 * @todo   implementations should be in separate files for better readability
 */
#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "program.h"
#include "platform.h"

namespace ql
{
    // rotation_optimize pass
    void rotation_optimize(ql::quantum_program* programp, const ql::quantum_platform& platform, std::string passname);
}

#endif // OPTIMIZER_H
