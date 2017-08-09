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
	    size_t          total_exec_time = 0;
	    size_t          buffer_matrix[__operation_types_num__][__operation_types_num__]; 

	    #define __ns_to_cycle(t) ((size_t)t/(size_t)ns_per_cycle)
	   
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
	       // buffer matrix: 
	       buffer_matrix[__rf__][__rf__]                   = __ns_to_cycle(platform.hardware_settings["mw_mw_buffer"]);
	       buffer_matrix[__rf__][__flux__]                 = __ns_to_cycle(platform.hardware_settings["mw_flux_buffer"]);
	       buffer_matrix[__rf__][__measurement__]          = __ns_to_cycle(platform.hardware_settings["mw_readout_buffer"]);
	       buffer_matrix[__flux__][__rf__]                 = __ns_to_cycle(platform.hardware_settings["flux_mw_buffer"]);
	       buffer_matrix[__flux__][__flux__]               = __ns_to_cycle(platform.hardware_settings["flux_flux_buffer"]);
	       buffer_matrix[__flux__][__measurement__]        = __ns_to_cycle(platform.hardware_settings["flux_readout_buffer"]);
	       buffer_matrix[__measurement__][__rf__]          = __ns_to_cycle(platform.hardware_settings["readout_mw_buffer"]);
	       buffer_matrix[__measurement__][__flux__]        = __ns_to_cycle(platform.hardware_settings["readout_flux_buffer"]);
	       buffer_matrix[__measurement__][__measurement__] = __ns_to_cycle(platform.hardware_settings["readout_readout_buffer"]);

	       println("[=] buffer matrix : ");
	       for (size_t i=0; i<__operation_types_num__; ++i)
		  for (size_t j=0; j<__operation_types_num__; ++j)
		     println("(" << i << "," << j << ") :" << buffer_matrix[i][j]);

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
		     size_t latency        = 0;
		     if (!instruction_settings[id]["latency"].is_null())
			latency = __ns_to_cycle((size_t)instruction_settings[id]["latency"]);
		     operation_type_t type = operation_type(instruction_settings[id]["type"]);
		     if (type == __unknown_operation__)
			println("[x] error : unknow operation type of the instruction '" << id << "' !");
		     std::stringstream params;
		     if (operation == "pulse")
		     {
			json& j_params = instruction_settings[id]["qumis_instr_kw"];
			process_pulse(j_params, duration, type, latency);
			println("pulse code : " << qumis_instructions.back()->code());
		     } 
		     else if (operation == "codeword_trigger")
		     {
			json& j_params = instruction_settings[id]["qumis_instr_kw"];
			process_codeword_trigger(j_params, duration, type, latency);
		     }
		     else if  ((operation == "trigger") && (type == __measurement__))
		     {
			json& j_params = instruction_settings[id]["qumis_instr_kw"];
                        std::string qumis_instr = instruction_settings[id]["qumis_instr"];
			process_measure(j_params, qumis_instr, duration, type, latency);
		     }
		     // qumis = operation + " " + params.str();
		     //println("qumis : " << qumis);
		  }
		  else
		  {
		     println("[x] error : cbox_eqasm_compiler : instruction '" << id << "' not supported by the target platform !");
		  }
	       }

	       // time analysis
	       total_exec_time = time_analysis();

	       // compensate for latencies
	       compensate_latency();

	       // reschedule
	       resechedule();

	       // dump_instructions();
	       
	       // decompose meta-instructions
	       decompose_instructions();

	       // reorder instructions
	       // dump_instructions();
	       reorder_instructions();
	       // dump_instructions();

	       // insert waits

	       emit_eqasm();
	       // return eqasm_code;
	    }

	    /**
	     * display instruction and start time
	     */
	    void dump_instructions()
	    {
	       println("[d] instructions dump:");
	       for (qumis_instruction * instr : qumis_instructions)
	       {
		  size_t t = instr->start;
		  std::cout << t << " : " << instr->code() << std::endl;
	       }
	    }

	    /**
	     * decompose
	     */
	    void decompose_instructions()
	    {
	       qumis_program_t decomposed;  
	       for (qumis_instruction * instr : qumis_instructions)
	       {
		  qumis_program_t dec = instr->decompose();
		  for (qumis_instruction * i : dec)
		     decomposed.push_back(i);
	       }
	       qumis_instructions.swap(decomposed);
	    }


	    /**
	     * reorder instructions
	     */
	     void reorder_instructions()
	     {
	       std::sort(qumis_instructions.begin(),qumis_instructions.end(), qumis_comparator);
	     }

	    /**
	     * time analysis
	     */
	    size_t time_analysis()
	    {
	       println("time analysis...");
	       // update start time : find biggest latency
	       size_t max_latency = 0;
	       for (qumis_instruction * instr : qumis_instructions)
	       {
		  size_t l = instr->latency;
		  max_latency = (l > max_latency ? l : max_latency);
	       }
	       // set refrence time to max latency (avoid negative reference)
	       size_t time = max_latency; // 0;
	       for (qumis_instruction * instr : qumis_instructions)
	       {
		  println(time << ":");
		  println(instr->code());
		  // instr->start = time;
		  instr->set_start(time);
		  time        += instr->duration; //+1;
	       }
	       return time;
	    }



	    /**
	     * compensate for latencies
	     */
	    void compensate_latency()
	    {
	       for (qumis_instruction * instr : qumis_instructions)
		  instr->compensate_latency();
	    }
	    
	    /**
	     * optimize 
	     */
	    void resechedule()
	    {
	       std::vector<size_t>           hw_res_av(__trigger_width__+__awg_number__,0);
	       std::vector<operation_type_t> hw_res_op(__trigger_width__+__awg_number__,__none__);

	       for (qumis_instruction * instr : qumis_instructions)
	       {
		  resources_t      res  = instr->used_resources;
		  operation_type_t type = instr->get_operation_type();
		  size_t latest = 0;
		  size_t buf    = 0;
		  for (size_t r=0; r<res.size(); ++r)
		  {
		     if (res.test(r))
		     {
			size_t rbuf = buffer_size(hw_res_op[r],type);
			buf         = ((rbuf > buf) ? rbuf : buf);
			latest      = (hw_res_av[r] > latest ? hw_res_av[r] : latest);
		     }
		  }
		  if (buf)
		     println("[!] inserting buffer...");
		  instr->start = latest+buf;
		  for (size_t r=0; r<res.size(); ++r)
		  {
		     if (res.test(r))
		     {
			hw_res_av[r] = (instr->start+instr->duration);
			hw_res_op[r] = (type);
		     }
		  }
	       }
	    }

	    /**
	     * buffer size
	     */
	    size_t buffer_size(operation_type_t t1, operation_type_t t2)
	    {  
	       return buffer_matrix[t1][t2];
	    }

	    /**
	     * dump traces
	     */
	    void write_traces(std::string file_name="")
	    {
	       ql::arch::channels_t channels;

	       for (size_t i=0; i<__trigger_width__; i++)
	       {
		  std::string ch = "TRIG_"+std::to_string(i);
	          channels.push_back(ch);
	       }

	       for (size_t i=0; i<__awg_number__; i++)
	       {
		  std::string ch = "AWG_"+std::to_string(i);
	          channels.push_back(ch);
	       }

	       ql::arch::time_diagram diagram(channels,total_exec_time,4);

	       for (qumis_instruction * instr : qumis_instructions)
	       {
		  instruction_traces_t trs = instr->trace(); 
		  for (instruction_trace_t t : trs)
		     diagram.add_trace(t);
	       }

	       diagram.dump("../../lab/trace.dat");
	       
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
	       size_t t = 0;
	       for (qumis_instruction * instr : qumis_instructions)
	       {
		  size_t start = instr->start; 
		  size_t dt = start-t;
		  if (dt)
		  {
		     eqasm_code.push_back("wait "+std::to_string(dt));
		     t = start;
		  }
		  eqasm_code.push_back(instr->code());
	       }
	       eqasm_code.push_back("wait "+std::to_string(qumis_instructions.back()->duration));
	       eqasm_code.push_back("beq r14,r14");  // loop
	       println("compilation done.");
	    }

	    /**
	     * process pulse
	     */
	    void process_pulse(json& j_params, size_t duration, operation_type_t type, size_t latency)
	    {	
	       println("processing pulse instruction...");
	       size_t codeword = j_params["codeword"];
	       size_t awg_nr   = j_params["awg_nr"];
	       println("\tcodeword: " << codeword);
	       println("\tawg     : " << awg_nr);
	       qumis_instructions.push_back(new pulse(codeword,awg_nr,duration,type,latency));
	    }

	    /**
	     * process codeword trigger
	     */
	    void process_codeword_trigger(json& j_params, size_t duration, operation_type_t type, size_t latency)
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
	       
	       codeword_trigger * instr = new codeword_trigger(main_codeword_trigger, duration, codeword_ready_bit, codeword_ready_bit_duration, type, latency);
	       
	       println("\tcode: " << instr->code());

	       qumis_instructions.push_back(instr);
	    }

	    /**
	     * process readout
	     */
	    void process_measure(json& j_params, std::string instr, size_t duration, operation_type_t type, size_t latency)
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
		  qumis_instr = new trigger(cw, trigger_duration, __measurement__, latency); 
		  qumis_instructions.push_back(new measure(qumis_instr, duration,latency));
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

