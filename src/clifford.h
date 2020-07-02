/**
 * @file   clifford.h
 * @date   05/2019
 * @author Hans van Someren
 * @brief  clifford sequence optimizer
 */
#ifndef CLIFFORD_H
#define CLIFFORD_H

#include "program.h"
#include "platform.h"


namespace ql
{

/*
 * clifford sequence optimizer
 */
    void clifford_optimize(quantum_program* programp, const ql::quantum_platform& platform, std::string passname);
}

#endif // CLIFFORD_H
