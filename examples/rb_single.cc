

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cassert>

#include <time.h>

#include <openql.h>

// clifford inverse lookup table for grounded state
const size_t inv_clifford_lut_gs[] = {0, 2, 1, 3, 8, 10, 6, 11, 4, 9, 5, 7, 12, 16, 23, 21, 13, 17, 18, 19, 20, 15, 22, 14};
//const size_t inv_clifford_lut_es[] = {3, 8, 10, 0, 2, 1, 9, 5, 7, 6, 11, 4, 21, 13, 17, 12, 16, 23, 15, 22, 14, 18, 19, 20};

typedef std::vector<int> cliffords_t;


/**
 * build rb circuit
 */
void build_rb(int num_cliffords, ql::quantum_kernel& k)
{
   assert((num_cliffords%2) == 0);
   int n = num_cliffords/2;

   cliffords_t cl;
   cliffords_t inv_cl;

   // add the clifford and its reverse
   for (int i=0; i<n; ++i)
   {
      int r = rand()%24;
      cl.push_back(r);
      inv_cl.insert(inv_cl.begin(), inv_clifford_lut_gs[r]);
   }
   cl.insert(cl.begin(),inv_cl.begin(),inv_cl.end());

   k.prepz(0);
   // build the circuit
   for (int i=0; i<num_cliffords; ++i)
      k.clifford(cl[i]);
   k.measure(0);

   return;
}


int main(int argc, char ** argv)
{
   srand(0);

   int   num_circuits       = 4;
   float sweep_points[]   = { 2, 4, 8, 16 };  // sizes of the clifford circuits per randomization

   // create platform
   ql::quantum_platform qx_platform("qx_simulator","hardware_config_qx.json");

   // print info
   qx_platform.print_info();

   ql::quantum_program rb("rb", qx_platform, 1);

   rb.set_sweep_points(sweep_points, num_circuits);

   ql::quantum_kernel kernel("rb1024", qx_platform, 1);

   build_rb(1024, kernel);

   rb.add(kernel);
   rb.compile();

   // std::cout<< rb.qasm() << std::endl;
   // std::cout << rb.qasm() << std::endl;

   return 0;
}
