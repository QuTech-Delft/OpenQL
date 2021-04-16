/** \file
 * Platform header for target-specific compilation.
 */

#include "ql/plat/platform.h"

namespace ql {
namespace plat {

using namespace utils;

/**
 * Constructs a platform from the given configuration filename.
 */
Platform::Platform(
    const Str &name,
    const Str &configuration_file_name
) :
    name(name),
    configuration_file_name(configuration_file_name)
{
    HardwareConfiguration hwc(configuration_file_name);
    hwc.load(instruction_map, instruction_settings, hardware_settings, resources, topology);
    eqasm_compiler_name = hwc.eqasm_compiler_name;
    QL_DOUT("eqasm_compiler_name= " << eqasm_compiler_name);

    if (hardware_settings.count("qubit_number") <= 0) {
        QL_FATAL("qubit number of the platform is not specified in the configuration file !");
    } else {
        qubit_count = hardware_settings["qubit_number"];
    }
    if (hardware_settings.count("creg_number") <= 0) {
        creg_count = 0;
        compat_implicit_creg_count = true;
    } else {
        creg_count = hardware_settings["creg_number"];
        compat_implicit_creg_count = false;
    }
    if (hardware_settings.count("breg_number") <= 0) {
        breg_count = 0;
        compat_implicit_breg_count = true;
    } else {
        breg_count = hardware_settings["breg_number"];
        compat_implicit_breg_count = false;
    }

    if (hardware_settings.count("cycle_time") <= 0) {
        QL_FATAL("cycle time of the platform is not specified in the configuration file !");
    } else {
        cycle_time = hardware_settings["cycle_time"];
    }

    grid.emplace(qubit_count, topology);
}

/**
 * display information about the platform
 */
void Platform::print_info() const {
    QL_PRINTLN("[+] platform name      : " << name);
    QL_PRINTLN("[+] qubit number       : " << qubit_count);
    QL_PRINTLN("[+] creg number        : " << creg_count);
    QL_PRINTLN("[+] breg number        : " << breg_count);
    QL_PRINTLN("[+] eqasm compiler     : " << eqasm_compiler_name);
    QL_PRINTLN("[+] configuration file : " << configuration_file_name);
    QL_PRINTLN("[+] supported instructions:");
    for (const auto &i : instruction_map) {
        QL_PRINTLN("  |-- " << i.first);
    }
}

// find settings for custom gate, preventing JSON exceptions
const Json &Platform::find_instruction(const Str &iname) const {
    // search the JSON defined instructions, to prevent JSON exception if key does not exist
    if (!QL_JSON_EXISTS(instruction_settings, iname)) {
        QL_FATAL("JSON file: instruction not found: '" << iname << "'");
    }
    return instruction_settings[iname];
}


// find instruction type for custom gate
Str Platform::find_instruction_type(const Str &iname) const {
    const Json &instruction = find_instruction(iname);
    if (!QL_JSON_EXISTS(instruction, "type")) {
        QL_FATAL("JSON file: field 'type' not defined for instruction '" << iname << "'");
    }
    return instruction["type"];
}

UInt Platform::time_to_cycles(Real time_ns) const {
    return ceil(time_ns / cycle_time);
}

} // namespace plat
} // namespace ql
