#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cassert>

#include <time.h>

#include <openql>

int main() {
   size_t nqubits = 5;

   // create platform
   auto qplatform = ql::Platform("target_platform", "cc_light");

   // create program
   auto prog = ql::Program("prog", qplatform, nqubits);

   // create a kernel
   auto kernel = ql::Kernel("my_kernel", qplatform, nqubits);

   // add gates to kernel
   kernel.gate("prepz", std::vector<size_t>{0});
   kernel.gate("prepz", std::vector<size_t>{1});
   kernel.gate("x", std::vector<size_t>{0});
   kernel.gate("y", std::vector<size_t>{2});
   kernel.gate("cnot", std::vector<size_t>{0, 2});
   kernel.gate("measure", std::vector<size_t>{0});
   kernel.gate("measure", std::vector<size_t>{1});
   kernel.gate("measure", std::vector<size_t>{2});

   // add kernel to prog
   prog.add_kernel(kernel);

   // compile the program
   prog.compile();

   return 0;
}

