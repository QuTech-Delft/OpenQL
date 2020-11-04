/**
 * @file   hardware_configuration.h
 * @date   07/2017
 * @author Nader Khammassi
 *         Imran Ashraf
 * @brief  hardware configuration loader
 */

#pragma once

#include "gate.h"

namespace ql {

typedef std::map<std::string, custom_gate *> instruction_map_t;

/**
 * loading hardware configuration
 */
class hardware_configuration {
public:
    std::string config_file_name;
    std::string eqasm_compiler_name;

    hardware_configuration(const std::string &config_file_name);

    void load(
        instruction_map_t &instruction_map,
        utils::json &instruction_settings,
        utils::json &hardware_settings,
        utils::json &resources,
        utils::json &topology,
        utils::json &aliases
    );

};

}
