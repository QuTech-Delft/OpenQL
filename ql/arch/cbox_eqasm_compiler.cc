

#include <ql/openql.h>
#include <ql/arch/cbox_eqasm_compiler.h>
#include <ql/arch/instruction_scheduler.h>


int main(int argc, char ** argv)
{
   // initialize openql
   ql::init();

   // create platform
   ql::quantum_platform starmon("starmon","hardware_config_cbox.json");

   // print info
   starmon.print_info();

   // set platform
   ql::set_platform(starmon);


   ql::arch::cbox_eqasm_compiler compiler;

   ql::quantum_kernel kernel("kernel",starmon);

   kernel.gate("x180 q0",0);
   // kernel.gate("y180 q0",0);
   // kernel.gate("x180 q1",1);
   // kernel.gate("x180 q0",0);
   // kernel.gate("y90 q0",0);
   // kernel.gate("x90 q1",1);
   // kernel.gate("x180 q0",0);
   // kernel.gate("y180 q0",0);
   // kernel.gate("x180 q1",1);
   kernel.gate("measure q0",0);

   // kernel.x(0);
   // kernel.y(1);

   /*
   println(ql::pauli_x(0).qasm());
   println(ql::custom_gate("x180 q0").qasm());
   println(ql::custom_gate("y180 q0").qasm());
   println(ql::custom_gate("y90  q0").qasm());
   */

   compiler.compile(kernel.get_circuit(), starmon);
   compiler.write_eqasm();
   compiler.write_traces();

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
