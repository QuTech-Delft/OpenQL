/**
 * @file   cbox_eqasm_compiler.h
 * @date   07/2017
 * @author Nader Khammassi
 * @brief  cbox eqasm compiler implementation
 */

#ifndef QL_CBOX_EQASM_COMPILER_H
#define QL_CBOX_EQASM_COMPILER_H

#include <kernel.h>
#include <platform.h>
#include <eqasm_compiler.h>
#include <arch/cbox/qumis.h>
#include <utils.h>

// eqasm code : set of qumis instructions
typedef std::vector<ql::arch::qumis_instr_t> eqasm_t;

namespace ql
{
   namespace arch
   {

      typedef std::pair<std::string, size_t> sch_qasm_t;
      typedef std::pair<double,size_t>       segment_t;
      typedef std::vector<segment_t>         waveform_t;

      typedef enum __phase_t
      { __initialization__ , __manip__ , __readout__ } phase_t;

      typedef std::pair<phase_t,size_t>  timed_phase_t;
      typedef std::vector<timed_phase_t> timed_phases_t;


      /**
       * tqasm comparator
       */
      static bool tqasm_comparator(sch_qasm_t q1, sch_qasm_t q2)
      {
         return (q1.second < q2.second);
      }

      /**
       * cbox eqasm compiler
       */
      class cbox_eqasm_compiler : public eqasm_compiler
      {
         public:

            qumis_program_t qumis_instructions;
            size_t          num_qubits;
            size_t          ns_per_cycle;
            size_t          max_latency = 0;
            size_t          total_exec_time = 0;
            size_t          buffer_matrix[__operation_types_num__][__operation_types_num__];
            size_t          iterations;  // loop iterations
            eqasm_t         timed_eqasm_code;

            #define __ns_to_cycle(t) ((size_t)t/(size_t)ns_per_cycle)

         public:

            /*
             * compile qasm to qumis
             */
            void compile(quantum_program* programp, const ql::quantum_platform& platform)
            {
                DOUT("Compiling " << programp->kernels.size() << " kernels to generate CBOX eQASM ... ");
                std::string unique_name = programp->unique_name;
        
                IOUT("fusing quantum kernels...");
                for(auto &kernel : programp->kernels)
                {
                    ql::circuit     fused;
                    ql::circuit& kc = kernel.get_circuit();
                    for(size_t i=0; i<kernel.iterations; i++)
                    {
                        fused.insert(fused.end(), kc.begin(), kc.end());
                    }

                    try
                    {
                        IOUT("compiling eqasm code ...");
                        compile_circuit(unique_name, fused, platform);
                    }
                    catch (ql::exception &e)
                    {
                        EOUT("[x] error : eqasm_compiler.compile() : compilation interrupted due to fatal error.");
                        throw e;
                    }

                    IOUT("writing eqasm code to '" << ( ql::options::get("output_dir") + "/" + unique_name+".asm"));
                    write_eqasm( ql::options::get("output_dir") + "/" + unique_name + ".asm");

                    IOUT("writing traces to '" << ( ql::options::get("output_dir") + "/trace.dat"));
                    write_traces( ql::options::get("output_dir") + "/trace.dat");
                }
            }

        private:
            void compile_circuit(std::string prog_name, ql::circuit& c, const ql::quantum_platform& platform)
            {
               IOUT("[-] compiling qasm code ...");
               if (c.empty())
               {
                  EOUT("[-] empty circuit, eqasm compilation aborted !");
                  return;
               }
               IOUT("[-] loading circuit (" <<  c.size() << " gates)...");

               iterations = 0;

               try
               {
                  iterations = platform.hardware_settings["iterations"];
               }
               catch (json::exception &e)
               {
                  // don't throw exception : iterations is a non-mondatory field
                  // throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while reading hardware settings : parameter '"+params[p-1]+"'\n\t"+ std::string(e.what()),false);
                  iterations = 0;
               }

               IOUT("[-] iterations : " << iterations);

               eqasm_t eqasm_code;
               // ql::instruction_map_t& instr_map = platform.instruction_map;
               const json& instruction_settings       = platform.instruction_settings;

               std::string params[] = { "qubit_number", "cycle_time", "mw_mw_buffer", "mw_flux_buffer", "mw_readout_buffer", "flux_mw_buffer",
                  "flux_flux_buffer", "flux_readout_buffer", "readout_mw_buffer", "readout_flux_buffer", "readout_readout_buffer" };
               size_t p = 0;

               IOUT("[-] loading hardware seetings...");

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
               catch (json::exception &e)
               {
                  throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while reading hardware settings : parameter '"+params[p-1]+"'\n\t"+ std::string(e.what()),false);
               }

               /*
                  println("[=] buffer matrix : ");
                  for (size_t i=0; i<__operation_types_num__; ++i)
                  for (size_t j=0; j<__operation_types_num__; ++j)
                  println("(" << i << "," << j << ") :" << buffer_matrix[i][j]);
                */

               IOUT("[-] loading instruction settings...");

               for (ql::gate * g : c)
               {
                  // COUT( g->name );
                  // COUT( g->qasm() );
                  // COUT( ql::utils::to_string(g->operands, "qubits") );
                  // std::string id = g->qasm(); // g->name;
                  std::string id = g->name;

                  str::lower_case(id);
                  str::replace_all(id,"  ","");
                  std::string qumis;
                  std::string operation;

                  IOUT("[-] loading instruction '" << id << " ...");

                  if (!instruction_settings[id].is_null())
                  {
                     if (instruction_settings[id]["qumis_instr"].is_null())
                        throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while reading hardware settings : 'qumis_instr' for instruction '"+id+"' is not specified !",false);
                     operation             = instruction_settings[id]["qumis_instr"].get<std::string>();
                     size_t duration       = __ns_to_cycle((size_t)instruction_settings[id]["duration"]);
                     size_t latency        = 0;

                     if (!instruction_settings[id]["latency"].is_null())
                        latency = __ns_to_cycle((size_t)instruction_settings[id]["latency"]);
                     else
                        throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while reading hardware settings : parameter 'latency' of instruction '"+id+"' not specified !",false);

                     max_latency = (latency > max_latency ? latency : max_latency);
                     // used qubits
                     qubit_set_t used_qubits;
                     size_t parameters = instruction_settings[id]["qubits"].size();
                     if (!instruction_settings[id]["qubits"].is_null())
                     {
                        // println("instr : " << id);
                        for (size_t i=0; i<parameters; ++i)
                        {
                           std::string qid = instruction_settings[id]["qubits"][i];
                           if (!is_qubit_id(qid))
                           {
                              EOUT("invalid qubit id in attribute 'qubits' !");
                              throw ql::exception("[x] error : ql::cbox_eqasm_compiler() : error while loading instruction '" + id + "' : attribute 'qubits' : invalid qubit id !", false);
                           }
                           used_qubits.push_back(qubit_id(qid));
                           // println("qubit id: " << qubit_id(qid));
                        }
                     }
                     // instruction type processing
                     if (instruction_settings[id]["type"].is_null())
                        throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while reading hardware settings : parameter 'type' of instruction '"+id+"' not specified !",false);
                     operation_type_t type = operation_type(instruction_settings[id]["type"]);
                     if (type == __unknown_operation__)
                     {
                        EOUT("Unknow operation type of the instruction '" << id << "' !");
                        throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while reading hardware settings : the type of instruction '"+id+"' is unknown !",false);
                     }
                     if (instruction_settings[id]["qumis_instr_kw"].is_null())
                        throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while reading hardware settings : 'qumis_instr_kw' for instruction '"+id+"' is not specified !",false);

                     std::stringstream params;

                     if (operation == "pulse")
                     {
                        // println("pulse id: " << id);
                        const json& j_params = instruction_settings[id]["qumis_instr_kw"];
                        process_pulse(j_params, duration, type, latency, g->operands, id);
                        // println("pulse code : " << qumis_instructions.back()->code());
                     }
                     else if (operation == "codeword_trigger")
                     {
                        // println("cw id: " << id);
                        const json& j_params = instruction_settings[id]["qumis_instr_kw"];
                        // process_codeword_trigger(j_params, duration, type, latency, g->operands, id);
                        process_codeword_trigger(j_params, duration, type, latency, used_qubits, id);
                     }
                     else if (operation == "pulse_trigger")
                     {
                        // println("cw id: " << id);
                        const json& j_params = instruction_settings[id]["qumis_instr_kw"];
                        // process_pulse_trigger(j_params, duration, type, latency, g->operands, id);
                        process_pulse_trigger(j_params, duration, type, latency, used_qubits, id);
                     }
                     else if (operation == "trigger_sequence")
                     {
                        // println("cw id: " << id);
                        const json& j_params = instruction_settings[id]["qumis_instr_kw"];
                        // process_trigger_sequence(j_params, duration, type, latency, g->operands, id);
                        // process_trigger_sequence(j_params, duration, type, latency, qubits, id);
                        process_trigger_sequence(j_params, duration, type, latency, used_qubits, id);
                     }
                     else if  ((operation == "trigger") && (type == __measurement__))
                     {
                        // println("measurement (trig) id: " << id);
                        const json& j_params = instruction_settings[id]["qumis_instr_kw"];
                        std::string qumis_instr = instruction_settings[id]["qumis_instr"];
                        // process_measure(j_params, qumis_instr, duration, type, latency, g->operands, id);
                        process_measure(j_params, qumis_instr, duration, type, latency, used_qubits, id);
                     }
                     else if  ((operation == "trigger"))
                     {
                        // println("trig id: " << id);
                        const json& j_params = instruction_settings[id]["qumis_instr_kw"];
                        std::string qumis_instr = instruction_settings[id]["qumis_instr"];
                        // process_trigger(j_params, qumis_instr, duration, type, latency, g->operands, id);
                        process_trigger(j_params, qumis_instr, duration, type, latency, used_qubits, id);
                     }
                     // qumis = operation + " " + params.str();
                     //println("qumis : " << qumis);
                  }
                  else
                  {
                     EOUT("cbox_eqasm_compiler : instruction '" << id << "' not supported by the target platform !");
                     throw ql::exception("[x] error : cbox_eqasm_compiler : error while reading hardware settings : instruction '"+id+"' not supported by the target platform !",false);
                  }
                  IOUT("[-] instructions loaded successfully.");
               }


               // time analysis
               total_exec_time = time_analysis();

               // reschedule
               resechedule();

               // compensate for latencies
               compensate_latency();

               // dump_instructions();

               // decompose meta-instructions
               decompose_instructions();

               // reorder instructions
               reorder_instructions();

               // split/merge concurrent triggers
               process_concurrent_triggers();

               // emit eqasm
               emit_eqasm();

               // dump timed eqasm code
               write_timed_eqasm(ql::options::get("output_dir") + "/program.tasm");

               // return eqasm_code;
            }
        public:

            /**
             * match qubit id
             */
            bool is_qubit_id(std::string& str)
            {
               if (str[0] != 'q')
                  return false;
               uint32_t l = str.length();
               if (l>=1)
               {
                  for (size_t i=1; i<l; ++i)
                     if (!str::is_digit(str[i]))
                        return false;
               }
               return true;
            }

            /**
             * return qubit id
             */
            size_t qubit_id(std::string qubit)
            {
               std::string id = qubit.substr(1);
               return (atoi(id.c_str()));
            }



            /**
             * display instruction and start time
             */
            void dump_instructions()
            {
               IOUT(" instructions dump:");
               for (qumis_instruction * instr : qumis_instructions)
               {
                  size_t t = instr->start;
                  std::cout << t << " : " << instr->code() << std::endl;
               }
            }


            /**
             * write time eqasm
             */
             void write_timed_eqasm(std::string file_name="")
             {
               std::stringstream ss;
               IOUT("writing time qumis code...");
               for (std::string l : timed_eqasm_code)
                  ss << l << '\n';
               std::string t_eqasm = ss.str();
               if (file_name == "")
                  std::cout << ss.str() << std::endl;
               else
                  utils::write_file(file_name,t_eqasm);
            }

            /**
             * decompose
             */
            void decompose_instructions()
            {
               IOUT("decomposing instructions...");
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
               DOUT("reodering instructions...");
               std::sort(qumis_instructions.begin(),qumis_instructions.end(), qumis_comparator);
            }

            /**
             * time analysis
             */
            size_t time_analysis()
            {
               DOUT("time analysis...");
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
               DOUT("clustering concurent instructions...");
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
                     // println("processing parallel section...");
                     // println(qumis_instructions[i]->start << " : " << qumis_instructions[i]->code());
                     // continue within the parallel section
                     ps.push_back(qumis_instructions[i]);
                  }
                  st = qumis_instructions[i]->start;
               }
               parallel_sections.push_back(ps);
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
               DOUT("detecting concurent triggers...");
               for (qumis_program_t& p : parallel_sections)
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

                  DOUT("merging and splitting concurent triggers...");
                  for (size_t i=0; i<triggers.size(); ++i)
                  {
                     // println("in : trigger " << i << " : " << triggers[i]->code());
                     if (prev_duration == triggers[i]->duration)
                        continue;  // already merged with the previous trigger
                     triggers[i]->duration -= prev_duration;
                     triggers[i]->start    += prev_duration;
                     prev_duration          = triggers[i]->duration;
                     codeword_t codeword = ((trigger *)triggers[i])->codeword;
                     for (size_t j=i+1; j<triggers.size(); ++j)
                        codeword |= ((trigger*)triggers[j])->codeword;
                     ((trigger *)triggers[i])->codeword = codeword;
                     // println("out: trigger " << i << " : " << triggers[i]->code());
                     merged_triggers.push_back(triggers[i]);
                  }
                  // println("=> merged triggers : ");
                  // for (qumis_instruction * instr : merged_triggers)
                     // println("\t(" << instr->start << ") : " << instr->code());

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

                  // println("=> updated parallel section : ");
                  // for (qumis_instruction * instr : p)
                     // println("\t(" << instr->start << ") : " << instr->code());
               }

               DOUT("updating qumis program...");
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
               DOUT("latency compensation...");
               for (qumis_instruction * instr : qumis_instructions)
                  instr->compensate_latency();
            }

            /**
             * optimize
             */
            void resechedule()
            {
               DOUT("instruction rescheduling...");
               DOUT("resource dependency analysis...");
               DOUT("buffer insertion...");

               std::vector<size_t>           hw_res_av(__trigger_width__+__awg_number__,max_latency);
               std::vector<size_t>           qu_res_av(num_qubits,max_latency);
               std::vector<operation_type_t> hw_res_op(__trigger_width__+__awg_number__,__none__);
               std::vector<operation_type_t> qu_res_op(num_qubits,__none__);

               std::map<std::string,size_t>   timed_qasm;
               std::vector<sch_qasm_t> qasm_schedule;

               size_t execution_time = 0;

               for (qumis_instruction * instr : qumis_instructions)
               {
                  resources_t      hw_res  = instr->used_resources;
                  qubit_set_t      qu_res  = instr->used_qubits;
                  operation_type_t type    = instr->get_operation_type();
                  size_t latest_hw = 0;
                  size_t buf_hw    = 0;
                  size_t latest_qu = 0;
                  size_t buf_qu    = 0;

                  // hw deps
                  for (size_t r=0; r<hw_res.size(); ++r)
                  {
                     if (hw_res.test(r))
                     {
                        size_t rbuf = buffer_size(hw_res_op[r],type);
                        buf_hw      = ((rbuf > buf_hw) ? rbuf : buf_hw);
                        latest_hw   = (hw_res_av[r] > latest_hw ? hw_res_av[r] : latest_hw);
                     }
                  }

                  // println("- instr : " << instr->qasm_label);

                  // qubit deps
                  for (size_t q : qu_res) // qubits used by the instr
                  {
                     // println("uq : " << q);
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

                  // update execution time
                  size_t end_time = instr->start + instr->duration;
                  execution_time = (end_time > execution_time ? end_time : execution_time);

                  // update timed qasm
                  std::map<std::string,size_t>::iterator it = timed_qasm.find(instr->qasm_label);
                  if (it == timed_qasm.end())
                     timed_qasm[instr->qasm_label] = latest+buf;
                  qasm_schedule.push_back(sch_qasm_t(instr->qasm_label,latest+buf));

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
               // sch_qasm gen
               #define __seg_unit__ (250)
               // println(">> tqasm map: ");
               // for (std::map<std::string,size_t>::iterator it=timed_qasm.begin(); it != timed_qasm.end(); it++)
               //    println("[ " << it->first << " : " << it->second << " ]");
               // println(">> qasm schedule : ");
               // for (sch_qasm_t instr : qasm_schedule)
               //    println("[" << instr.first << " : " << instr.second << "]");
               std::sort(qasm_schedule.begin(),qasm_schedule.end(),tqasm_comparator);
               // println(">> sorted qasm schedule : ");
               // for (sch_qasm_t instr : qasm_schedule)
               //   println("[" << instr.first << " : " << instr.second << "]");

               // const double init_level      = 0.1f;
               // const double operation_level = 0.2f;
               // const double readout_level   = 0.3f;

               // init sequence
               waveform_t init_wf;
               init_wf.push_back(segment_t(30*0.5* 0.004 , 1500*4));
               init_wf.push_back(segment_t(30*0.5*-0.004 , 1500*4));
               init_wf.push_back(segment_t(30*0.5*-0.0105, 100*4));
               init_wf.push_back(segment_t(0, 4000*4));
               init_wf.push_back(segment_t(30*0.5*0.002, 100*4));
               init_wf.push_back(segment_t(30*0.5*0.004, 1));  // extra 250ns
               // manip
               waveform_t manip_wf;
               manip_wf.push_back(segment_t(30*0.5*0.004 , 1500*4));
               // readout
               waveform_t readout_wf;
               readout_wf.push_back(segment_t(30*0.5*0.004, 1));  // extra 250ns
               readout_wf.push_back(segment_t(30*0.5*0.000, 1500*4));

               std::vector<waveform_t> wfs;

               timed_phases_t tps;

               phase_t p = __initialization__;
               // wfs.push_back(init_wf);
               tps.push_back(timed_phase_t(__initialization__,1));

               // println("0\t:  init");
               for (size_t i=0; i < qasm_schedule.size(); ++i)
               {
                  sch_qasm_t qi = qasm_schedule[i];
                  phase_t cp = (is_inialization(qi) ? __initialization__ : ( is_readout(qi) ? __readout__ : __manip__));
                  if (cp != p)
                  {
                     // println(qi.second << "\t: " << (cp == __initialization__ ? " init " : (cp == __readout__ ? " readout " : " manip ")));
                     tps.push_back(timed_phase_t(cp,qi.second));
                  }
                  p = cp;
               }
               // println(" timed phases: ");
               // for (auto p : tps)
               //   println("- " << p.first << " : " << p.second);

               // println(" tektronix vector: ");
               for (size_t i=0; i<tps.size()-1; ++i)
               {
                  auto p = tps[i];
                  // println("- " << p.first << " : " << (p.first == __manip__ ?  (tps[i+1].second-p.second)*ns_per_cycle/__seg_unit__ : 0));
                  // println("- " << p.first << " : " << (p.first == __manip__ ?  (tps[i+1].second-p.second) : 0));
                  if (p.first == __initialization__)
                     wfs.push_back(init_wf);
                  else if (p.first == __readout__)
                     wfs.push_back(readout_wf);
                  else if (p.first == __manip__)
                  {
                     waveform_t w = manip_wf;
                     w[0].second  = (tps[i+1].second-p.second)*ns_per_cycle/__seg_unit__;
                     wfs.push_back(w);
                  }
               }
               wfs.push_back(readout_wf);
               write_waveforms(wfs,execution_time);
            }

            /**
             * write waveforms sequence
             */
            void write_waveforms(std::vector<waveform_t>& wfs, size_t execution_time)
            {
               // std::string file_name = ql::options::get("output_dir") + "/waveforms_sequence.json";
               std::string file_name = ql::options::get("output_dir") + "/waveform_sequence.dat";
               DOUT("writing waveforms sequence to '" << file_name << "'...");

               std::stringstream js;
               js << "\n{ \n   \"execution_time\" : " << (execution_time*ns_per_cycle/1e9) << ",\n   \"segment_size\" : 300,\n   \"sequence\" : [";
               for (size_t wi=0; wi < wfs.size(); wi++)
               {
                  waveform_t w = wfs[wi];
                  for (size_t si=0; si < w.size(); si++)
                  {
                     segment_t s = w[si];
                     js << " [ " << s.first << ", " << s.second << "]";
                     if (!((si == (w.size()-1)) && (wi == (wfs.size()-1))))
                        js << ", ";
                  }
               }
               js << "]\n}";
               std::string fs = js.str();
               str::replace_all(fs,", ,",",");
               utils::write_file(file_name,fs);
            }

            /**
             * is initialization
             */
            bool is_inialization(sch_qasm_t& i)
            {
               size_t f = i.first.find("prepz");
               if (f != std::string::npos) return true;
               return false;
            }

            /**
             * is readout
             */
            bool is_readout(sch_qasm_t& i)
            {
               size_t f = i.first.find("measure");
               if (f != std::string::npos) return true;
               return false;
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
                  WOUT("Empty qumis code : not traces to dump !");
                  return;
               }

               for (size_t i=__trigger_width__; i>0; i--)
               {
                  std::string ch = "- MRK_"+std::to_string(i-1);
                  channels.push_back(ch);
               }

               for (size_t i=0; i<__awg_number__; i++)
               {
                  std::string ch = "- AWG_"+std::to_string(i);
                  channels.push_back(ch);
               }

               ql::arch::time_diagram diagram(channels,total_exec_time,4);

               for (qumis_instruction * instr : qumis_instructions)
               {
                  instruction_traces_t trs = instr->trace();
                  for (instruction_trace_t t : trs)
                     diagram.add_trace(t);
               }

               diagram.dump(ql::options::get("output_dir") + "/trace.dat");

            }


         private:

            #define __max_wait__ (32767)

            /**
             * emit qasm code
             */
            void emit_eqasm()
            {
               DOUT("compiling eqasm...");
               eqasm_code.clear();
               timed_eqasm_code.clear();
               eqasm_code.push_back("wait 1");       // add wait 1 at the begining
               eqasm_code.push_back("mov r12, 1");   // counter step
               eqasm_code.push_back("mov r13, 0");   // boundary
               if (iterations)
               eqasm_code.push_back("mov r14, "+std::to_string(iterations));   // 0: infinite loop
               eqasm_code.push_back("start:");       // label
               eqasm_code.push_back("wait 2");       // label
               size_t t = 0;
               size_t i = 0;
               for (qumis_instruction * instr : qumis_instructions)
               {
                  size_t start = instr->start;
                  size_t dt = start-t;
                  if (dt)
                  {
                     // eqasm_code.push_back("wait "+std::to_string(dt));
                     wait(dt);
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
                  timed_eqasm_code.push_back(std::to_string(t)+"\t: "+instr->code());
                  // println(t << " : " << instr->code());
                  i++;
               }
               eqasm_code.push_back("wait "+std::to_string(qumis_instructions.back()->duration));
               if (iterations)
               {
                  eqasm_code.push_back("sub r14, r14, r12");
                  eqasm_code.push_back("bne r13, r14 start");
               }
               else
                  eqasm_code.push_back("beq r13, r13 start");
               DOUT("eqasm compilation done.");
            }


            /**
             * wait instruction
             */
            void wait(size_t t)
            {
               if (t < __max_wait__)
               {
                  eqasm_code.push_back("wait "+std::to_string(t));
               }
               else
               {
                  size_t num_max_waits = t/__max_wait__;
                  size_t rest          = t%__max_wait__;
                  for (size_t i=0; i<num_max_waits; i++)
                     eqasm_code.push_back("wait "+std::to_string(__max_wait__));
                  if (rest)
                     eqasm_code.push_back("wait "+std::to_string(rest));
               }
            }


            /**
             * process pulse
             */
            void process_pulse(const json& j_params, size_t duration, operation_type_t type, size_t latency, qubit_set_t& qubits, std::string& qasm_label)
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
               p->set_used_qubits(qubits);
               p->qasm_label  = qasm_label;
               qumis_instructions.push_back(p);
            }

            /**
             * process codeword trigger
             */
            void process_codeword_trigger(const json& j_params, size_t duration, operation_type_t type, size_t latency, qubit_set_t& qubits, std::string& qasm_label)
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

               if ((codeword_ready_bit > (__trigger_width__-1)) || (codeword_ready_bit == 0))
               {
                  throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while processing codeword trigger : 'ready_bit' of instruction '"+qasm_label+"' is out of range ! (should be a value whithin [1..7])",false);
               }

               // ready trigger
               // codeword_t ready_codeword = 0;
               // ready_codeword.set(codeword_ready_bit);
               // trigger * ready_trigger = new trigger(ready_codeword_trigger, codeword_ready_bit_duration, type);
               // println("\t code (r_trigger): " << ready_trigger->code() );

               // codeword trigger
               codeword_t main_codeword_trigger = 0;
               for (size_t b : bits) main_codeword_trigger.set(7-b);
               // trigger * main_trigger = new trigger(main_codeword_trigger, duration, type);
               //println("\t code (m_trigger): " << main_trigger->code() );

               codeword_trigger * instr = new codeword_trigger(main_codeword_trigger, duration, codeword_ready_bit, codeword_ready_bit_duration, type, latency, qasm_label);

               // for (auto q : qubits) instr->used_qubits.push_back(q);
               instr->set_used_qubits(qubits);
               // instr->used_qubits = qubits;
               instr->qasm_label  = qasm_label;

               // println("\tcode: " << instr->code());

               qumis_instructions.push_back(instr);
            }


            /**
             * process pulse trigger
             */
            void process_pulse_trigger(const json& j_params, size_t duration, operation_type_t type, size_t latency, qubit_set_t& qubits, std::string& qasm_label)
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

               // println("\ttrigger channel    : " << trigger_channel);
               // println("\tcodeword           : " << codeword.to_ulong());

               if ((trigger_channel > (__trigger_width__-1)) || (trigger_channel == 0))
               {
                  throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while processing codeword trigger : 'trigger_channel' of instruction '"+qasm_label+"' is out of range ! (should be whithin [1..7])",false);
               }

               // ready trigger
               // codeword_t ready_codeword = 0;
               // ready_codeword.set(codeword_ready_bit);
               // trigger * ready_trigger = new trigger(ready_codeword_trigger, codeword_ready_bit_duration, type);
               // println("\t code (r_trigger): " << ready_trigger->code() );

               // pulse trigger
               pulse_trigger * instr = new pulse_trigger(codeword, trigger_channel, duration, type, latency, qasm_label);

               // for (auto q : qubits) instr->used_qubits.push_back(q);
               instr->set_used_qubits(qubits);
               // instr->used_qubits = qubits;
               instr->qasm_label  = qasm_label;

               // println("\tcode: " << instr->code());

               qumis_instructions.push_back(instr);
            }

            /**
             * process trigger sequence
             */
            void process_trigger_sequence(const json& j_params, size_t duration, operation_type_t type, size_t latency, qubit_set_t& qubits, std::string& qasm_label)
            {
               // println("processing codeword trigger instruction...");
               // check for hardware configuration integrity
               if (j_params["trigger_width"].is_null())
                  throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while processing trigger sequence: 'trigger_width' for instruction '"+qasm_label+"' is not specified !",false);
               if (j_params["trigger_channel"].is_null())
                  throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while processing trigger sequence: 'trigger_channel' for instruction '"+qasm_label+"' is not specified !",false);

               size_t trigger_width   = j_params["trigger_width"];
               size_t trigger_channel = j_params["trigger_channel"];

               // println("\ttrigger channel    : " << trigger_channel);
               // println("\ttrigger width      : " << trigger_width);

               if ((trigger_channel > (__trigger_width__-1)) || (trigger_channel == 0))
               {
                  throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while processing codeword trigger : 'trigger_channel' of instruction '"+qasm_label+"' is out of range ! (should be a value whithin [1..7])",false);
               }

               // trigger sequence
               trigger_sequence * instr = new trigger_sequence(trigger_channel, trigger_width, duration, type, latency, qasm_label);

               // for (auto q : qubits) instr->used_qubits.push_back(q);
               instr->set_used_qubits(qubits);
               // instr->used_qubits = qubits;
               instr->qasm_label  = qasm_label;

               // println("\tcode: " << instr->code());

               qumis_instructions.push_back(instr);
            }





            /**
             * process readout
             */
            void process_measure(const json& j_params, std::string instr, size_t duration, operation_type_t type, size_t latency, qubit_set_t& qubits, std::string& qasm_label)
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
                  if ((trigger_bit > (__trigger_width__-1)) || (trigger_bit == 0))
                  {
                     //println("[x] error while processing the 'readout' instruction : invalid trigger bit.");
                     throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while processing measure instruction '"+qasm_label+"' : invalid trigger bit (out of range, trigger should be in [1..7]) !",false);
                  }
                  codeword_t cw = 0;
                  cw.set(7-trigger_bit);
                  qumis_instr = new trigger(cw, trigger_duration, __measurement__, latency);
                  // for (auto q : qubits) qumis_instr->used_qubits.push_back(q);
                  qumis_instr->set_used_qubits(qubits);
                  // qumis_instr->used_qubits = qubits;
                  qumis_instr->qasm_label  = qasm_label;
                  measure * m = new measure(qumis_instr, duration,latency);
                  // for (auto q : qubits) m->used_qubits.push_back(q);
                  m->set_used_qubits(qubits);
                  // m->used_qubits = qubits;
                  m->qasm_label  = qasm_label;
                  qumis_instructions.push_back(m);
               }
               else
               {
                  EOUT("while processing the 'readout' instruction : only trigger-based implementation is supported !");
                  throw ql::exception("[x] error : ql::eqasm_compiler::compile() : error while processing the '"+qasm_label+"' instruction : only trigger-based implementation is supported !",false);
               }
               // println("measure instruction processed.");
            }


            /**
             * process trigger
             */
            void process_trigger(const json& j_params, std::string instr, size_t duration, operation_type_t type, size_t latency, qubit_set_t& qubits, std::string& qasm_label)
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
               cw.set(7-trigger_bit);
               trig = new trigger(cw, trigger_duration, __measurement__, latency);
               // for (auto q : qubits) trig->used_qubits.push_back(q);
               // trig->used_qubits = qubits;
               trig->qasm_label  = qasm_label;
               trig->set_used_qubits(qubits);
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

