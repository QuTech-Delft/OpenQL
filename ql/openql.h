/**
 * @file   openql.h
 * @date   11/2016
 * @author Nader Khammassi
 * @brief  main openql header
 */
#ifndef OPENQL_H
#define OPENQL_H

// deprecation options
#define OPT_MICRO_CODE          0   // enable old support for CBOX microcode
#define OPT_TARGET_PLATFORM     0   // use target_platform, which is not actually used


#include "instruction_map.h"
#include "optimizer.h"
#include "circuit.h"
#include "program.h"

#include <fstream>
#include <map>


namespace ql
{
/**
 * openql types
 */

typedef std::vector<float> sweep_points_t;
#if 0   // FIXME: unused
typedef std::stringstream  str_t;
#endif

#if OPT_MICRO_CODE
/**
 * configurable instruction map
 */
/* static */ dep_instruction_map_t dep_instruction_map;
#endif

/* static */ // bool              initialized = false;
/* static */ // ql_platform_t     target_platform;

#if OPT_TARGET_PLATFORM
// target platform
ql::quantum_platform           target_platform;
#endif

// deprecated : for back compatibility
// should be removed
/*
void init(ql_platform_t platform, std::string dep_instruction_map_file="")
{
}
*/

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
