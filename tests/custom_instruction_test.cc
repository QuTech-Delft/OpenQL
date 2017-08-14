/**
 * @file   custom_instruction_test.cc
 * @date   04/2017
 * @author Nader Khammassi
 * @brief  custom instruction test
 */

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

#include <ql/openql.h>

int main(int argc, char ** argv)
{
   srand(0);

   int   num_circuits       = 1;
   float sweep_points[]     = { 1, 1.25, 1.75, 2.25, 2.75 };

   ql::init();
   // ql::init(ql::transmon_platform, "instructions.map");

   // create platform
   ql::quantum_platform starmon("starmon","hardware_config_cbox.json");

   // print info
   starmon.print_info();

   // set platform
   ql::set_platform(starmon);

   // create program
   ql::quantum_program prog("prog", 2, starmon);
   prog.set_sweep_points(sweep_points, num_circuits);

   ql::quantum_kernel k("custom_gate_test",starmon);

   // deprecated !
   // load custom instruction definition
   // k.load_custom_instructions();

   // print user-defined instructions (qasm/microcode)
   k.print_gates_definition();

  // create kernel
   k.prepz(0);
   k.x(0);
   k.gate("rx180", 0);  // custom gate
   k.measure(0);
   prog.add(k);

   // compile the program
   prog.compile( /*verbose*/ 1 );

   // print qasm
   println(prog.qasm());

   // print micro code
   println(prog.microcode());

   return 0;
}
