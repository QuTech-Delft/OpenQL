/**
 * @file   decompose_toffoli.h
 * @date   11/2016
 * @author Nader Khammassi
 */
#ifndef DECOMPOSE_TOFFOLI_H
#define DECOMPOSE_TOFFOLI_H

#include "program.h"
#include "platform.h"

namespace ql
{
    // rotation_optimize pass
    void decompose_toffoli(ql::quantum_program* programp, const ql::quantum_platform& platform, std::string passname);
}

#endif // DECOMPOSE_TOFFOLI_H
