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

#include <ql/arch/instruction_scheduler.h>

namespace ql
{
   namespace arch
   {
      #define __trigger_width__ (8)
      #define __lut_id_width__  (4)
      #define __awg_number__    (3)
      #define __resources__    \
              (__trigger_width__+__awg_number__)
      
      const std::string channel_name[] = {"TRIG_0","TRIG_1","TRIG_2","TRIG_3","TRIG_4","TRIG_5","TRIG_6","TRIG_7","AWG_0","AWG_1","AWG_2"};

      // single qumis instruction
      // typedef size_t               channel_t;
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
	    size_t              latency = 0; 
	    size_t              start = 0;
	    qumis_instr_type_t  instruction_type;
	    operation_type_t    operation_type;
	    bool                latency_compensated = false;
	    
	 public:

	    /**
	     * emit qumis code 
	     */
	    virtual qumis_instr_t code() = 0;

	    /**
	     * return instruction trace
	     */
	    virtual instruction_traces_t trace() = 0;

	    /**
	     *  compensate for latency
	     */
	    void compensate_latency()
	    {
	       if (!latency_compensated)
	       {
		  println("compensate latency : " << start << " -> " << (start-latency) << " : latency = " << latency);
		  start -= latency;
		  latency_compensated = true;
	       }
	       else
	       {
		  println("[x] warning : latency of instruction '" << this << "' is already compensated !");
	       }
	    }    
	    
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
	    
	    size_t codeword; // lut id
	    size_t awg;
	    
	 public:

	    /**
	     * ctor
	     */
	    pulse(size_t codeword, size_t awg, size_t duration, ql::arch::operation_type_t operation_type, size_t latency=0) : codeword(codeword), awg(awg)
	    {
	       this->operation_type    = operation_type ;
	       this->instruction_type  = __qumis_pulse__;
	       this->duration          = duration;
	       this->latency           = latency;
	       latency_compensated     = false;
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

	   /**
	    * trace
	    */
	    instruction_traces_t trace()
	    {
	       instruction_traces_t trs;
	       size_t latent_start = (latency_compensated ? (start) : (start-latency));
	       instruction_trace_t t  = { (__trigger_width__+awg), code(), start, (start+duration), "#4567aa", __top_pos__};
	       // instruction_trace_t tl = { (__trigger_width__+awg), "", start-latency, (start-latency+duration), "#403377", __bottom_pos__ }; // latent
	       instruction_trace_t tl = { (__trigger_width__+awg), code(), latent_start, latent_start+duration, "#808080", __bottom_pos__ }; // latent
	       trs.push_back(t);
	       trs.push_back(tl);
	       return trs; 
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
	    trigger(codeword_t codeword, size_t duration, ql::arch::operation_type_t operation_type, size_t latency=0) : codeword(codeword) 
	    {
	       this->operation_type    = operation_type   ;
	       this->instruction_type  = __qumis_trigger__;
	       this->duration          = duration;
	       this->latency           = latency;
	       used_resources          = codeword.to_ulong();
	    }
	    
	    /**
	     * generate code 
	     */
	    qumis_instr_t code()
	    {
	       std::stringstream params;
	       params << codeword << "," << duration << "\nwait " << duration; 
	       qumis_instr_t instr = "trigger " + params.str();
	       // println("[i] used resources : " << used_resources);
	       return instr;
	    }

	   /**
	    * trace
	    */
	    instruction_traces_t trace()
	    {
	       instruction_traces_t trs;
	       for (size_t ch=0; ch<codeword.size(); ++ch)
	       {
		  if (codeword.test(ch))
		  {
		     size_t latent_start = (latency_compensated ? (start) : (start-latency));
		     instruction_trace_t t  = { ch, code(), start, (start+duration), "#c467aa", __top_pos__ };
		     instruction_trace_t lt = { ch, code(), latent_start, (latent_start+duration), "#808080", __bottom_pos__ };
		     trs.push_back(lt);
		     trs.push_back(t);
		  }
	       }
	       return trs; 
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
			     ql::arch::operation_type_t operation_type, size_t latency=0) : codeword(codeword), ready_bit(ready_bit), ready_bit_duration(ready_bit_duration) 
	    {
	       this->operation_type    = operation_type   ;
	       this->instruction_type  = __qumis_cw_trigger__;
	       this->duration          = duration;
	       this->latency           = latency;
	       used_resources          = codeword.to_ulong();
	       used_resources.set(ready_bit);
	       if (ready_bit_duration > (duration-1))
		  println("[x] error in codeword trigger definition : 'ready_bit_duration' cannot be greater than overall 'duration' !");
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

	   /**
	    * trace
	    */
	    instruction_traces_t trace()
	    {
	       instruction_traces_t trs;
	       size_t latent_start = (latency_compensated ? (start) : (start-latency));
	       for (size_t ch=0; ch<codeword.size(); ++ch)
	       {
		  if (codeword.test(ch))
		  {
		     instruction_trace_t t  = { ch, code(), start, (start+duration), "#DD5437", __top_pos__ };
		     instruction_trace_t lt = { ch, code(), latent_start, (latent_start+duration), "#808080", __bottom_pos__  };
		     trs.push_back(lt);
		     trs.push_back(t);
		  }
	       }
	       instruction_trace_t t = { ready_bit, code(), start+1, (start+1+ready_bit_duration), "#DD5437", __top_pos__ };
	       instruction_trace_t lt = { ready_bit, code(), latent_start+1, (latent_start+1+ready_bit_duration), "#808080", __bottom_pos__ };
	       trs.push_back(lt);
	       trs.push_back(t);
	       return trs; 
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
	    measure(qumis_instruction * instruction, size_t duration, size_t latency=0) : instruction(instruction)
	    {
	       this->duration = duration;
	       this->latency  = latency;
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

	    /**
	     *  compensate for latency
	     */
	    void compensate_latency()
	    {
	       if (!latency_compensated)
	       {
		  println("compensate latency : " << start << " -> " << (start-latency) << " : latency = " << latency);
		  start -= latency;
		  latency_compensated = true;
		  instruction->compensate_latency();
	       }
	       else
	       {
		  println("[x] warning : latency of instruction '" << this << "' is already compensated !");
	       }
	    }


	   /**
	    * trace
	    */
	    instruction_traces_t trace()
	    {
	       instruction_traces_t itrs = instruction->trace();
	       instruction_traces_t trs; 
	       bool lt = true;
	       for (instruction_trace_t t : itrs)
	       {
		  t.start = start;
		  t.end   = start+duration;
		  t.label = code();
		  t.color = (lt ? "#808080" : "#328F1C");
		  trs.push_back(t);
		  lt = !lt;
	       }
	       return trs; 
	    }


      };
   
   }
}

#endif // QL_QUMIS_H





