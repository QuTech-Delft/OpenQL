/**
 * @file   eqasm_compiler.h
 * @date   07/2017
 * @author Nader Khammassi
 * @brief  executable qasm compiler interface 
 */

#include "circuit.h"

#ifndef QL_EQASM_COMPILER_H
#define QL_EQASM_COMPILER_H

typedef std::vector<std::string> eqasm_t; 

namespace ql
{
   /**
    * eqasm compiler interface
    */
   class eqasm_compiler
   {
      public:

         eqasm_t eqasm_code;

      public:

         /*
	  * compile must be implemented by all compilation backends.
	  */
	 virtual void compile(ql::circuit& c, ql::quantum_platform& p) = 0;

	 /**
	  * write eqasm code to file/stdout
	  */
	 virtual void write_eqasm(std::string file_name="")
	 {
	    if (eqasm_code.empty())
	       return;
	    if (file_name=="")
	    {
	       println("[c] eqasm code (" << eqasm_code.size() << " lines) :");
	       for (std::string l : eqasm_code)
		  std::cout << l << std::endl;
	    }
	    else
	    {
	       // write to file
	    }
	 }
   };
}

#endif // QL_EQASM_COMPILER_H

