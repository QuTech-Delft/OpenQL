/**
 * @file   latency_compensation.h
 * @date   11/2016
 * @author Nader Khammassi
 * @author Hans van Someren
 */
#ifndef LATENCY_COMPENSATION_H
#define LATENCY_COMPENSATION_H

#include "program.h"
#include "platform.h"

namespace ql
{
    // buffer_delay_insertion pass
    void latency_compensation(ql::quantum_program* programp, const ql::quantum_platform& platform, std::string passname);
}

#endif // LATENCY_COMPENSATION_H
