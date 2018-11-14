/**
 * @file   hardware_configuration_loader.cc
 * @date   07/2017
 * @author Nader Khammassi
 * @brief  temporary test for hardware configuration
 */

#include <iostream>
#include <fstream>
#include <string>
#include <map>

#include <cmath>

#include <ql/openql.h>
#include <ql/json.h>
#include <ql/gate.h>

using namespace nlohmann;

#ifndef println
#define println(x) std::cout << x << std::endl
#endif

int main(int argc, char ** argv)
{
   ql::quantum_platform transmon("transmon","test_cfg_cbox.json");

   transmon.print_info();

   return 0;
}
