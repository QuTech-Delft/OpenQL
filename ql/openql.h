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
#include "transmon.h"
#include "program.h"

#include <fstream>
#include <map>

namespace ql
{
/**
 * openql types
 */

typedef std::vector<float> sweep_points_t;
typedef std::stringstream  str_t;


/**
 * configurable instruction map
 */
/* static */ dep_instruction_map_t dep_instruction_map;
/* static */ bool              initialized = false;
/* static */ // ql_platform_t     target_platform;

// target platform
ql::quantum_platform           target_platform;          


// deprecated : for back compatibility
// should be removed
void init(ql_platform_t platform, std::string dep_instruction_map_file="")
{
}

void set_platform(ql::quantum_platform platform)
{
    target_platform = platform;
}



// void init(ql_platform_t platform, std::string dep_instruction_map_file="")
void init(ql::quantum_platform platform=ql::quantum_platform(), std::string dep_instruction_map_file="")
{
    target_platform = platform;

    if (dep_instruction_map_file != "")
    {
        if (!load_instruction_map(dep_instruction_map_file,dep_instruction_map))
            println("[x] error : failed to load the instruction map !");
    }
    else
    {
#if 0
        /**
         * predefined setups : transmon, starmon..
         */
        if (target_platform == transmon_platform)
        {
#endif
            dep_instruction_map["rx90" ]   = ucode_inst_t("     pulse 1011 0000 1011\n     wait 10");
            dep_instruction_map["mrx90"]   = ucode_inst_t("     pulse 1101 0000 1101\n     wait 10");
            dep_instruction_map["rx180"]   = ucode_inst_t("     pulse 1001 0000 1001\n     wait 10");
            dep_instruction_map["ry90" ]   = ucode_inst_t("     pulse 1100 0000 1100\n     wait 10");
            dep_instruction_map["mry90"]   = ucode_inst_t("     pulse 1110 0000 1110\n     wait 10");
            dep_instruction_map["ry180"]   = ucode_inst_t("     pulse 1010 0000 1010\n     wait 10");
            dep_instruction_map["prepz"]   = ucode_inst_t("     waitreg r0\n     waitreg r0\n");
            dep_instruction_map["measure"] = ucode_inst_t("     wait 60\n     pulse 0000 1111 1111\n     wait 50\n     measure\n");
#if 0
        }
        else if (target_platform == starmon_platform)
        {
        }
#endif

    }
    initialized = true;
}

/**
 * generate qasm for a give circuit
 */
std::string qasm(ql::circuit c)
{
    std::stringstream ss;
    for (size_t i=0; i<c.size(); ++i)
    {
        ss << c[i]->qasm() << "\n";
        std::cout << c[i]->qasm() << std::endl;
    }
    return ss.str();
}

}

#endif // OPENQL_H
