/**
 * @file   platform.h
 * @date   11/2016
 * @author Nader Khammassi
 * @brief  platform header for target-specific compilation
 */
#ifndef PLATFORM_H
#define PLATFORM_H

#include <string>

#include "circuit.h"
#include "hardware_configuration.h"

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
	size_t                  qubit_number;             // number of qubits
         
      public:

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
	   hwc.load(instruction_map, instruction_settings, hardware_settings);
	   if (hardware_settings["qubit_number"].is_null())
	   {
	      println("[x] error : qubit number of the platform is not specified in the configuration file !");
	      throw std::exception();
	   }
	   qubit_number = hardware_settings["qubit_number"];
	}

	/**
	 * display information about the platform
	 */
	void print_info()
	{
	   println("[+] platform name      : " << name);
	   println("[+] qubit number       : " << qubit_number);
	   println("[+] configuration file : " << configuration_file_name);
	   println("[+] supported instructions:");
	   for (ql::instruction_map_t::iterator i=instruction_map.begin(); i!=instruction_map.end(); i++)
	      println("  |-- " << (*i).first);
	}

      // protected:
      public:
        
	std::string             configuration_file_name;  // configuration file name
	ql::instruction_map_t   instruction_map;          // supported operations
	json                    instruction_settings;     // instruction settings (to use by the eqasm backend)
	json                    hardware_settings;        // additional hardware settings (to use by the eqasm backend)

	// std::vector<ql::custom_gate *> supported_instructions; // supported operation 

   };

}

#endif // PLATFORM_H
