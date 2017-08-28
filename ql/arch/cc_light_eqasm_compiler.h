/**
 * @file   cc_light_eqasm_compiler.h
 * @date   08/2017
 * @author Nader Khammassi
 * @brief  cclighteqasm compiler implementation
 */

#ifndef QL_CC_LIGHT_EQASM_COMPILER_H
#define QL_CC_LIGHT_EQASM_COMPILER_H

#include <ql/platform.h>
#include <ql/eqasm_compiler.h>
#include <ql/arch/cc_light_eqasm.h>

// eqasm code : set of cc_light_eqasm instructions
typedef std::vector<ql::arch::cc_light_eqasm_instr_t> eqasm_t;

namespace ql
{
   namespace arch
   {
      /**
       * cbox eqasm compiler
       */
      class cc_light_eqasm_compiler : public eqasm_compiler
      {
	 public:

	    cc_light_eqasm_program_t cc_light_eqasm_instructions;
	    size_t          num_qubits;
	    size_t          ns_per_cycle;
	    size_t          total_exec_time = 0;
	    size_t          buffer_matrix[__operation_types_num__][__operation_types_num__];
	    bool            verbose = false;

	    #define __ns_to_cycle(t) ((size_t)t/(size_t)ns_per_cycle)

	 public:

	    /*
	     * compile qasm to cc_light_eqasm
	     */
	    // eqasm_t
	    void compile(ql::circuit& c, ql::quantum_platform& platform) throw (ql::exception)
	    {
	       if (verbose) println("[-] compiling qasm code ...");
	       if (c.empty())
	       {
		  println("[-] empty circuit, eqasm compilation aborted !");
		  return;
	       }
	       if (verbose) println("[-] loading circuit (" <<  c.size() << " gates)...");
	       eqasm_t eqasm_code;
	       // ql::instruction_map_t& instr_map = platform.instruction_map;
	       json& instruction_settings       = platform.instruction_settings;

	       std::string params[] = { "qubit_number", "cycle_time", "mw_mw_buffer", "mw_flux_buffer", "mw_readout_buffer", "flux_mw_buffer", 
	                                "flux_flux_buffer", "flux_readout_buffer", "readout_mw_buffer", "readout_flux_buffer", "readout_readout_buffer" };
	       size_t p = 0;

	       try 
	       {
		  num_qubits                                      = platform.hardware_settings[params[p++]];
		  ns_per_cycle                                    = platform.hardware_settings[params[p++]];

		  buffer_matrix[__rf__][__rf__]                   = __ns_to_cycle(platform.hardware_settings[params[p++]]);
		  buffer_matrix[__rf__][__flux__]                 = __ns_to_cycle(platform.hardware_settings[params[p++]]);
		  buffer_matrix[__rf__][__measurement__]          = __ns_to_cycle(platform.hardware_settings[params[p++]]);
		  buffer_matrix[__flux__][__rf__]                 = __ns_to_cycle(platform.hardware_settings[params[p++]]);
		  buffer_matrix[__flux__][__flux__]               = __ns_to_cycle(platform.hardware_settings[params[p++]]);
		  buffer_matrix[__flux__][__measurement__]        = __ns_to_cycle(platform.hardware_settings[params[p++]]);
		  buffer_matrix[__measurement__][__rf__]          = __ns_to_cycle(platform.hardware_settings[params[p++]]);
		  buffer_matrix[__measurement__][__flux__]        = __ns_to_cycle(platform.hardware_settings[params[p++]]);
		  buffer_matrix[__measurement__][__measurement__] = __ns_to_cycle(platform.hardware_settings[params[p++]]);
	       }
	       catch (json::exception e)
	       {
		 throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while reading hardware settings : parameter '"+params[p-1]+"'\n\t"+ std::string(e.what()),false);
	       }

	       std::stringstream control_store;
	       control_store << "         Condition  OpTypeLeft  CW_Left  OpTypeRight  CW_Right\n";
	       control_store << "     0:      0          0          0          0           0    \n";
	       for (auto i : instruction_settings)
	       {
		  if (i["cc_light_instr_type"] == "single_qubit_gate")
		  {
		     auto opcode     = i["cc_light_opcode"];
		     auto condition  = 0;
		     auto optype     = (i["type"] == "mw" ? 1 : (i["type"] == "flux" ? 2 : ((i["type"] == "readout" ? 3 : 0))));
		     auto codeword   = i["cc_light_codeword"];
		     control_store << "     " << i["cc_light_opcode"] << ":     0          " << optype << "          " << codeword << "          0          0\n";
		  }
		  else if (i["cc_light_instr_type"] == "two_qubits_gate")
		  {
		     auto opcode     = i["cc_light_opcode"];
		     auto condition  = 0;
		     auto optype     = (i["type"] == "mw" ? 1 : (i["type"] == "flux" ? 2 : ((i["type"] == "readout" ? 3 : 0))));
		     auto codeword_l = i["cc_light_left_codeword"];
		     auto codeword_r = i["cc_light_right_codeword"];
		     control_store << "     " << i["cc_light_opcode"] << ":     0          " << optype << "          " << codeword_l << "          " << optype << "          " << codeword_r << "\n";
		  }
		  else
		     throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while reading hardware settings : invalid 'cc_light_instr_type' for instruction !",false);
		  // println("\n" << control_store.str());
	       }

	       std::string cs_filename = ql::utils::get_output_dir() + "/cs.txt";
	       println("writing control store fil to '" << cs_filename << "' ...");
	       std::string s = control_store.str();
	       ql::utils::write_file(cs_filename,s);

	       for (ql::gate * g : c)
	       {
		  std::string id = g->qasm(); // g->name;
		  str::lower_case(id);
		  str::replace_all(id,"  ","");
		  std::string cc_light_eqasm;
		  std::string operation;
		  if (!instruction_settings[id].is_null())
		  {
		     if (instruction_settings[id]["cc_light_instr"].is_null())
			throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while reading hardware settings : 'cc_light_instr' for instruction '"+id+"' is not specified !",false);
		     operation             = instruction_settings[id]["cc_light_instr"];
		     size_t duration       = __ns_to_cycle((size_t)instruction_settings[id]["duration"]);
		     size_t latency        = 0;
		     
		     if (!instruction_settings[id]["latency"].is_null())
			latency = __ns_to_cycle((size_t)instruction_settings[id]["latency"]);
		     else
			throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while reading hardware settings : parameter 'latency' of instruction '"+id+"' not specified !",false);
		     // used qubits
		     qubit_set_t used_qubits;
		     // instruction type processing
		     if (instruction_settings[id]["type"].is_null())
			throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while reading hardware settings : parameter 'type' of instruction '"+id+"' not specified !",false);
		     operation_type_t type = operation_type(instruction_settings[id]["type"]);
		     if (type == __unknown_operation__)
		     {
			println("[x] error : unknow operation type of the instruction '" << id << "' !");
			throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while reading hardware settings : the type of instruction '"+id+"' is unknown !",false);
		     }

		     if (instruction_settings[id]["cc_light_instr_type"].is_null())
		     {
			println("[x] error : unknow operation type of the instruction '" << id << "' !");
			throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while reading hardware settings : the type of instruction '"+id+"' is unknown !",false);
		     }

		     std::string instr_type = instruction_settings[id]["cc_light_instr_type"];

		     if (instr_type == "single_qubit_gate")
		     {
			cc_light_single_qubit_gate * instr = new cc_light_single_qubit_gate(operation,single_qubit_mask(g->operands[0]));
			cc_light_eqasm_instructions.push_back(instr);
		     }
		     else if (instr_type == "two_qubits_gate")
		     {
			cc_light_two_qubits_gate * instr = new cc_light_two_qubits_gate(operation,two_qubits_mask(qubit_pair_t(g->operands[0],g->operands[1])));
			cc_light_eqasm_instructions.push_back(instr);
		     } else // unknown type of operation
		     {
			println("[x] error : unknow instruction type of the instruction '" << id << "' (should be single or two qubit gates) !");
			throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while reading hardware settings : the type of instruction '"+id+"' is unknown !",false);
		     }
		  }
		  else
		  {
		     println("[x] error : cc_light_eqasm_compiler : instruction '" << id << "' not supported by the target platform !");
		  }
	       }

	       // time analysis
	       // total_exec_time = time_analysis();

	       // compensate for latencies
	       // compensate_latency();

	       // reschedule
	       // resechedule();

	       // dump_instructions();

	       // decompose meta-instructions
	       // decompose_instructions();

	       // reorder instructions
	       // reorder_instructions();

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
	       for (cc_light_eqasm_instruction * instr : cc_light_eqasm_instructions)
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
	       /*
	       if (verbose) println("decomposing instructions...");
	       cc_light_eqasm_program_t decomposed;
	       for (cc_light_eqasm_instruction * instr : cc_light_eqasm_instructions)
	       {
		  cc_light_eqasm_program_t dec = instr->decompose();
		  for (cc_light_eqasm_instruction * i : dec)
		     decomposed.push_back(i);
	       }
	       cc_light_eqasm_instructions.swap(decomposed);
	       */
	    }


	    /**
	     * reorder instructions
	     */
	     void reorder_instructions()
	     {
	       // if (verbose) println("reodering instructions...");
	       // std::sort(cc_light_eqasm_instructions.begin(),cc_light_eqasm_instructions.end(), cc_light_eqasm_comparator);
	     }

	    /**
	     * time analysis
	     */
	    size_t time_analysis()
	    {
	       if (verbose) println("time analysis...");
	       // update start time : find biggest latency
	       size_t max_latency = 0;
	       for (cc_light_eqasm_instruction * instr : cc_light_eqasm_instructions)
	       {
		  size_t l = instr->latency;
		  max_latency = (l > max_latency ? l : max_latency);
	       }
	       // set refrence time to max latency (avoid negative reference)
	       size_t time = max_latency; // 0;
	       for (cc_light_eqasm_instruction * instr : cc_light_eqasm_instructions)
	       {
		  // println(time << ":");
		  // println(instr->code());
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
	       if (verbose) println("latency compensation...");
	       for (cc_light_eqasm_instruction * instr : cc_light_eqasm_instructions)
		  instr->compensate_latency();
	    }

	    /**
	     * optimize
	     */
	    void resechedule()
	    {
	       if (verbose) println("instruction rescheduling...");
	       if (verbose) println("resource dependency analysis...");
	       if (verbose) println("buffer insertion...");
#if 0
	       std::vector<size_t>           hw_res_av(__trigger_width__+__awg_number__,0);
	       std::vector<size_t>           qu_res_av(num_qubits,0);
	       std::vector<operation_type_t> hw_res_op(__trigger_width__+__awg_number__,__none__);
	       std::vector<operation_type_t> qu_res_op(num_qubits,__none__);

	       for (cc_light_eqasm_instruction * instr : cc_light_eqasm_instructions)
	       {
		  resources_t      hw_res  = instr->used_resources;
		  qubit_set_t      qu_res  = instr->used_qubits;
		  operation_type_t type    = instr->get_operation_type();
		  size_t latest_hw = 0;
		  size_t buf_hw    = 0;
		  size_t latest_qu = 0;
		  size_t buf_qu    = 0;
		  // hardware dependency
		  for (size_t r=0; r<hw_res.size(); ++r)
		  {
		     if (hw_res.test(r))
		     {
			size_t rbuf = buffer_size(hw_res_op[r],type);
			buf_hw      = ((rbuf > buf_hw) ? rbuf : buf_hw);
			latest_hw   = (hw_res_av[r] > latest_hw ? hw_res_av[r] : latest_hw);
		     }
		  }

		  // qubit dependency
		  for (size_t q : qu_res) // qubits used by the instr
		  {
			size_t rbuf  = buffer_size(qu_res_op[q],type);
			buf_qu       = ((rbuf > buf_qu) ? rbuf : buf_qu);
			latest_qu    = (qu_res_av[q] > latest_qu ? qu_res_av[q] : latest_qu);
		  }

		  // println("latest_hw: " << latest_hw);
		  // println("latest_qu: " << latest_qu);

		  size_t latest = std::max(latest_hw,latest_qu);
		  size_t buf    = std::max(buf_hw,buf_qu);

		  //if (buf)
		    // println("[!] inserting buffer...");

		  instr->start = latest+buf;
		  // update latest hw record
		  for (size_t r=0; r<hw_res.size(); ++r)
		  {
		     if (hw_res.test(r))
		     {
			hw_res_av[r] = (instr->start+instr->duration);
			hw_res_op[r] = (type);
		     }
		  }
		  // update latest hw record
		  for (size_t q : qu_res) // qubits used by the instr
		  {
		     qu_res_av[q] = (instr->start+instr->duration);
		     qu_res_op[q] = (type);
		  }
	       }
#endif
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
#if 0
	       ql::arch::channels_t channels;
	       if (cc_light_eqasm_instructions.empty())
	       {
		  println("[!] warning : empty cc_light_eqasm code : not traces to dump !");
		  return;
	       }

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

	       for (cc_light_eqasm_instruction * instr : cc_light_eqasm_instructions)
	       {
		  instruction_traces_t trs = instr->trace();
		  for (instruction_trace_t t : trs)
		     diagram.add_trace(t);
	       }

	       diagram.dump(ql::utils::get_output_dir() + "/trace.dat");
#endif
	    }


	 private:

	    /**
	     * emit qasm code
	     */
	    void emit_eqasm()
	    {
	       if (verbose) println("compiling eqasm...");
	       eqasm_code.clear();
	       // eqasm_code.push_back("wait 1");       // add wait 1 at the begining
	       // eqasm_code.push_back("mov r14, 0");   // 0: infinite loop
	       // eqasm_code.push_back("start:");       // label
	       size_t t = 0;
	       for (cc_light_eqasm_instruction * instr : cc_light_eqasm_instructions)
	       {
		  size_t start = instr->start;
		  size_t dt = start-t;
		  if (dt)
		  {
		     // eqasm_code.push_back("wait "+std::to_string(dt));
		     // t = start;
		  }
		  eqasm_code.push_back(instr->code());
	       }
	       // eqasm_code.push_back("wait "+std::to_string(cc_light_eqasm_instructions.back()->duration));
	       // eqasm_code.push_back("beq r14, r14 start");  // loop
	       println("compilation done.");
	    }

	    /**
	     * process 
	     */
	    void process_single_qubit_gate(std::string instr_name, size_t duration, operation_type_t type, size_t latency, qubit_set_t& qubits, std::string& qasm_label)
	    {
	       cc_light_single_qubit_gate * instr = new cc_light_single_qubit_gate(instr_name,single_qubit_mask(qubits[0]));
	       cc_light_eqasm_instructions.push_back(instr);
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

#endif // QL_CC_LIGHT_EQASM_COMPILER_H

