/** \file
 * JSON hardware configuration loader.
 */

#pragma once

#include "utils/str.h"
#include "utils/map.h"
#include "gate.h"

namespace ql {

typedef utils::Map<utils::Str, custom_gate*> instruction_map_t;

/**
 * loading hardware configuration
 */
class hardware_configuration {
public:
    utils::Str config_file_name;
    utils::Str eqasm_compiler_name;

    hardware_configuration(const utils::Str &config_file_name);

    void load(
        instruction_map_t &instruction_map,
        utils::Json &instruction_settings,
        utils::Json &hardware_settings,
        utils::Json &resources,
        utils::Json &topology,
        utils::Json &aliases
    );

};

}
