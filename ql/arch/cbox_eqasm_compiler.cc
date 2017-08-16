

#include <ql/openql.h>
#include <ql/arch/cbox_eqasm_compiler.h>
#include <ql/arch/instruction_scheduler.h>



int main(int argc, char ** argv)
{

   float  sweep_points[]   = { 1, 2.25, 2.75, 3.25, 3.75 };  // single sweep point with 4 calibration points
   size_t n_swp_pts        = 5;

   // initialize openql
   ql::init();

   // create platform
   ql::quantum_platform starmon("starmon","test_cfg_cbox.json");

   // print info
   starmon.print_info();

   // set platform
   ql::set_platform(starmon);

   // ql::arch::cbox_eqasm_compiler compiler

   size_t qubit_number = starmon.qubit_number;

   ql::quantum_program prog("prog", qubit_number, starmon);

   prog.set_sweep_points(sweep_points,n_swp_pts);

   ql::quantum_kernel kernel("kernel",starmon);

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

   // compiler.compile(kernel.get_circuit(), starmon);
   // compiler.write_eqasm();
   // compiler.write_traces();

   // kernel.x(0);
   // kernel.y(1);

   /*
   println(ql::pauli_x(0).qasm());
   println(ql::custom_gate("x180 q0").qasm());
   println(ql::custom_gate("y180 q0").qasm());
   println(ql::custom_gate("y90  q0").qasm());
   */



   /*
   ql::arch::channels_t channels;
   channels.push_back("AWG_0");
   channels.push_back("AWG_1");
   channels.push_back("AWG_2");
   channels.push_back("TRIG_0");
   channels.push_back("TRIG_1");
   channels.push_back("TRIG_2");
   channels.push_back("TRIG_3");
   channels.push_back("TRIG_5");
   channels.push_back("TRIG_6");
   channels.push_back("TRIG_7");

   ql::arch::time_diagram diagram(channels,100,5);
   diagram.add_trace(0,0,20 ,"pulse 0001,0000,0000");
   diagram.add_trace(2,0,20 ,"pulse 0000,0000,0110");
   diagram.add_trace(1,20,40,"pulse 0000,0101,0000");
   diagram.add_trace(4,30,50,"trigger 00110000,4");
   diagram.add_trace(5,30,50,"trigger 00110000,4");
   diagram.add_trace(6,35,40,"trigger 01000000,3");

   diagram.dump();
   */

   return 0;
}
