/**
 * @file   cbox_eqasm_compiler.h
 * @date   07/2017
 * @author Nader Khammassi
 * @brief  cbox eqasm compiler implementation 
 */

#ifndef QL_CBOX_EQASM_COMPILER_H
#define QL_CBOX_EQASM_COMPILER_H

#include <ql/platform.h>
#include <ql/eqasm_compiler.h>
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

	    qumis_program_t qumis_instructions;
	    size_t          ns_per_cycle;

	    #define __ns_to_cycle(t) (t/ns_per_cycle)
	   
	 public:

	    /*
	     * compile qasm to qumis
	     */
	    // eqasm_t 
	    void compile(ql::circuit& c, ql::quantum_platform& platform)
	    {
	       println("[-] compiling qasm code ...");
	       println("[-] loading circuit (" <<  c.size() << " gates)...");
	       eqasm_t eqasm_code;
	       ql::instruction_map_t& instr_map = platform.instruction_map; 
	       json& instruction_settings       = platform.instruction_settings;
	       ns_per_cycle                     = platform.hardware_settings["cycle_time"];
	       println("[+] cycle_time : " << ns_per_cycle);
	       for (ql::gate * g : c)
	       {
		  std::string id = g->name;
		  str::lower_case(id);
		  str::replace_all(id,"  ","");
		  println("id : " << id);
		  std::string qumis;
		  std::string operation;
		  if (!instruction_settings[id].is_null())
		  {
		     operation             = instruction_settings[id]["qumis_instr"]; 
		     size_t duration       = __ns_to_cycle((size_t)instruction_settings[id]["duration"]);
		     operation_type_t type = operation_type(instruction_settings[id]["type"]);
		     if (type == __unknown_operation__)
			println("[x] error : unknow operation type of the instruction '" << id << "' !");
		     std::stringstream params;
		     if (operation == "pulse")
		     {
			json& j_params = instruction_settings[id]["qumis_instr_kw"];
			process_pulse(j_params,duration,type);
			println("pulse code : " << qumis_instructions.back()->code());
		     } 
		     else if (operation == "codeword_trigger")
		     {
			json& j_params = instruction_settings[id]["qumis_instr_kw"];
			process_codeword_trigger(j_params, duration, type);
		     }
		     else if  ((operation == "trigger") && (type == __measurement__))
		     {
			json& j_params = instruction_settings[id]["qumis_instr_kw"];
                        std::string qumis_instr = instruction_settings[id]["qumis_instr"];
			process_measure(j_params, qumis_instr, duration, type);
		     }
		     // qumis = operation + " " + params.str();
		     //println("qumis : " << qumis);
		  }
		  else
		  {
		     println("[x] error : cbox_eqasm_compiler : instruction '" << id << "' not supported by the target platform !");
		  }
	       }

	       emit_eqasm();
	       // return eqasm_code;
	    }


	 private:

	    /**
	     * emit qasm code
	     */
	    void emit_eqasm()
	    {
	       println("compiling eqasm...");
	       eqasm_code.clear();
	       eqasm_code.push_back("wait 1");       // add wait 1 at the begining
	       eqasm_code.push_back("mov r14, 0");   // 0: infinite loop
	       eqasm_code.push_back("start:");       // label
	       for (qumis_instruction * instr : qumis_instructions)
		  eqasm_code.push_back(instr->code());
	       eqasm_code.push_back("beq r14,r14");  // loop
	       println("compilation done.");
	    }

	    /**
	     * process pulse
	     */
	    void process_pulse(json& j_params, size_t duration, operation_type_t type)
	    {	
	       println("processing pulse instruction...");
	       size_t codeword = j_params["codeword"];
	       size_t awg_nr   = j_params["awg_nr"];
	       println("\tcodeword: " << codeword);
	       println("\tawg     : " << awg_nr);
	       qumis_instructions.push_back(new pulse(codeword,awg_nr,duration,type));
	    }

	    /**
	     * process codeword trigger
	     */
	    void process_codeword_trigger(json& j_params, size_t duration, operation_type_t type)
	    {
	       println("processing codeword trigger instruction...");
	       size_t codeword_ready_bit           = j_params["codeword_ready_bit"];
	       size_t codeword_ready_bit_duration  = __ns_to_cycle((size_t)j_params["codeword_ready_bit_duration"]);
	       size_t bit_nr                       = j_params["codeword_bits"].size();
	       std::vector<size_t> bits            = j_params["codeword_bits"];

	       println("\tcodeword_ready_bit          : " << codeword_ready_bit);
	       println("\tcodeword_ready_bit_duration : " << codeword_ready_bit_duration);
	       println("\tbit_nr                      : " << bit_nr);

	       if (codeword_ready_bit > (__trigger_width__-1))
	       {
		  println("[x] error : invalid 'read bit' number in the codeword trigger definition !");
		  throw std::exception();
	       }

	       // ready trigger
	       // codeword_t ready_codeword = 0;
	       // ready_codeword.set(codeword_ready_bit);
	       // trigger * ready_trigger = new trigger(ready_codeword_trigger, codeword_ready_bit_duration, type);
	       // println("\t code (r_trigger): " << ready_trigger->code() );

	       // codeword trigger
	       codeword_t main_codeword_trigger = 0;
	       for (size_t b : bits) main_codeword_trigger.set(b);
	       // trigger * main_trigger = new trigger(main_codeword_trigger, duration, type);
	       //println("\t code (m_trigger): " << main_trigger->code() );
	       
	       codeword_trigger * instr = new codeword_trigger(main_codeword_trigger, duration, codeword_ready_bit, codeword_ready_bit_duration, type);
	       
	       println("\tcode: " << instr->code());

	       qumis_instructions.push_back(instr);
	    }

	    /**
	     * process readout
	     */
	    void process_measure(json& j_params, std::string instr, size_t duration, operation_type_t type)
	    {
	       println("processing measure instruction...");
	       qumis_instruction * qumis_instr;
	       if (instr == "trigger")
	       {
		  println("  ---> processing trigger : ");
		  size_t trigger_bit       = j_params["trigger_bit"];
		  size_t trigger_duration  = __ns_to_cycle((size_t)j_params["trigger_duration"]);
		  println("\ttrigger bit      : " << trigger_bit);
		  println("\ttrigger duration : " << trigger_duration);
		  if (trigger_bit > (__trigger_width__-1))
		  {
		     println("[x] error while processing the 'readout' instruction : invalid trigger bit.");
		     throw std::exception();
		  }
		  codeword_t cw = 0;
		  cw.set(trigger_bit);
		  qumis_instr = new trigger(cw, trigger_duration, __measurement__); 
		  qumis_instructions.push_back(new measure(qumis_instr, duration));
	       }
	       else
	       {
		     println("[x] error while processing the 'readout' instruction : only trigger-based implementation is supported !");
		     throw std::exception();
	       }
	       println("measure instruction processed.");
	    }


	    /**
	     * return operation type
	     */
	    operation_type_t operation_type(std::string type)
	    {
	       if (type == "mw")
		  return __rf__;
	       else if (type == "flux")
		  return __flux__;
	       else if (type == "readout")
		  return __measurement__;
	       else
		  return __unknown_operation__;
	    }
      };
   }
}

#endif // QL_CBOX_EQASM_COMPILER_H

