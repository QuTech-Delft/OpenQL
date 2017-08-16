
#include <ql/openql.h>
#include <ql/arch/cbox_eqasm_compiler.h>
#include <ql/arch/instruction_scheduler.h>


#define x180(q) kernel.gate("x180",q)
#define y180(q) kernel.gate("x180",q)
#define x90(q) kernel.gate("x180",q)
#define y90(q) kernel.gate("x180",q)

#define prepz(q) kernel.gate("prepz",q)
#define measure(q) kernel.gate("measure",q)

/**
 * main
 */
int main(int argc, char ** argv)
{
   // initialize openql
   ql::init();

   // create platform
   ql::quantum_platform starmon("starmon","test_cfg_cbox.json");

   // print info
   starmon.print_info();

   // set platform
   ql::set_platform(starmon);

   // compiler
   ql::arch::cbox_eqasm_compiler compiler;

   ql::quantum_kernel kernel("kernel",starmon);

   prepz(0);
   measure(0);

   prepz(0);
   x180(0);
   x180(0);
   measure(0);

   prepz(0);
   y180(0);
   y180(0);
   measure(0);

   prepz(0);
   x180(0);
   y180(0);
   measure(0);

   prepz(0);
   y180(0);
   x180(0);
   measure(0);

   prepz(0);
   x90(0);
   measure(0);

   prepz(0);
   y90(0);
   measure(0);

   prepz(0);
   x90(0);
   y90(0);
   measure(0);

   prepz(0);
   y90(0);
   x90(0);
   measure(0);

   prepz(0);
   x90(0);
   y180(0);
   measure(0);

   prepz(0);
   y90(0);
   x180(0);
   measure(0);

   prepz(0);
   x180(0);
   y90(0);
   measure(0);

   prepz(0);
   y180(0);
   x90(0);
   measure(0);

   prepz(0);
   x90(0);
   x180(0);
   measure(0);

   prepz(0);
   x180(0);
   x90(0);
   measure(0);

   prepz(0);
   y90(0);
   y180(0);
   measure(0);

   prepz(0);
   y180(0);
   y90(0);
   measure(0);

   prepz(0);
   x180(0);
   measure(0);


   prepz(0);
   y180(0);
   measure(0);

   prepz(0);
   x90(0);
   x90(0);
   measure(0);

   prepz(0);
   y90(0);
   y90(0);
   measure(0);


   compiler.compile(kernel.get_circuit(), starmon);
   compiler.write_eqasm();
   compiler.write_traces();



   return 0;
}
