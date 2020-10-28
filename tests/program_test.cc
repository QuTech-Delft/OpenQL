#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cassert>

#include <time.h>

#include <openql.h>

int main(int argc, char ** argv)
{
   size_t nqubits = 5;
   float sweep_points[] = {1};

   // create platform
   ql::quantum_platform qplatform("target_platform", "hardware_config_cc_light.json");

   // print platform info
   qplatform.print_info();

   // create program
   ql::quantum_program prog("prog", qplatform, nqubits);
   prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

   // create a kernel
   ql::quantum_kernel kernel("my_kernel", qplatform, nqubits);

   // add gates to kernel
   kernel.gate("prepz", {0});
   kernel.gate("prepz", {1});
   kernel.gate("x", {0});
   kernel.gate("y", {2});
   kernel.gate("cnot", {0,2});
   kernel.gate("measure", {0});
   kernel.gate("measure", {1});
   kernel.gate("measure", {2});

   // add kernel to prog
   prog.add(kernel);

   // compile the program
   prog.compile();

   return 0;
}

