/**
 * @file   quantum_platform_test.cc
 * @date   07/2017
 * @author Nader Khammassi
 * @brief  test for quantum_platform creation.
 */

#include <iostream>
#include <fstream>
#include <string>
#include <map>

#include <cmath>

#include <src/openql.h>
#include <src/json.h>
#include <src/gate.h>

using namespace nlohmann;

#ifndef println
#define println(x) std::cout << x << std::endl
#endif

int main(int argc, char ** argv)
{
   // create platform
   ql::quantum_platform transmon("transmon","test_cfg_cbox.json");

   // print info
   transmon.print_info();

   return 0;
}
