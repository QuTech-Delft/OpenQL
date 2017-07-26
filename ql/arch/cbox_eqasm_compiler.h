/**
 * @file   cbox_eqasm_compiler.h
 * @date   07/2017
 * @author Nader Khammassi
 * @brief  cbox eqasm compiler implementation 
 */

#ifndef QL_CBOX_EQASM_COMPILER_H
#define QL_CBOX_EQASM_COMPILER_H

#include "<ql/eqasm_compiler>"
#include <ql/arch/qumis.h>

// eqasm code : set of qumis instructions
typedef std::vector<ql::arch::qumis_instr_t> eqasm_t; 

namespace ql
{
   namespace arch
   {
      /**
       * cbox eqasm compiler
       */
      class cbox_eqasm_compiler : public eqasm_compiler
      {
	 public:

	    /*
	     * compile qasm to qumis
	     */
	    eqasm_t compile(ql::circuit& c)
	    {
	    }
      };
}

#endif // QL_CBOX_EQASM_COMPILER_H

