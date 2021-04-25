/** \file
 * JSON hardware configuration loader.
 */

#pragma once

#include "ql/utils/str.h"
#include "ql/utils/map.h"
#include "ql/utils/ptr.h"
#include "ql/ir/gate.h"
#include "ql/arch/architecture.h"

namespace ql {
namespace plat {

using CustomGateRef = utils::One<ir::gate_types::Custom>;

using InstructionMap = utils::Map<utils::Str, CustomGateRef>;

/**
 * loading hardware configuration
 */
class HardwareConfiguration {
public:
    utils::Json config;

    HardwareConfiguration(const utils::Json &config);

    void load(
        InstructionMap &instruction_map,
        arch::CArchitectureRef &architecture,
        utils::Json &compiler_settings,
        utils::Json &instruction_settings,
        utils::Json &hardware_settings,
        utils::Json &resources,
        utils::Json &topology
    );

};

} // namespace plat
} // namespace ql
