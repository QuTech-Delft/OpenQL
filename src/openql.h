/**
 * @file   openql.h
 * @date   11/2016
 * @author Nader Khammassi
 * @brief  main openql header
 */
#ifndef OPENQL_H
#define OPENQL_H

#if defined(WIN32)
#define __attribute__(A) /* do nothing on windows */
#endif

#include "compile_options.h"
#include "instruction_map.h"
#include "optimizer.h"
#include "circuit.h"
#include "program.h"
#include "cqasm/cqasm_reader.h"

namespace ql
{
/**
 * openql types
 */

#if OPT_MICRO_CODE
/**
 * configurable instruction map
 */
/* static */ dep_instruction_map_t dep_instruction_map;
#endif


#if OPT_TARGET_PLATFORM
// target platform
ql::quantum_platform           target_platform;
#endif

#if OPT_TARGET_PLATFORM
// NB: set_plaform is already removed from SWIG interface
void set_platform(ql::quantum_platform platform)
{
    target_platform = platform;
}
#endif
}

#endif // OPENQL_H
