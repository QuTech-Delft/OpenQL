

#include <ql/openql.h>
#include <ql/arch/cbox_eqasm_compiler.h>


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
   kernel.gate("y180 q0",0);
   kernel.gate("x180 q1",1);
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


   return 0;
}
