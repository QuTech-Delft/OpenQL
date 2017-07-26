/**
 * @file   eqasm_compiler.h
 * @date   07/2017
 * @author Nader Khammassi
 * @brief  executable qasm compiler interface 
 */

#include "circuit.h"

#ifndef QL_EQASM_COMPILER_H
#define QL_EQASM_COMPILER_H

typedef std::vector<std:string> eqasm_t; 

namespace ql
{
   class eqasm_compiler
   {
      public:

         /*
	  * compile must be implemented by all compilation backends.
	  */
	 virtual eqasm_t compile(ql::circuit& c) = 0;
   };
}

#endif // QL_EQASM_COMPILER_H

