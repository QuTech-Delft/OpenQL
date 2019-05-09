/**
 * @file   openql.h
 * @date   11/2016
 * @author Nader Khammassi
 * @brief  main openql header
 */
#ifndef OPENQL_H
#define OPENQL_H

#include <ql/compile_options.h>
#include "instruction_map.h"
#include "optimizer.h"
#include "circuit.h"
#include "program.h"

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

void set_platform(ql::quantum_platform platform)
{
#if OPT_TARGET_PLATFORM
    target_platform = platform;
#else
    WOUT("set_platform() is not necessary and will therefore be deprecated");
#endif
}
}

#endif // OPENQL_H
