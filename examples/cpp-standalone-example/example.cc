#include <iostream>
#include <openql_i.h>

int main(int argc, char ** argv)
{
  // create platform
  ql::quantum_platform platf("seven_qubits_chip", "hardware_config_cc_light.json");

  // create program
  ql::quantum_program prog("aProgram", platf, 2);

  // create kernel
  ql::quantum_kernel k("aKernel", platf, 2);

  k.gate("prepz", 0);
  k.gate("prepz", 1);
  k.gate("x", 0);
  k.gate("y", 1);
  k.measure(0);
  k.measure(1);

  // add kernel to program
  prog.add(k);

  // compile the program
  prog.compile();

  std::cout << "Seems good to me!" << std::endl;
  return 0;
}
