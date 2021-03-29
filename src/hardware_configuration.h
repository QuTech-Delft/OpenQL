/** \file
 * JSON hardware configuration loader.
 */

#pragma once

#include "ql/utils/str.h"
#include "ql/utils/map.h"
#include "ql/utils/ptr.h"
#include "ql/ir/gate.h"

namespace ql {

using CustomGateRef = utils::One<ir::gates::Custom>;

using InstructionMap = utils::Map<utils::Str, CustomGateRef>;

/**
 * loading hardware configuration
 */
class hardware_configuration {
public:
    utils::Str config_file_name;
    utils::Str eqasm_compiler_name;

    hardware_configuration(const utils::Str &config_file_name);

    void load(
        InstructionMap &instruction_map,
        utils::Json &instruction_settings,
        utils::Json &hardware_settings,
        utils::Json &resources,
        utils::Json &topology,
        utils::Json &aliases
    );

};

}
