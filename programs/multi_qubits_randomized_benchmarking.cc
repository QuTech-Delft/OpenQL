

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <cassert>

#include <time.h>

#include "ql/openql.h"

#define println(x) std::cout << x << std::endl

// clifford inverse lookup table for grounded state 
const size_t inv_clifford_lut_gs[] = {0, 2, 1, 3, 8, 10, 6, 11, 4, 9, 5, 7, 12, 16, 23, 21, 13, 17, 18, 19, 20, 15, 22, 14};
// const size_t inv_clifford_lut_es[] = {3, 8, 10, 0, 2, 1, 9, 5, 7, 6, 11, 4, 21, 13, 17, 12, 16, 23, 15, 22, 14, 18, 19, 20};

typedef std::vector<int> cliffords_t;


/**
 * build rb circuit
 */
void build_rb(int num_cliffords, ql::quantum_kernel& k, int qubits=1, bool different=false)
{
   assert((num_cliffords%2) == 0);
   int n = num_cliffords/2;

   if (!different)
   {
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

      // build the circuit
      for (int q=0; q<qubits; q++)
	 k.prepz(q);

      for (int i=0; i<num_cliffords; ++i)
      {
	 for (int q=0; q<qubits; q++)
	 {
	    // println("clifford " << q);
	    k.clifford(cl[i],q);
	 }
      }
      for (int q=0; q<qubits; q++)
	 k.measure(q);
   }
   else 
   {
      // build the circuit
      for (int q=0; q<qubits; q++)
	 k.prepz(q);
      for (int q=0; q<qubits; q++)
      {
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

	 for (int i=0; i<num_cliffords; ++i)
	 {
	    // println("clifford " << q);
	    k.clifford(cl[i],q);
	 }
	 k.measure(q);
      }
   }

   return;
}


int main(int argc, char ** argv)
{	
   srand(clock());

   ql::init(ql::transmon_platform, "instructions.map");

   int   num_qubits = 1; 
   int   num_cliffords = 4096;
   bool  different  = false;

   if (argc == 3)
   {
      num_qubits = atoi(argv[1]);
      different  = (argv[2][0] == 'd'); 
   }

   int   num_circuits       = 1;
   float sweep_points[]     = { 1, 1.25, 1.75, 2.25, 2.75 };  // sizes of the clifford circuits per randomization  

   println("[+] num_qubits    : " << num_qubits);
   println("[+] num_cliffords : " << num_cliffords);
   println("[+] different     : " << (different ? "yes" : "no") );

   // create program 
   ql::str_t prog_name;
   prog_name << "rb_" << num_qubits << "_" << (different ? "diff" : "same");   
   ql::quantum_program rb(prog_name.str(),num_qubits);
   rb.set_sweep_points(sweep_points, num_circuits);
   rb.set_config_file("rb_config.json");

   // create subcircuit
   ql::str_t name;
   name << "rb_" << num_qubits;
   ql::quantum_kernel kernel(name.str());
   build_rb(num_cliffords, kernel, num_qubits, different);
   rb.add(kernel);

   // compile the program
   rb.compile();
   rb.schedule();

   return 0;
}
