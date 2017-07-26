/**
 * @file   qumis.h
 * @date   07/2017
 * @author Nader Khammassi
 * @brief  qumis code emitter
 */

#ifndef QL_QUMIS_H
#define QL_QUMIS_H

#include <string>

namespace ql
{
   namespace arch
   {
      // single qumis instruction
      typedef std::string qumis_instr_t;

      /**
       * qumis instruction interface  
       */
      class qumis_instruction
      {
	 public:

	    // emit associated code 
	    virtual qumis_instr_t code() = 0;
      };
}

#endif // QL_QUMIS_H





