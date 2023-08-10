#include <openql>

#include <gtest/gtest.h>
#include <iostream>


TEST(v1x_example, cpp_standalone) {
    // create platform
    auto platform = ql::Platform("seven_qubits_chip", "cc_light");

    // create program
    auto program = ql::Program("aProgram", platform, 2);

    // create kernel
    auto kernel = ql::Kernel("aKernel", platform, 2);

    kernel.gate("prepz", 0);
    kernel.gate("prepz", 1);
    kernel.gate("x", 0);
    kernel.gate("y", 1);
    kernel.measure(0);
    kernel.measure(1);

    // add kernel to program
    program.add_kernel(kernel);

    // compile the program
    program.compile();

    std::cout << "Seems good to me!" << std::endl;
}
