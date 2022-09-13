#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cassert>

#include <time.h>

#include <openql>

int main(int argc, char **argv) {
   size_t nqubits = 5;

   // create platform
   auto qplatform = ql::Platform("target_platform", "cc_light");

   // create program
   auto prog = ql::Program("prog", qplatform, nqubits);

   // create a kernel
   auto kernel = ql::Kernel("my_kernel", qplatform, nqubits);

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
   prog.add_kernel(kernel);

   // compile the program
   prog.compile();

   return 0;
}

