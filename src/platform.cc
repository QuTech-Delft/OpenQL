/**
 * @file   platform.cc
 * @date   11/2016
 * @author Nader Khammassi
 * @brief  platform header for target-specific compilation
 */

#include "platform.h"

#include <hardware_configuration.h>
#include <gate.h>

// FIXME: global variables: not the best of places, but this currently is our sole .cc file
#include <utils.h>
#include <options.h>

namespace ql { namespace utils { namespace logger {
    log_level_t LOG_LEVEL;
}}}
namespace ql { namespace options {
    ql::Options ql_options("OpenQL Options");
}}

namespace ql
{

quantum_platform::quantum_platform(std::string name, std::string configuration_file_name) : name(name),
    configuration_file_name(configuration_file_name)
{
    ql::hardware_configuration hwc(configuration_file_name);
    hwc.load(instruction_map, instruction_settings, hardware_settings, resources, topology, aliases);
    eqasm_compiler_name = hwc.eqasm_compiler_name;
    DOUT("eqasm_compiler_name= " << eqasm_compiler_name);

    if(hardware_settings.count("qubit_number") <= 0)
    {
        FATAL("qubit number of the platform is not specified in the configuration file !");
    }
    else
        qubit_number = hardware_settings["qubit_number"];

    // FIXME: add creg_count to JSNN file and platform

    if(hardware_settings.count("cycle_time") <= 0)
    {
        FATAL("cycle time of the platform is not specified in the configuration file !");
    }
    else
        cycle_time = hardware_settings["cycle_time"];
}

/**
 * display information about the platform
 */
void quantum_platform::print_info() const
{
    println("[+] platform name      : " << name);
    println("[+] qubit number       : " << qubit_number);
    println("[+] eqasm compiler     : " << eqasm_compiler_name);
    println("[+] configuration file : " << configuration_file_name);
    println("[+] supported instructions:");
    for (ql::instruction_map_t::const_iterator i=instruction_map.begin(); i!=instruction_map.end(); i++)
        println("  |-- " << (*i).first);
}

// find settings for custom gate, preventing JSON exceptions
const json& quantum_platform::find_instruction(std::string iname) const
{
    // search the JSON defined instructions, to prevent JSON exception if key does not exist
    if(!JSON_EXISTS(instruction_settings, iname)) FATAL("JSON file: instruction not found: '" << iname << "'");
    return instruction_settings[iname];
}


// find instruction type for custom gate
std::string quantum_platform::find_instruction_type(std::string iname) const
{
    const json &instruction = find_instruction(iname);
    if(!JSON_EXISTS(instruction, "type")) FATAL("JSON file: field 'type' not defined for instruction '" << iname <<"'");
    return instruction["type"];
}

size_t quantum_platform::time_to_cycles(float time_ns) const
{
    return std::ceil(time_ns/cycle_time);
}


}   // namespace
