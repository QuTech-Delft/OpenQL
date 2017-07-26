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
       
        /**
	 * quantum_platform constructor
	 */
	quantum_platform(std::string name, std::string configuration_file_name) : name(name), 
	                                                                          configuration_file_name(configuration_file_name)
	{
	   ql::hardware_configuration hwc(configuration_file_name);
	   hwc.load(supported_instructions, hardware_settings);
	}

	/**
	 * display information about the platform
	 */
	void print_info()
	{
	   println("[+] platform name      : " << name);
	   println("[+] configuration file : " << configuration_file_name);
	   println("[+] supported instructions:");
	   for (auto g : supported_instructions)
	      println(g->qasm());
	}

      protected:
        
	std::string  name;
	std::string  configuration_file_name;

	std::vector<ql::custom_gate *> supported_instructions; // supported operation 
	json                           hardware_settings;      // additional hardware settings (to use by the eqasm backend)
   };

}

#endif // PLATFORM_H
