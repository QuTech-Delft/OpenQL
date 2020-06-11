/**
 * @file   write_sweep_points.h
 * @date   11/2016
 * @author Nader Khammassi
 */
#ifndef WRITE_SWEEP_POINTS_H
#define WRITE_SWEEP_POINTS_H

#include "program.h"
#include "platform.h"

namespace ql
{
    void write_sweep_points(ql::quantum_program* programp, const ql::quantum_platform& platform, std::string passname);
}

#endif // WRITE_SWEEP_POINTS_H
