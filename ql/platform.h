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

#include <ql/circuit.h>
#include <ql/hardware_configuration.h>

namespace ql
{
typedef enum __ql_platform_t
{
    transmon_platform,
    starmon_platform,
    qx_simulator_platform,
    unsupported_platform
} ql_platform_t;

typedef std::vector<std::string> micro_code_t;


#if 0   // FIXME: unused
/**
 * abstract platform interface (deprecated)
 * should be removed soon
 */
class platform
{
public:
    virtual int compile(circuit& c, std::string file_name, bool optimize=false) = 0;
};
#endif


/**
 * quantum platform
 */
class quantum_platform
{

public:

    std::string             name;                     // platform name
    std::string             eqasm_compiler_name;      // eqasm backend
    size_t                  qubit_number;             // number of qubits
    size_t                  cycle_time;
    std::string             configuration_file_name;  // configuration file name
    ql::instruction_map_t   instruction_map;          // supported operations
    json                    instruction_settings;     // instruction settings (to use by the eqasm backend)
    json                    hardware_settings;        // additional hardware settings (to use by the eqasm backend)
    json                    resources;
    json                    topology;
    json                    aliases;                  // workaround the generic instruction composition

    /**
     * quantum_platform constructor
     */
    quantum_platform() : name("default")
    {
    }

    /**
     * quantum_platform constructor
     */
    quantum_platform(std::string name, std::string configuration_file_name) : name(name),
        configuration_file_name(configuration_file_name)
    {
        ql::hardware_configuration hwc(configuration_file_name);
        // hwc.load(instruction_map, instruction_settings, hardware_settings, resources, topology);
        hwc.load(instruction_map, instruction_settings, hardware_settings, resources, topology, aliases);
        eqasm_compiler_name = hwc.eqasm_compiler_name;

        if(hardware_settings.count("qubit_number") <=0)
        {
            EOUT("qubit number of the platform is not specified in the configuration file !");
            throw std::exception();
        }
        else
            qubit_number = hardware_settings["qubit_number"];

        if(hardware_settings.count("cycle_time") <=0)
        {
            EOUT("cycle time of the platform is not specified in the configuration file !");
            throw std::exception();
        }
        else
            cycle_time = hardware_settings["cycle_time"];
    }

    /**
     * display information about the platform
     */
    void print_info()
    {
        println("[+] platform name      : " << name);
        println("[+] qubit number       : " << qubit_number);
        println("[+] eqasm compiler     : " << eqasm_compiler_name);
        println("[+] configuration file : " << configuration_file_name);
        println("[+] supported instructions:");
        for (ql::instruction_map_t::iterator i=instruction_map.begin(); i!=instruction_map.end(); i++)
            println("  |-- " << (*i).first);
    }

    size_t get_qubit_number()
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
    std::string get_instruction_name(std::string &iname) const
    {
        std::string instr_name;
        auto it = instruction_map.find(iname);
        if (it != instruction_map.end())
        {
            custom_gate* g = it->second;
            instr_name = g->arch_operation_name;
            if(instr_name.empty())
            {
                FATAL("arch_operation_name not defined for instruction: " << iname << " !");
            }
            // DOUT("arch_operation_name: " << instr_name);
        }
        else
        {
            FATAL("custom instruction not found for : " << iname << " !");
        }
        return instr_name;
    }


    // find settings for custom gate, preventing JSON exceptions
    const json& find_instruction(std::string iname) const
    {
        // search the JSON defined instructions, to prevent JSON exception if key does not exist
        if (instruction_settings.find(iname) == instruction_settings.end())
        {
            FATAL("instruction settings not found for '" << iname << "'!");
        }

        return instruction_settings[iname];
    }


    // find instruction type for custom gate
    std::string find_instruction_type(std::string iname) const
    {
        const json &instruction = find_instruction(iname);
        return instruction["type"];
    }
};

}

#endif // QL_PLATFORM_H
