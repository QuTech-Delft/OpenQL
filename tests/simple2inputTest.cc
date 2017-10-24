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

   float sweep_points[]     = { 2 };  // sizes of the clifford circuits per randomization

   // ql::init(ql::transmon_platform, "instructions.map");

   // create program
   ql::quantum_program prog("prog", 2);
   prog.set_sweep_points(sweep_points, sizeof(sweep_points)/sizeof(float));

   for (int j=0; j<1; j++)
   {
      int c_size = sweep_points[j];
      // create subcircuit
      ql::str_t name;
      name << "kernel" << c_size;
      ql::quantum_kernel kernel(name.str());
      kernel.prepz(0);
      kernel.prepz(1);
      kernel.x(0);
      kernel.y(1);
      kernel.hadamard(0);
      kernel.cnot(0,1);
      kernel.measure(1);
      prog.add(kernel);
   }

   // compile the program
   prog.compile( /*verbose*/ 1 );

   println(prog.qasm());

   return 0;
}
