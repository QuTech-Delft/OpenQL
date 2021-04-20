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
    const Str &platform_config_file_name,
    const Str &compiler_config_file_name
) :
    name(name),
    configuration_file_name(platform_config_file_name)
{
    HardwareConfiguration hwc(platform_config_file_name);
    hwc.load(instruction_map, compiler_settings, instruction_settings, hardware_settings, resources, topology);
    if (!compiler_config_file_name.empty()) {
        compiler_settings = load_json(compiler_config_file_name);
    }

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
 * Dumps some basic info about the platform to the given stream.
 */
void Platform::dump_info(std::ostream &os, utils::Str line_prefix) const {
    os << line_prefix << "[+] platform name      : " << name << "\n";
    os << line_prefix << "[+] qubit number       : " << qubit_count << "\n";
    os << line_prefix << "[+] creg number        : " << creg_count << "\n";
    os << line_prefix << "[+] breg number        : " << breg_count << "\n";
    os << line_prefix << "[+] configuration file : " << configuration_file_name << "\n";
    os << line_prefix << "[+] architecture       : " << get_architecture() << "\n";
    os << line_prefix << "[+] supported instructions:" << "\n";
    for (const auto &i : instruction_map) {
        os << line_prefix << "  |-- " << i.first << "\n";
    }
}

/**
 * Returns the architecture corresponding to this platform.
 */
arch::Architecture Platform::get_architecture() const {
    if (compiler_settings.type() == Json::value_t::string) {
        return arch::architecture_from_string(compiler_settings.get<utils::Str>());
    } else if (compiler_settings.type() == Json::value_t::object) {
        auto it = compiler_settings.find("strategy");
        if (it == compiler_settings.end() || it->type() != Json::value_t::object) {
            return arch::Architecture::NONE;
        }
        const auto &strategy = *it;
        it = strategy.find("architecture");
        if (it == strategy.end() || it->type() != Json::value_t::string) {
            return arch::Architecture::NONE;
        }
        return arch::architecture_from_string(it->get<utils::Str>());
    }
    return arch::Architecture::NONE;
}

/**
 * Returns the JSON data for a custom gate, throwing a semi-useful
 * exception if the instruction is not found.
 */
const Json &Platform::find_instruction(const Str &iname) const {
    // search the JSON defined instructions, to prevent JSON exception if key does not exist
    if (!QL_JSON_EXISTS(instruction_settings, iname)) {
        QL_FATAL("JSON file: instruction not found: '" << iname << "'");
    }
    return instruction_settings[iname];
}

/**
 * Converts the given time in nanoseconds to cycles.
 */
UInt Platform::time_to_cycles(Real time_ns) const {
    return ceil(time_ns / cycle_time);
}

} // namespace plat
} // namespace ql
