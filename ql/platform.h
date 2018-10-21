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
// #include <ql/eqasm_compiler.h>
// #include <ql/arch/cbox_eqasm_compiler.h>

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

/**
 * abstract platform interface (deprecated)
 * should be removed soon
 */
class platform
{
public:
    virtual int compile(circuit& c, std::string file_name, bool optimize=false) = 0;
};


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

    // ql::eqasm_compiler *      backend_compiler;         // backend compiler
    // std::vector<ql::custom_gate *> supported_instructions; // supported operation

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

        // if (eqasm_compiler_name == "qumis_compiler")
        // {
        //    backend_compiler = new ql::arch::cbox_eqasm_compiler();
        // }
        // else if (eqasm_compiler_name == "none")
        // {
        //    backend_compiler = NULL;
        // }
        // else
        // {
        //    EOUT("eqasm compiler backend specified in the hardware configuration file is not supported !");
        //    throw std::exception();
        // }
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
};

}

#endif // PLATFORM_H
