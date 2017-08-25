/**
 * @file   cc_light_eqasm.h
 * @date   07/2017
 * @author Nader Khammassi
 * @brief  cc_light_eqasm code emitter
 */

#ifndef QL_CC_LIGHT_EQASM_H
#define QL_CC_LIGHT_EQASM_H

#include <string>
#include <sstream>
#include <bitset>

// #include <ql/arch/instruction_scheduler.h>

namespace ql
{
   namespace arch
   {
      typedef bool                 bit_t;
      typedef std::string          cc_light_eqasm_instr_t;
      typedef std::vector<bit_t>   bitset_t;

      class cc_light_eqasm_instruction;

      typedef std::vector<cc_light_eqasm_instruction *> cc_light_eqasm_program_t;

      // instruction type
      typedef enum 
      {
	 // classical instruction

	 __cc_light_eqasm_add__      ,
	 __cc_light_eqasm_sub__      ,
	 __cc_light_eqasm_and__      ,
	 __cc_light_eqasm_or__       ,
	 __cc_light_eqasm_xor_       ,
	 __cc_light_eqasm_not__      ,

	 __cc_light_eqasm_cmp__      ,
	 __cc_light_eqasm_br__       ,
	 __cc_light_eqasm_fbr__      ,
	 __cc_light_eqasm_fmr__      ,

	 __cc_light_eqasm_ldi__      ,
	 __cc_light_eqasm_ldui__     ,

	 __cc_light_eqasm_smis__     ,
	 __cc_light_eqasm_smit__     ,

	 // quantum instruction

	 __cc_light_eqasm_i__        ,
	 __cc_light_eqasm_x__        ,
	 __cc_light_eqasm_y__        ,
	 __cc_light_eqasm_z__        ,
	 __cc_light_eqasm_h__        ,

	 __cc_light_eqasm_x90__      ,
	 __cc_light_eqasm_mx90__     ,
	 __cc_light_eqasm_y90__      ,
	 __cc_light_eqasm_my90__     ,

	 __cc_light_eqasm_s__        ,
	 __cc_light_eqasm_sdag__     ,
	 __cc_light_eqasm_t__        ,
	 __cc_light_eqasm_tdag__     ,

	 __cc_light_eqasm_prepz__    ,
	 __cc_light_eqasm_prepx__    ,

	 __cc_light_eqasm_measurex__ ,
	 __cc_light_eqasm_measurez__ ,

	 __cc_light_eqasm_cnot__     ,
	 __cc_light_eqasm_cz__       ,
	 __cc_light_eqasm_swap__     ,

	 __cc_light_eqasm_qwait__    ,
	 __cc_light_eqasm_qwaitr__   ,
	 __cc_light_eqasm_qnop__

      } cc_light_eqasm_instr_type_t;

      // operation type
      // typedef enum 
      // {
      //    __none__       ,
      //    __rf__         ,
      //    __flux__       ,
      //    __measurement__,
      //    __wait__       ,
      //    __unknown_operation__  
      // } operation_type_t;

      // #define __operation_types_num__ (4)

      // hardware resources
      typedef std::vector<size_t>        qubit_set_t;
      typedef std::pair<size_t,size_t>   qubit_pair_t;
      typedef std::vector<qubit_pair_t>  qubit_pair_set_t;
      typedef std::string                mask_t;


      /**
       * qubit mask
       */
      class single_qubit_mask
      {
	 public:
	    
	    qubit_set_t qs;

	    /**
	     * ctor 
	     */
	    single_qubit_mask(qubit_set_t& qs) : qs(qs)
	    {
	    }

	    /**
	     * ctor 
	     */
	    single_qubit_mask(size_t qubit) 
	    {
	       qs.push_back(qubit);
	    }

	    /**
	     * get mask
	     */
	    mask_t get_mask(size_t reg)
	    {
	       size_t n=qs.size();
	       std::stringstream m;
	       // to do : make sure there is no duplicate qubits
	       m << "smis s" << reg << ", { ";
	       if (n == 1)
	       {
		  m << qs[0] << " }"; 
	       }
	       else
	       {
		  size_t i=0;
		  while (i++ < n)
		     m << qs[i] << ",";
		  m << qs[i] << "}"; 
	       }
	       return m.str();
	    }
      };


      /**
       * two qubits mask
       */
      class two_qubits_mask
      {
	 public:
	    
	    qubit_pair_set_t qs;

	    /**
	     * ctor mask
	     */
	    two_qubits_mask(qubit_pair_set_t& qs) : qs(qs)
	    {
	    }

	    /**
	     * ctor mask
	     */
	    two_qubits_mask(qubit_pair_t p) 
	    {
	       qs.push_back(p);
	    }

	    /**
	     * get mask
	     */
	    mask_t get_mask(size_t reg)
	    {
	       size_t n=qs.size();
	       std::stringstream m;
	       // to do : make sure reg is not reserved
	       // to do : make sure there is no duplicate qubits
	       m << "smit t" << reg << ", { ";
	       if (n == 1)
	       {
		  m << "(" << qs[0].first << "," << qs[0].second <<  ") }";
	       }
	       else
	       {
		  size_t i=0;
		  while (i++ < n)
		     m << "(" << qs[i].first << "," << qs[i].second <<  "),";
		  m << "(" << qs[i].first << "," << qs[i].second << ") }"; 
	       }
	       return m.str();
	    }
      };

      /**
       * cc_light_eqasm instruction interface  
       */
      class cc_light_eqasm_instruction
      {
	 public:
	    
	    qubit_set_t            used_qubits;

	    size_t                 duration;
	    size_t                 latency = 0; 
	    size_t                 start = 0;
	    size_t                 codeword = 0;
	    size_t                 opcode = 0;
	    size_t                 condition = 0;

	    operation_type_t            operation_type;
	    cc_light_eqasm_instr_type_t instr_type; 

	    std::string            qasm_label;

	    bool                   latency_compensated = false;

	    std::string            name;
	    
	 public:


	    /**
	     * emit cc_light_eqasm code 
	     */
	    virtual cc_light_eqasm_instr_t code() = 0;

	    /**
	     * return instruction trace
	     */
	    // virtual instruction_traces_t trace() = 0;

	    /**
	     *  compensate for latency
	     */
	    virtual void compensate_latency()
	    {
	       if (!latency_compensated)
	       {
		  // println("compensate latency : " << start << " -> " << (start-latency) << " : latency = " << latency);
		  start -= latency;
		  latency_compensated = true;
	       }
	       else
	       {
		  println("[x] warning : latency of instruction '" << this << "' is already compensated !");
	       }
	    }    


	    /**
	     * set start
	     */
	    virtual void set_start(size_t t)
	    {
	       start = t;
	    }

	    /**
	     * decompose meta-instructions
	     */
	    virtual cc_light_eqasm_program_t decompose()
	    {
	       cc_light_eqasm_program_t p;
	       p.push_back(this);
	       return p;
	    }
	    
	    /**
	     * return cc_light_eqasm instruction type
	     */
	    virtual cc_light_eqasm_instr_type_t get_instruction_type()
	    {
	       return instr_type; 
	    }

	    /**
	     * return operation type
	     */
	    virtual operation_type_t get_operation_type()
	    {
	       return operation_type;
	    }
      };

      
     /**
      * cc_light_single_qubit_gate
      */
      class cc_light_single_qubit_gate : public cc_light_eqasm_instruction
      {

	 public:

	    single_qubit_mask mask;


	    /**
	     * ctor
	     */
	    cc_light_single_qubit_gate(std::string name, single_qubit_mask mask) : mask(mask)
	    {
	       this->name = name;
	    }

	    /**
	     * emit cc_light_eqasm code 
	     */
	    cc_light_eqasm_instr_t code()
	    {
	       std::stringstream c;
	       c << mask.get_mask(7) << "\n";
	       c << "bs 1 " << name << " s7";
	       return c.str();
	    }

      };

      /**
       * cc_light_two_qubit_gate
       */
      class cc_light_two_qubits_gate : public cc_light_eqasm_instruction
      {

	 public:

	    two_qubits_mask mask;


	    /**
	     * ctor
	     */
	    cc_light_two_qubits_gate(std::string name, two_qubits_mask mask) : mask(mask)
	    {
	       this->name = name;
	    }

	    /**
	     * emit cc_light_eqasm code 
	     */
	    cc_light_eqasm_instr_t code()
	    {
	       std::stringstream c;
	       c << mask.get_mask(7) << "\n";
	       c << "bs 1 " << name << " t7";
	       return c.str();
	    }

      };


      /**
       * cc_light_eqasm comparator
       */
      // bool cc_light_eqasm_comparator(cc_light_eqasm_instruction * i1, cc_light_eqasm_instruction * i2) 
      // { 
      //    return (i1->start < i2->start);
      // }
   
   }
}

#endif // QL_CC_LIGHT_EQASM_H





