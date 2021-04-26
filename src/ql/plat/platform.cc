/** \file
 * Platform header for target-specific compilation.
 */

#include "ql/plat/platform.h"

#include "ql/arch/factory.h"

namespace ql {
namespace plat {

using namespace utils;

/**
 * Loads the platform members from the given JSON data and optional
 * auxiliary compiler configuration file.
 */
void Platform::load(
    const utils::Json &platform_config,
    const utils::Str &compiler_config
) {
    HardwareConfiguration hwc(platform_config);
    utils::Json topology_json;
    hwc.load(
        instruction_map,
        architecture,
        compiler_settings,
        instruction_settings,
        hardware_settings,
        resources,
        topology_json
    );
    if (!compiler_config.empty()) {
        compiler_settings = load_json(compiler_config);
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

    topology.emplace(qubit_count, topology_json);
}

/**
 * Constructs a platform from the given configuration filename.
 */
Platform::Platform(
    const Str &name,
    const Str &platform_config,
    const Str &compiler_config
) : name(name) {

    arch::Factory arch_factory = {};

    // If the configuration filename itself is a recognized architecture name,
    // query the default configuration for that architecture. Otherwise
    // interpret it as a filename, which it's historically always been.
    Json config;
    architecture = arch_factory.build_from_namespace(platform_config);
    if (architecture.has_value()) {
        std::istringstream is{architecture->get_default_platform()};
        config = parse_json(is);
    } else {
        try {
            config = load_json(platform_config);
        } catch (Json::exception &e) {
            QL_FATAL(
                "failed to load the hardware config file : malformed json file: \n\t"
                    << Str(e.what()));
        }
    }

    load(config, compiler_config);
}

/**
 * Constructs a platform from the given configuration *data*. The dummy
 * argument only serves to differentiate from the previous constructor.
 */
Platform::Platform(
    const utils::Str &name,
    const utils::Json &platform_config,
    const Str &compiler_config
) : name(name) {
    load(platform_config, compiler_config);
}

/**
 * Dumps some basic info about the platform to the given stream.
 */
void Platform::dump_info(std::ostream &os, utils::Str line_prefix) const {
    os << line_prefix << "[+] platform name      : " << name << "\n";
    os << line_prefix << "[+] qubit number       : " << qubit_count << "\n";
    os << line_prefix << "[+] creg number        : " << creg_count << "\n";
    os << line_prefix << "[+] breg number        : " << breg_count << "\n";
    os << line_prefix << "[+] architecture       : " << architecture->get_friendly_name() << "\n";
    os << line_prefix << "[+] supported instructions:" << "\n";
    for (const auto &i : instruction_map) {
        os << line_prefix << "  |-- " << i.first << "\n";
    }
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
