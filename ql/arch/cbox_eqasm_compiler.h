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
            size_t          num_qubits;
            size_t          ns_per_cycle;
            size_t          total_exec_time = 0;
            size_t          buffer_matrix[__operation_types_num__][__operation_types_num__];
            bool            verbose = false;

         #define __ns_to_cycle(t) ((size_t)t/(size_t)ns_per_cycle)

         public:

            /*
             * compile qasm to qumis
             */
            // eqasm_t
            void compile(std::string prog_name, ql::circuit& c, ql::quantum_platform& platform, bool verbose=false) throw (ql::exception)
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
#if 0
                  buffer_matrix[__rf__][__rf__]                   = __ns_to_cycle(platform.hardware_settings["mw_mw_buffer"]);
                  buffer_matrix[__rf__][__flux__]                 = __ns_to_cycle(platform.hardware_settings["mw_flux_buffer"]);
                  buffer_matrix[__rf__][__measurement__]          = __ns_to_cycle(platform.hardware_settings["mw_readout_buffer"]);
                  buffer_matrix[__flux__][__rf__]                 = __ns_to_cycle(platform.hardware_settings["flux_mw_buffer"]);
                  buffer_matrix[__flux__][__flux__]               = __ns_to_cycle(platform.hardware_settings["flux_flux_buffer"]);
                  buffer_matrix[__flux__][__measurement__]        = __ns_to_cycle(platform.hardware_settings["flux_readout_buffer"]);
                  buffer_matrix[__measurement__][__rf__]          = __ns_to_cycle(platform.hardware_settings["readout_mw_buffer"]);
                  buffer_matrix[__measurement__][__flux__]        = __ns_to_cycle(platform.hardware_settings["readout_flux_buffer"]);
                  buffer_matrix[__measurement__][__measurement__] = __ns_to_cycle(platform.hardware_settings["readout_readout_buffer"]);
#endif
               }
               catch (json::exception e)
               {
                  throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while reading hardware settings : parameter '"+params[p-1]+"'\n\t"+ std::string(e.what()),false);
               }

               /*
                  println("[=] buffer matrix : ");
                  for (size_t i=0; i<__operation_types_num__; ++i)
                  for (size_t j=0; j<__operation_types_num__; ++j)
                  println("(" << i << "," << j << ") :" << buffer_matrix[i][j]);
                */

               for (ql::gate * g : c)
               {
                  std::string id = g->qasm(); // g->name;
                  str::lower_case(id);
                  str::replace_all(id,"  ","");
                  std::string qumis;
                  std::string operation;
                  if (!instruction_settings[id].is_null())
                  {
                     if (instruction_settings[id]["qumis_instr"].is_null())
                        throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while reading hardware settings : 'qumis_instr' for instruction '"+id+"' is not specified !",false);
                     operation             = instruction_settings[id]["qumis_instr"];
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
                     if (instruction_settings[id]["qumis_instr_kw"].is_null())
                        throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while reading hardware settings : 'qumis_instr_kw' for instruction '"+id+"' is not specified !",false);

                     std::stringstream params;

                     if (operation == "pulse")
                     {
                        // println("pulse id: " << id);
                        json& j_params = instruction_settings[id]["qumis_instr_kw"];
                        process_pulse(j_params, duration, type, latency, g->operands, id);
                        // println("pulse code : " << qumis_instructions.back()->code());
                     }
                     else if (operation == "codeword_trigger")
                     {
                        // println("cw id: " << id);
                        json& j_params = instruction_settings[id]["qumis_instr_kw"];
                        process_codeword_trigger(j_params, duration, type, latency, g->operands, id);
                     }
                     else if (operation == "pulse_trigger")
                     {
                        // println("cw id: " << id);
                        json& j_params = instruction_settings[id]["qumis_instr_kw"];
                        process_pulse_trigger(j_params, duration, type, latency, g->operands, id);
                     }
                     else if  ((operation == "trigger") && (type == __measurement__))
                     {
                        // println("measurement (trig) id: " << id);
                        json& j_params = instruction_settings[id]["qumis_instr_kw"];
                        std::string qumis_instr = instruction_settings[id]["qumis_instr"];
                        process_measure(j_params, qumis_instr, duration, type, latency, g->operands, id);
                     }
                     else if  ((operation == "trigger"))
                     {
                        // println("trig id: " << id);
                        json& j_params = instruction_settings[id]["qumis_instr_kw"];
                        std::string qumis_instr = instruction_settings[id]["qumis_instr"];
                        process_trigger(j_params, qumis_instr, duration, type, latency, g->operands, id);
                     }
                     // qumis = operation + " " + params.str();
                     //println("qumis : " << qumis);
                  }
                  else
                  {
                     println("[x] error : cbox_eqasm_compiler : instruction '" << id << "' not supported by the target platform !");
                     throw ql::exception("[x] error : cbox_eqasm_compiler : error while reading hardware settings : instruction '"+id+"' not supported by the target platform !",false);
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
               reorder_instructions();

               // split/merge concurrent triggers
               process_concurrent_triggers();

               // emit eqasm 
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
               if (verbose) println("decomposing instructions...");
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
               if (verbose) println("reodering instructions...");
               std::sort(qumis_instructions.begin(),qumis_instructions.end(), qumis_comparator);
            }

            /**
             * time analysis
             */
            size_t time_analysis()
            {
               if (verbose) println("time analysis...");
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
                  // println(time << ":");
                  // println(instr->code());
                  // instr->start = time;
                  instr->set_start(time);
                  time        += instr->duration; //+1;
               }
               return time;
            }

            /**
             *
             */
            void merge_triggers(qumis_program_t& pti, qumis_program_t& pto)
            {
            }

            /**
             * process concurrent triggers
             */
            void process_concurrent_triggers()
            {
               if (qumis_instructions.empty())
                  return;
               
               // find parallel sections
               qumis_program_t              ps; 
               std::vector<qumis_program_t> parallel_sections;
               ps.push_back(qumis_instructions[0]);
               size_t st = qumis_instructions[0]->start;
               // group by start time
               if (verbose) println("clustering concurent instructions..."); 
               for (size_t i=1; i<qumis_instructions.size(); ++i)
               {
                  if (qumis_instructions[i]->start != st)
                  {
                     parallel_sections.push_back(ps);
                     ps.clear();
                     ps.push_back(qumis_instructions[i]);
                  }
                  else
                  {
                     // continue within the parallel section
                     ps.push_back(qumis_instructions[i]);
                  }
                  st = qumis_instructions[i]->start;
               }
               // print prallel sections
               #if 0
               for (qumis_program_t p : parallel_sections)
               {
                  println("[+] parallel section : ");
                  for (qumis_instruction * instr : p)
                     println("\t(" << instr->start << ") : " << instr->code());
               }
               #endif 
               // detect parallel triggers
               if (verbose) println("detecting concurent triggers..."); 
               for (qumis_program_t p : parallel_sections)
               {
                  qumis_program_t triggers;
                  for (qumis_instruction * instr : p)
                     if (instr->instruction_type == __qumis_trigger__)
                        triggers.push_back(instr);
                  // process triggers ... 
                  if (triggers.size() < 2)
                     continue;
                  // println("[+] merging triggers... ");
                  std::sort(triggers.begin(), triggers.end(), triggers_comparator);
                  qumis_program_t merged_triggers;
                  size_t prev_duration = 0;

                  if (verbose) println("merging and splitting concurent triggers..."); 
                  for (size_t i=0; i<triggers.size(); ++i)
                  {
                     if (prev_duration == triggers[i]->duration)
                        continue;  // already merged with the previous trigger
                     triggers[i]->duration -= prev_duration;
                     triggers[i]->start    += prev_duration;
                     prev_duration          = triggers[i]->duration;
                     codeword_t codeword = ((trigger *)triggers[i])->codeword;
                     for (size_t j=i+1; j<triggers.size(); ++j)
                        codeword |= ((trigger*)triggers[j])->codeword;
                     ((trigger *)triggers[i])->codeword = codeword;
                     merged_triggers.push_back(triggers[i]);
                  }

                  // update parallel section with merged triggers
                  for (qumis_instruction * instr : p)
                     if (instr->instruction_type != __qumis_trigger__)
                        merged_triggers.push_back(instr);

                  // print merged triggers
                  #if 0
                  println(" ---> merged triggers: ");
                  for (qumis_instruction * mt : merged_triggers)
                     println("\t + [" << mt->start << "] : " << mt->code() );
                  #endif

                  p.swap(merged_triggers);

                  std::sort(p.begin(), p.end(), qumis_comparator);
               }

               if (verbose) println("updating qumis program..."); 
               qumis_instructions.clear();
               for (qumis_program_t p : parallel_sections)
                  for (qumis_instruction * instr : p)
                     qumis_instructions.push_back(instr);
            }


            /**
             * compensate for latencies
             */
            void compensate_latency()
            {
               if (verbose) println("latency compensation...");
               for (qumis_instruction * instr : qumis_instructions)
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

               std::vector<size_t>           hw_res_av(__trigger_width__+__awg_number__,0);
               std::vector<size_t>           qu_res_av(num_qubits,0);
               std::vector<operation_type_t> hw_res_op(__trigger_width__+__awg_number__,__none__);
               std::vector<operation_type_t> qu_res_op(num_qubits,__none__);

               for (qumis_instruction * instr : qumis_instructions)
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
               if (qumis_instructions.empty())
               {
                  println("[!] warning : empty qumis code : not traces to dump !");
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

               for (qumis_instruction * instr : qumis_instructions)
               {
                  instruction_traces_t trs = instr->trace();
                  for (instruction_trace_t t : trs)
                     diagram.add_trace(t);
               }

               diagram.dump(ql::utils::get_output_dir() + "/trace.dat");

            }


         private:

            /**
             * emit qasm code
             */
            void emit_eqasm()
            {
               if (verbose) println("compiling eqasm...");
               eqasm_code.clear();
               eqasm_code.push_back("wait 1");       // add wait 1 at the begining
               eqasm_code.push_back("mov r14, 0");   // 0: infinite loop
               eqasm_code.push_back("start:");       // label
               size_t t = 0;
               size_t i = 0;
               for (qumis_instruction * instr : qumis_instructions)
               {
                  size_t start = instr->start;
                  size_t dt = start-t;
                  if (dt)
                  {
                     eqasm_code.push_back("wait "+std::to_string(dt));
                     t = start;
                  }
                  #if 0
                  else
                  {
                     if (i)
                     {
                        println("[+] concurrent instructions : ");
                        println("\t1: " << qumis_instructions[i-1]->code());
                        println("\t2: " << qumis_instructions[i]->code());
                     }
                  }
                  #endif
                  eqasm_code.push_back(instr->code());
                  i++;
               }
               eqasm_code.push_back("wait "+std::to_string(qumis_instructions.back()->duration));
               eqasm_code.push_back("beq r14, r14 start");  // loop
               println("compilation done.");
            }

            /**
             * process pulse
             */
            void process_pulse(json& j_params, size_t duration, operation_type_t type, size_t latency, qubit_set_t& qubits, std::string& qasm_label)
            {
               // println("processing pulse instruction...");
               // check for hardware configuration integrity
               if (j_params["codeword"].is_null())
                  throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while processing pulse instruction : 'codeword' for instruction '"+qasm_label+"' is not specified !",false);
               if (j_params["awg_nr"].is_null())
                  throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while processing pulse instruction : 'awg_nr' for instruction '"+qasm_label+"' is not specified !",false);

               size_t codeword = j_params["codeword"];
               size_t awg_nr   = j_params["awg_nr"];

               if (awg_nr >= __awg_number__)
                  throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while processing pulse instruction : 'awg_nr' for instruction '"+qasm_label+"' is out of range !",false);
               // println("\tcodeword: " << codeword);
               // println("\tawg     : " << awg_nr);
               pulse * p = new pulse(codeword,awg_nr,duration,type,latency);
               p->used_qubits = qubits;
               p->qasm_label  = qasm_label;
               qumis_instructions.push_back(p);
            }

            /**
             * process codeword trigger
             */
            void process_codeword_trigger(json& j_params, size_t duration, operation_type_t type, size_t latency, qubit_set_t& qubits, std::string& qasm_label)
            {
               // println("processing codeword trigger instruction...");
               // check for hardware configuration integrity
               if (j_params["codeword_ready_bit"].is_null())
                  throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while processing codeword trigger : 'codeword_ready_bit' for instruction '"+qasm_label+"' is not specified !",false);
               if (j_params["codeword_ready_bit_duration"].is_null())
                  throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while processing codeword trigger : 'codeword_ready_bit_duration' for instruction '"+qasm_label+"' is not specified !",false);
               if (j_params["codeword_bits"].is_null())
                  throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while processing codeword trigger : 'codeword_bits' for instruction '"+qasm_label+"' is not specified !",false);

               size_t codeword_ready_bit           = j_params["codeword_ready_bit"];
               size_t codeword_ready_bit_duration  = __ns_to_cycle((size_t)j_params["codeword_ready_bit_duration"]);
               // size_t bit_nr                       = j_params["codeword_bits"].size();
               std::vector<size_t> bits            = j_params["codeword_bits"];

               // println("\tcodeword_ready_bit          : " << codeword_ready_bit);
               // println("\tcodeword_ready_bit_duration : " << codeword_ready_bit_duration);
               // println("\tbit_nr                      : " << bit_nr);

               if (codeword_ready_bit > (__trigger_width__-1))
               {
                  throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while processing codeword trigger : 'ready_bit' of instruction '"+qasm_label+"' is out of range !",false);
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

               codeword_trigger * instr = new codeword_trigger(main_codeword_trigger, duration, codeword_ready_bit, codeword_ready_bit_duration, type, latency, qasm_label);

               instr->used_qubits = qubits;
               instr->qasm_label  = qasm_label;

               // println("\tcode: " << instr->code());

               qumis_instructions.push_back(instr);
            }


            /**
             * process pulse trigger
             */
            void process_pulse_trigger(json& j_params, size_t duration, operation_type_t type, size_t latency, qubit_set_t& qubits, std::string& qasm_label)
            {
               // println("processing codeword trigger instruction...");
               // check for hardware configuration integrity
               if (j_params["codeword"].is_null())
                  throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while processing pulse trigger : 'codeword' for instruction '"+qasm_label+"' is not specified !",false);
               if (j_params["trigger_channel"].is_null())
                  throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while processing trigger channel : 'trigger_channel' for instruction '"+qasm_label+"' is not specified !",false);

               size_t cw              = j_params["codeword"];
               size_t trigger_channel = j_params["trigger_channel"];

               pulse_cw_t codeword = cw;  

               println("\ttrigger channel    : " << trigger_channel);
               println("\tcodeword           : " << codeword.to_ulong());

               if (trigger_channel > (__trigger_width__-1))
               {
                  throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while processing codeword trigger : 'trigger_channel' of instruction '"+qasm_label+"' is out of range !",false);
               }

               // ready trigger
               // codeword_t ready_codeword = 0;
               // ready_codeword.set(codeword_ready_bit);
               // trigger * ready_trigger = new trigger(ready_codeword_trigger, codeword_ready_bit_duration, type);
               // println("\t code (r_trigger): " << ready_trigger->code() );

               // pulse trigger
               pulse_trigger * instr = new pulse_trigger(codeword, trigger_channel, duration, type, latency, qasm_label);

               instr->used_qubits = qubits;
               instr->qasm_label  = qasm_label;

               // println("\tcode: " << instr->code());

               qumis_instructions.push_back(instr);
            }



            /**
             * process readout
             */
            void process_measure(json& j_params, std::string instr, size_t duration, operation_type_t type, size_t latency, qubit_set_t& qubits, std::string& qasm_label)
            {
               // println("processing measure instruction...");
               qumis_instruction * qumis_instr;
               if (instr == "trigger")
               {
                  // check for hardware configuration integrity
                  if (j_params["trigger_bit"].is_null())
                     throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while processing measure instruction : 'trigger_bit' for instruction '"+qasm_label+"' is not specified !",false);
                  if (j_params["trigger_duration"].is_null())
                     throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while processing measure instruction : 'trigger_duration' for instruction '"+qasm_label+"' is not specified !",false);

                  size_t trigger_bit       = j_params["trigger_bit"];
                  size_t trigger_duration  = __ns_to_cycle((size_t)j_params["trigger_duration"]);

                  // println("\ttrigger bit      : " << trigger_bit);
                  // println("\ttrigger duration : " << trigger_duration);
                  if (trigger_bit > (__trigger_width__-1))
                  {
                     //println("[x] error while processing the 'readout' instruction : invalid trigger bit.");
                     throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while processing measure instruction '"+qasm_label+"' : invalid trigger bit (out of range) !",false);
                  }
                  codeword_t cw = 0;
                  cw.set(trigger_bit);
                  qumis_instr = new trigger(cw, trigger_duration, __measurement__, latency);
                  qumis_instr->used_qubits = qubits;
                  qumis_instr->qasm_label  = qasm_label;
                  measure * m = new measure(qumis_instr, duration,latency);
                  m->used_qubits = qubits;
                  m->qasm_label  = qasm_label;
                  qumis_instructions.push_back(m);
               }
               else
               {
                  println("[x] error while processing the 'readout' instruction : only trigger-based implementation is supported !");
                  throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while processing the '"+qasm_label+"' instruction : only trigger-based implementation is supported !",false);
               }
               // println("measure instruction processed.");
            }


            /**
             * process trigger
             */
            void process_trigger(json& j_params, std::string instr, size_t duration, operation_type_t type, size_t latency, qubit_set_t& qubits, std::string& qasm_label)
            {
               // println("processing trigger instruction...");
               qumis_instruction * trig;

               // check for hardware configuration integrity
               if (j_params["trigger_bit"].is_null())
                  throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while processing trigger instruction : 'trigger_bit' for instruction '"+qasm_label+"' is not specified !",false);
               if (j_params["trigger_duration"].is_null())
                  throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while processing trigger instruction : 'trigger_duration' for instruction '"+qasm_label+"' is not specified !",false);

               size_t trigger_bit       = j_params["trigger_bit"];
               size_t trigger_duration  = __ns_to_cycle((size_t)j_params["trigger_duration"]);

               // println("\ttrigger bit      : " << trigger_bit);
               // println("\ttrigger duration : " << trigger_duration);
               if (trigger_bit > (__trigger_width__-1))
               {
                  // println("[x] error while processing the 'trigger' instruction : invalid trigger bit.");
                  throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while processing trigger instruction '"+qasm_label+"' : invalid trigger bit (out of range) !",false);
               }
               codeword_t cw = 0;
               cw.set(trigger_bit);
               trig = new trigger(cw, trigger_duration, __measurement__, latency);
               trig->used_qubits = qubits;
               trig->qasm_label  = qasm_label;
               qumis_instructions.push_back(trig);
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

