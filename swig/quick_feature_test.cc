#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cassert>

#include <time.h>

// enable optimizations
// #define ql_optimize

#include <openql.h>

int main(int argc, char ** argv)
{
   srand(0);

   int   num_circuits       = 1;
   float sweep_points[]     = { 1, 1.25, 1.75, 2.25, 2.75 };


   // create platform
   ql::quantum_platform platf("seven_qubits_chip","../tests/hardware_config_cc_light.json");

   // print info
   // platf.print_info();

   // set platform
   ql::set_platform(platf);

   // create program
   ql::quantum_program prog("aProgram", 7, platf);
   prog.set_sweep_points(sweep_points, num_circuits);

   ql::quantum_kernel k("aKernel",platf);

   // print user-defined instructions (qasm/microcode)
   // k.print_gates_definition();

  // create kernel
   k.prepz(0);
   k.x(0);
   k.gate("rx90", 0);  // custom gate
   k.measure(0);
   prog.add(k);

   // compile the program
   prog.compile( /*optimize*/ false, /*verbose*/ true );

   // print qasm
   println(prog.qasm());

   // print micro code
   // println(prog.microcode());

   return 0;
}
