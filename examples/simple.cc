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
   srand(0);

   // number of circuits
   int num_ckts = 13;

   // sizes of the clifford circuits per randomization
   float sweep_points[] = {2, 4, 8, 16, 32, 64, 128, 256, 512, 512.25, 512.75, 513.25, 513.75 };

   // create platform
   ql::quantum_platform my_platform("target_platform","hardware_config_cc_light.json");
   // ql::quantum_platform my_platform("target_platform","test_cfg_cbox.json");

   // print info
   my_platform.print_info();

   // set platform
   ql::set_platform(my_platform);

   // create program
   ql::quantum_program prog("prog", my_platform, 1);
   prog.set_sweep_points(sweep_points, num_ckts);

   for (int j=0; j<1; j++)
   {
      int c_size = sweep_points[j];
      // create subcircuit
      ql::str_t name;
      name << "kernel" << c_size;
      ql::quantum_kernel kernel(name.str(),my_platform, 1);

      // populate kernel
      kernel.gate("prepz", {0});
      kernel.gate("x", {0});
      kernel.gate("y", {0});
      kernel.gate("z", {0});
      kernel.gate("measure", {0});

      // add kernel to program
      prog.add(kernel);
   }

   // compile the program
   prog.compile();

   return 0;
}
