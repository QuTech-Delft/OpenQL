/**
 * @file    qasm_loader_test.cc
 * @author	Nader Khammassi 
 * @date	   20-12-15
 * @brief	qasm loader test
 */


#include <ql/qasm_loader.h>


/**
 * load a qasm file : syntax and semantic check
 *   @arg file_name
 * 
 */
int main(int argc, char ** argv)
{
   if (argc == 2)
   {
      qx::qasm_loader code(argv[1]);
      code.parse();
   }
   else
   {
      println("[x] qasm file not specified !\n[?] usage : " << argv[0] << " file.qasm");
      return -1;
   }

   return 0;
}
