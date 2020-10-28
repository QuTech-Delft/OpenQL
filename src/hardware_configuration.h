/**
 * @file   hardware_configuration.h
 * @date   07/2017
 * @author Nader Khammassi
 *         Imran Ashraf
 * @brief  hardware configuration loader
 */

#pragma once

#include "utils.h"
#include "gate.h"

namespace ql {

typedef std::map<std::string, ql::custom_gate *> instruction_map_t;

/**
 * loading hardware configuration
 */
class hardware_configuration {
public:
    std::string config_file_name;
    std::string eqasm_compiler_name;

    hardware_configuration(const std::string &config_file_name);

    void load(
        ql::instruction_map_t &instruction_map,
        json &instruction_settings,
        json &hardware_settings,
        json &resources,
        json &topology,
        json &aliases
    );

};

}
