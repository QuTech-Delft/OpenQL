#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cassert>

#include <time.h>

#include <ql/openql.h>

int main(int argc, char ** argv)
{
   srand(0);

   // init
   // ql::init();
   // ql::init(ql::transmon_platform, "instructions.map");

   // create platform
   ql::quantum_platform starmon("starmon","test_cfg_cbox.json");

   // print info
   starmon.print_info();

   // set platform
   ql::set_platform(starmon);


   float sweep_points[] = {2};

   // create program
   ql::quantum_program prog("prog",5,starmon);
   prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

   // create a kernel
   ql::quantum_kernel kernel("my_kernel",starmon);

   // add gates to kernel
   kernel.prepz(0);
   kernel.prepz(1);
   kernel.x(0);
   kernel.y(0);
   kernel.cnot(0,1);
   kernel.cnot(0,2);
   kernel.toffoli(0,3,4);  // toffoli test
   kernel.measure(2);

   // add kernel to prog
   prog.add(kernel);

   // compile the program
   prog.compile();

   // schedule program to generate scheduled qasm
   prog.schedule();

   return 0;
}

