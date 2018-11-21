/**
 * @file   openql.h
 * @date   11/2016
 * @author Nader Khammassi
 * @brief  main openql header
 */
#ifndef OPENQL_H
#define OPENQL_H

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

/**
 * configurable instruction map
 */
/* static */ dep_instruction_map_t dep_instruction_map;
/* static */ // bool              initialized = false;
/* static */ // ql_platform_t     target_platform;

// target platform
ql::quantum_platform           target_platform;


// deprecated : for back compatibility
// should be removed
/*
void init(ql_platform_t platform, std::string dep_instruction_map_file="")
{
}
*/

void set_platform(ql::quantum_platform platform)
{
    target_platform = platform;
}

}

#endif // OPENQL_H
