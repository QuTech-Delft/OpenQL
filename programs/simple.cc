#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cassert>

#include <time.h>

#include "ql/openql.h"

int main(int argc, char ** argv)
{	
   srand(0);

   int   num_circuits       = 13;
   float sweep_points[]     = { 2, 4, 8, 16, 32, 64, 128, 256, 512, 512.25, 512.75, 513.25, 513.75 };  // sizes of the clifford circuits per randomization  

   ql::init();
   // ql::init(ql::transmon_platform, "instructions.map");

   // create platform
   ql::quantum_platform starmon("starmon","hardware_config_cbox.json");

   // print info
   starmon.print_info();

   // set platform
   ql::set_platform(starmon);

   // create program 
   ql::quantum_program prog("prog",1);
   prog.set_sweep_points(sweep_points, num_circuits);

   for (int j=0; j<1; j++)
   {
      int c_size = sweep_points[j];
      // create subcircuit
      ql::str_t name;
      name << "kernel" << c_size;
      ql::quantum_kernel kernel(name.str(),starmon);
      kernel.prepz(0);
      kernel.hadamard(0);
      kernel.x(0);
      kernel.y(0);
      kernel.z(0);
      kernel.hadamard(0);
      kernel.x(0);
      kernel.measure(0);
      prog.add(kernel);
   }

   // compile the program
   prog.compile();

   return 0;
}
