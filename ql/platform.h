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
    * abstract platform interface
    */
   class platform
   {
      public:
	 virtual int compile(circuit& c, std::string file_name, bool optimize=false) = 0;
   };
}

#endif // PLATFORM_H
