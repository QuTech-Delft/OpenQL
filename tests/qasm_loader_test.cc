
#include <ql/qasm_loader.h>



int main(int argc, char ** argv)
{
   // qx::qasm_loader code("test.qasm");
   if (argc == 2)
   {
      qx::qasm_loader code(argv[1]);
      code.parse();
   }
   else
   {
      qx::qasm_loader code("test.qasm");
      code.parse();
   }

   return 0;
}
