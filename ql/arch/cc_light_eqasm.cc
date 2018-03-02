

#include <ql/openql.h>

#include <ql/arch/cc_light_eqasm_compiler.h>
// #include <ql/arch/cc_light_eqasm.h>


typedef ql::arch::cc_light_eqasm_instruction  ccl_instr;

typedef ql::arch::cc_light_single_qubit_gate  ccl_sqg_instr;
typedef ql::arch::cc_light_two_qubit_gate     ccl_tqg_instr;

typedef ql::arch::single_qubit_mask           ccl_sqg_mask;
typedef ql::arch::two_qubit_mask             ccl_tqg_mask;

#ifndef __p
#define __p(x,y) ql::arch::qubit_pair_t(x,y)
#endif

int main(int argc, char ** argv)
{

   float  sweep_points[]   = { 1, 2.25, 2.75, 3.25, 3.75 };  // single sweep point with 4 calibration points
   size_t n_swp_pts        = 5;

   // create platform
   ql::quantum_platform seven_qubits_chip("seven_qubits_chip","hardware_config_cc_light.json");

   // print info
   seven_qubits_chip.print_info();

   // return 0;
   size_t qubit_number = seven_qubits_chip.qubit_number;

   // set platform
   ql::set_platform(seven_qubits_chip);

   ql::quantum_program prog("prog", qubit_number, seven_qubits_chip);

   prog.set_sweep_points(sweep_points,n_swp_pts);

   ql::quantum_kernel kernel("kernel",seven_qubits_chip);

   // kernel.prepz(0);
   kernel.gate("x",0);
   kernel.gate("y",0);
   kernel.gate("h",1);
   kernel.gate("cz",0,2);
   kernel.gate("cnot",6,4);

   kernel.gate("measure_all");

   // kernel.measure(0);

   // kernel.gate("measure",0);

   prog.add(kernel);

   prog.compile();

   /* 
   ccl_sqg_instr i0("x",ccl_sqg_mask(0));
   ccl_tqg_instr i1("cz",ccl_tqg_mask(__p(1,3)));

   println(" code : \n" << i0.code());
   println(" code : \n" << i1.code());
   */ 

   return 0;

#undef __p

/*
   // ql::arch::cbox_eqasm_compiler compiler

   size_t qubit_number = seven_qubits_chip.qubit_number;

   ql::quantum_program prog("prog", qubit_number, seven_qubits_chip);

   prog.set_sweep_points(sweep_points,n_swp_pts);

   ql::quantum_kernel kernel("kernel",seven_qubits_chip);

   kernel.prepz(0);
   kernel.gate("rx180",0);
   kernel.gate("ry180",0);
   kernel.gate("rx180",1);
   kernel.gate("cz",0,1);
   // kernel.gate("x180 q0",0);
   // kernel.gate("y90 q0",0);
   kernel.gate("rx90",1);

   kernel.measure(0);

   // kernel.gate("measure",0);

   prog.add(kernel);

   prog.compile();

   prog.schedule("ASAP");

   // compiler.compile(kernel.get_circuit(), seven_qubits_chip);
   // compiler.write_eqasm();
   // compiler.write_traces();

   // kernel.x(0);
   // kernel.y(1);


   return 0;
*/
}
