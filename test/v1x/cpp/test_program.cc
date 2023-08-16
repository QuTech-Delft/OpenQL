#include <openql>

#include <gtest/gtest.h>
#include <vector>


TEST(v1x, test_program) {
   size_t num_qubits = 5;

   // create platform
   auto qplatform = ql::Platform("target_platform", "cc_light");

   // create program
   auto prog = ql::Program("prog", qplatform, num_qubits);

   // create a kernel
   auto kernel = ql::Kernel("my_kernel", qplatform, num_qubits);

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
}
