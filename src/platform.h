/**
 * @file   platform.h
 * @date   11/2016
 * @author Nader Khammassi
 * @brief  platform header for target-specific compilation
 */
#ifndef QL_PLATFORM_H
#define QL_PLATFORM_H

#include <string>
#include <tuple>

#include <compile_options.h>
#include <json.h>
#include <instruction_map.h>

namespace ql
{
class quantum_platform
{

public:
    std::string             name;                     // platform name
    std::string             eqasm_compiler_name;      // eqasm backend
    size_t                  qubit_number;             // number of qubits
    size_t                  cycle_time;               // in [ns]
    std::string             configuration_file_name;  // configuration file name
    ql::instruction_map_t   instruction_map;          // supported operations
    json                    instruction_settings;     // instruction settings (to use by the eqasm backend)
    json                    hardware_settings;        // additional hardware settings (to use by the eqasm backend)

    json                    resources;
    json                    topology;
    json                    aliases;                  // workaround the generic instruction composition

//#if OPT_TARGET_PLATFORM   // FIXME: constructed object is not usable
    quantum_platform() : name("default")
    {
    }
//#endif

    quantum_platform(std::string name, std::string configuration_file_name);
    void print_info() const;
    size_t get_qubit_number() const  // FIXME: qubit_number is public anyway
    {
        return qubit_number;
    }


    /**
     * @brief   Find architecture instruction name for a custom gate
     *
     * @param   iname   Name of instruction, e.g. "x q5" ('specialized custom gate') or "x" ('parameterized custom gate')
     * @return  value of 'arch_operation_name', e.g. "x"
     * @note    On CC-light, arch_operation_name is set from JSON field cc_light_instr
     * @note    Based on cc_light_scheduler.h::get_cc_light_instruction_name()
     * @note    Only works for custom instructions defined in JSON
       FIXME:   it may be more useful to get the information directly from JSON, because arch_operation_name is not really generic
     */

    // find settings for custom gate, preventing JSON exceptions
    const json& find_instruction(std::string iname) const;

    // find instruction type for custom gate
    std::string find_instruction_type(std::string iname) const;

    size_t time_to_cycles(float time_ns) const;
};

}

#endif // QL_PLATFORM_H
