#include <iostream>
#include <openql>

int main(int argc, char ** argv)
{
  // create platform
  auto platf = ql::Platform("seven_qubits_chip", "hardware_config_cc_light.json");

  // create program
  auto prog = ql::Program("aProgram", platf, 2);

  // create kernel
  auto k = ql::Kernel("aKernel", platf, 2);

  k.gate("prepz", 0);
  k.gate("prepz", 1);
  k.gate("x", 0);
  k.gate("y", 1);
  k.measure(0);
  k.measure(1);

  // add kernel to program
  prog.add_kernel(k);

  // compile the program
  prog.compile();

  std::cout << "Seems good to me!" << std::endl;
  return 0;
}
