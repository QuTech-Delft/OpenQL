/**
 * @file   qumis.h
 * @date   07/2017
 * @author Nader Khammassi
 * @brief  qumis code emitter
 */

#ifndef QL_QUMIS_H
#define QL_QUMIS_H

#include <string>
#include <bitset>

namespace ql
{
   namespace arch
   {
      #define __trigger_width__ (8)
      #define __lut_id_width__  (4)
      #define __awg_number__    (3)
      #define __resources__    \
              (__trigger_width__+__awg_number__)

      // single qumis instruction
      typedef size_t               channel_t;
      typedef bool                 bit_t;
      typedef std::string          qumis_instr_t;
      typedef std::vector<bit_t>   bitset_t;

      class qumis_instruction;

      typedef std::vector<qumis_instruction *> qumis_program_t;


      // instruction type
      typedef enum 
      {
	 __qumis_trigger__    ,
	 __qumis_pulse__      ,
	 __qumis_cw_trigger__ ,
	 __qumis_readout__    ,
	 __qumis_wait__
      } qumis_instr_type_t;

      // operation type
      typedef enum 
      {
	 __rf__         ,
	 __flux__       ,
	 __measurement__,
	 __wait__       ,
	 __unknown_operation__  
      } operation_type_t;

      // pulse/trigger codewords
      typedef std::bitset<__lut_id_width__>   pulse_id_t;
      typedef std::bitset<__trigger_width__>  codeword_t;

      // hardware resources
      typedef std::bitset<__resources__>      resources_t;

      /**
       * qumis instruction interface  
       */
      class qumis_instruction
      {
	 public:
	    
	    resources_t         used_resources;
	    size_t              duration;
	    size_t              start = 0;
	    qumis_instr_type_t  instruction_type;
	    operation_type_t    operation_type;
	    
	 public:

	    /**
	     * emit qumis code 
	     */
	    virtual qumis_instr_t code() = 0;
#if 0
	    /**
	     * return qumis instruction type
	     */
	    virtual qumis_instr_type_t instruction_type() = 0;

	    /**
	     * return operation type
	     */
	    virtual operation_type_t   operation_type() = 0;
#endif
      };

      /**
       * pulse
       */
      class pulse : public qumis_instruction
      {
	 public:
	    
	    size_t codeword;
	    size_t awg;
	    
	 public:

	    /**
	     * ctor
	     */
	    pulse(size_t codeword, size_t awg, size_t duration, ql::arch::operation_type_t operation_type) : codeword(codeword), awg(awg)
	    {
	       this->operation_type    = operation_type ;
	       this->instruction_type  = __qumis_pulse__;
	       this->duration          = duration;
	       used_resources.set(__trigger_width__+awg);         
	    }

	    /**
	     * generate code 
	     */
	    qumis_instr_t code()
	    {
	       std::stringstream params;
	       pulse_id_t pid = codeword;
	       params << (awg == 0 ? pid : 0) << "," << (awg == 1 ? pid : 0) << "," << (awg == 2 ? pid : 0) << "\nwait " << duration; 
	       qumis_instr_t instr = "pulse " + params.str();
	       // println("[i] used resources : " << used_resources);
	       return instr;
	    }
#if 0
	    /**
	     * instruction_type
	     */
	    qumis_instr_type_t instruction_type()
	    {
	       return __qumis_pulse__;
	    }
#endif 
      };
      
      /**
       * trigger
       */
      class trigger : public qumis_instruction
      {
	 public:
	    
	    codeword_t codeword;

	 public:
	    
	    /**
	     * ctor
	     */
	    trigger(codeword_t codeword, size_t duration, ql::arch::operation_type_t operation_type) : codeword(codeword) 
	    {
	       this->operation_type    = operation_type   ;
	       this->instruction_type  = __qumis_trigger__;
	       this->duration          = duration;
	       used_resources          = codeword.to_ulong();
	    }
	    
	    /**
	     * generate code 
	     */
	    qumis_instr_t code()
	    {
	       std::stringstream params;
	       params << codeword << "," << duration; 
	       qumis_instr_t instr = "trigger " + params.str();
	       // println("[i] used resources : " << used_resources);
	       return instr;
	    }

#if 0
	    /**
	     * instruction_type
	     */
	    qumis_instr_type_t instruction_type()
	    {
	       return __qumis_trigger__;
	    }
#endif
      };

      /**
       * trigger
       */
      class codeword_trigger : public qumis_instruction
      {
	 public:
	    
	    codeword_t codeword;
	    size_t     ready_bit;
	    size_t     ready_bit_duration;

	 public:
	    
	    /**
	     * ctor
	     */
	    codeword_trigger(codeword_t codeword, size_t duration, 
	                     size_t ready_bit, size_t ready_bit_duration, 
			     ql::arch::operation_type_t operation_type) : codeword(codeword), ready_bit(ready_bit), ready_bit_duration(ready_bit_duration) 
	    {
	       this->operation_type    = operation_type   ;
	       this->instruction_type  = __qumis_cw_trigger__;
	       this->duration          = duration;
	       used_resources          = codeword.to_ulong();
	       used_resources.set(ready_bit);
	    }
	    
	    /**
	     * generate code 
	     */
	    qumis_instr_t code()
	    {
	       codeword_t ready_cw = 0;
	       ready_cw.set(ready_bit);
	       std::stringstream instr;
	       instr << "trigger " << codeword << "," << duration << "\nwait 1\n"; 
	       instr << "trigger " << ready_cw << "," << ready_bit_duration << "\nwait " << (duration-1); 
	       // println("[i] used resources : " << used_resources);
	       return instr.str();
	    }
      };


      /**
       * measure 
       */
      class measure : public qumis_instruction
      {
	 public:
	    qumis_instruction * instruction;

	 public:
	    
	    /**
	     * ctor
	     */
	    measure(qumis_instruction * instruction, size_t duration) : instruction(instruction)
	    {
	       this->duration = duration;
	       operation_type = __measurement__;
	       used_resources.set();
	    }
	    
	    /**
	     * generate code 
	     */
	    qumis_instr_t code()
	    {
	       std::stringstream inst;
	       inst << instruction->code() << "\n";
	       inst << "wait " << (duration-instruction->duration); 
	       // println("[i] used resources : " << used_resources);
	       return inst.str();
	  
	    }

      };
   
   }
}

#endif // QL_QUMIS_H





