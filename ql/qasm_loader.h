/**
 * @file    qasm_loader.h
 * @author	Nader Khammassi 
 * @date	   20-12-15
 * @brief	qasm code loader (taken from qx)	
 */

#ifndef QX_QUANTUM_CODE_LOADER
#define QX_QUANTUM_CODE_LOADER

#include <ql/openql.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <map>

#include <stdint.h>

#include <ql/str.h>
#include <ql/qx_interface.h>
#include <ql/quantum_state_loader.h>


/**
 * i/o helpers
 */

#ifndef println
#define println(x) std::cout << x << std::endl
#endif

#ifndef error
#define error(x) std::cout << "[x] error : " << x << std::endl;
#endif

// #include <core/circuit.h>
// #include <core/error_model.h>
// #include <quantum_code/token.h>
// #include <quantum_code/syntax_error_exception.h>

using namespace str;  // string utils

// #define __max_qubits__ 32

namespace qx
{

   // typedef std::vector<qx::gate *>             circuit_t;
   typedef std::map<std::string,std::string>   map_t;
   typedef std::vector<qx::circuit *>          circuits_t;
   typedef std::vector<qx::quantum_state_t *>  quantum_states_t;

   /**
    * quantum_code parser
    */
   class qasm_loader
   {

      private:

         // qasm file name
         std::string   file_name;
         std::string   path;

         // qubits count
         uint32_t      qubits_count;

         // current line
         int           line_index;

         // parsing error/success
         bool          parsed_successfully;
         bool          syntax_error;
         bool          semantic_error;

         // macro  definitions
         map_t         definitions;

         // circuits

         circuits_t                         circuits;
         std::vector<ql::quantum_kernel *>  kernels;

         quantum_states_t                   quantum_states;
         str::strings                       quantum_state_files;

         // noise
         // double                             phase_noise;
         // double                             rotation_noise;

         // decoherence
         // double                             decoherence; 

         // error model
         qx::error_model_t                  error_model;
         double                             error_probability;


      public:

         /**
          * \brief
          *    qasm_loader constructor
          */
         qasm_loader(std::string file_name) : file_name(file_name), qubits_count(0), parsed_successfully(false), 
                                              syntax_error(false), semantic_error(false), /* phase_noise(0), rotation_noise(0), decoherence(0), */
                                              error_model(qx::__unknown_error_model__), error_probability(0)
         {
            size_t last = file_name.find_last_of('/');
            if (last != std::string::npos)
            {
               path = file_name.substr(0,last+1);
            }
            else 
               path = "";
         }


         /**
          * \brief
          *    parse the quantum code file
          */
         int parse(bool exit_on_error=true)
         {
            line_index   = 0;
            syntax_error = false;
            println("[-] loading quantum_code file '" << file_name << "'...");
            std::ifstream stream(file_name.c_str());
            if (stream)
            {
               char buf[2048];
               while(stream.getline(buf, 2048))
               {
                  line_index++;
                  std::string line = buf;
                  if (line.length()>0)
                     process_line(line);
                  if (syntax_error)
                     break;
               }
               stream.close();
               if (syntax_error || semantic_error)
               {
                  if (exit_on_error)
                     exit(-1);
                  else
                  {
                     println("[+] failed to load the code : code contains errors. ");
                     return -1;
                  }

               }
               parsed_successfully = true;
               println("[+] code loaded successfully. ");
               return 0;
            }
            else 
            {
               error("cannot open file " << file_name);
               if (exit_on_error)
                  exit(-1);
               else
                  return -1;
            }
         }

         /**
          * destructor
          */
         virtual ~qasm_loader()
         {
            // clean();
         }


         /**
          * \brief print circuits
          */
         // void dump()
         // {
         //    if (!parsed_successfully)
         //    {
         //       println("[!] no circuits: quantum code file not loaded yet !");
         //       return;
         //    }
         //    for (uint32_t i=0; i<circuits.size(); ++i)
         //       circuits[i]->dump();
         // }

         /**
          * \brief execute circuits
          */
         // void execute(qx::qu_register& reg, uint32_t address=0, bool verbose=false, bool binary=false)
         // {
         //    if (!parsed_successfully)
         //    {
         //       println("[x] no circuits to execute: you must load quantum code file first !");
         //       return;
         //    }
         //    if (address > circuits.size())
         //    {
         //       println("[x] invalid circuit pointer (" << address << ") , there is only " << circuits.size() << " sub-circuits !");
         //       return;
         //    }
         //    for (uint32_t i=address; i<circuits.size(); ++i)
         //       circuits[i]->execute(reg,verbose,binary);
         // }

         /**
          * qubits
          */
         uint32_t qubits()
         {
            return qubits_count;
         }

         #define print_syntax_error(err) \
         {\
            std::cout << "[x] syntax error at line " << line_index << " : " << err << std::endl; \
            std::cout << "   +--> code: \"" << original_line << "\"" << std::endl; \
            syntax_error = true;\
            return 1;\
         }


         #define print_semantic_error(err) \
         {\
            std::cout << "[x] semantic error at line " << line_index << " : " << err << std::endl; \
            std::cout << "   +--> code: \"" << original_line << "\"" << std::endl; \
            semantic_error = true;\
            return 1;\
         }

      private:

         /**
          * \brief check if the circuit label is valid
          */
         bool is_label(std::string& str)
         {
            if (!is_dot(str[0]))
               return false;
            for (uint32_t i=1; i<str.size(); ++i)
            {
               if ((!is_letter(str[i])) && (!is_digit(str[i])) && (str[i]!='(') && (str[i]!=')'))
                  return false;
            }
            return true;
         }

         /**
          * \brief translate user-defined qubit/bit name to qubit/bit identifier
          */
         void translate(std::string& str)
         {
            // search in the qubit map first
            map_t::iterator it = definitions.find(str);
            if (it != definitions.end())
            {
               std::string id = it->second;
               //println(" $ translate : " << str << " -> " << id);
               str.replace(0,str.size(),id);
            }
         }

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
                  if (!is_digit(str[i]))
                     return false;
            }
            return true;
         }

         /**
          * \brief retrieve qubit number <N> from a string "qN"
          */
         uint32_t qubit_id(std::string& str)
         {
            std::string& original_line = str;
            std::string  qubit;
            // if (str[0] != 'q')
            if (!is_qubit_id(str))
            {
               // search in the qubit map first
               map_t::iterator it = definitions.find(str);
               if (it != definitions.end())
               {
                  qubit = it->second;
                  // println(" def[" << str << "] -> " << qubit);
                  if (qubit[0] != 'q')
                     print_syntax_error(" invalid qubit identifier : qubit name not defined, you should use 'map' to name qubit before using it !");
                  str = qubit;
               }
               else
                  print_syntax_error(" invalid qubit identifier !");
            }
            std::string id = str.substr(1);
            for (int i=0; i<id.size(); ++i)
            {
               if (!is_digit(id[i]))
                  print_syntax_error(" invalid qubit identifier !" << "(id:" << id << ")");
            }
            // println(" qubit id -> " << id);
            return (atoi(id.c_str()));
         }

         /**
          * \brief retrieve bit number <N> from an identifer <bN>
          */
         uint32_t bit_id(std::string& str)
         {
            std::string& original_line = str;
            if (str[0] != 'b')
               print_syntax_error(" invalid bit identifier !");
            std::string id = str.substr(1);
            for (int i=0; i<id.size(); ++i)
            {
               if (!is_digit(id[i]))
                  print_syntax_error(" invalid qubit identifier !");
            }
            return (atoi(id.c_str()));
         }

         /**
          * \return 
          *    a pointer to the current sub-circuit 
          */
         qx::circuit * current_sub_circuit(uint32_t qubits_count)
         {
            if (circuits.size())
            {
               return circuits.back();
            }
            circuits.push_back(new qx::circuit(qubits_count, "default"));
            return circuits.back();
         }


         uint32_t iterations(std::string label)
         {
            std::string& original_line = label;
            int32_t i1 = label.find('('); 
            int32_t i2 = label.find(')'); 
            // println("i1=" << i1 << ", i2=" << i2);
            if (i1 == -1)
               return 1;
            if (i2 == -1) 
               print_semantic_error(" invalid sub-circuit definition !");
            if (!(i2 > (i1+1)))
               print_semantic_error(" invalid sub-circuit's iteration count !");
            // find iteration count
            std::string it = label.substr(i1+1,i2-i1-1);
            // println("it = " << it);
            for (uint32_t i=0; i<it.length(); ++i)
            {
               if (!is_digit(it[i]))
                  print_semantic_error(" invalid sub-circuit's iteration count !");
            }
            return atoi(it.c_str());
         }

         std::string circuit_name(std::string& label)
         {
            std::string name = label.substr(1,label.find('(')-1);
            return name;
         }

         /**
          * \brief 
          *   process line
          */
         int32_t process_line(std::string& line, qx::parallel_gates * pg=0)
         {
            // println("processing line " << line_index << " ...");
            format_line(line);
            if (str::is_empty(line))
               return 0;
            if (line[0] == '#') // skip comments
            {
               // println("   comment.");
               return 0;
            }
            std::string original_line(line);
            remove_comment(line,'#');  // remove inline comment
            format_line(line);
            if (is_label(line))
            {
               if (qubits_count==0) 
               {
                  print_semantic_error(" qubits number must be defined first !");
               }
               else
               {
                  // println("label : new circuit '" << line << "' created.");
                  // circuits.push_back(new qx::circuit(qubits_count, line.substr(1)));
                  circuits.push_back(new qx::circuit(qubits_count, circuit_name(line), iterations(line)));
                  return 0;
               }
            }

            strings words = word_list(line, " ");
            // process display commands
            if (words.size() == 1)
            {
               if (words[0] == "display")
                  current_sub_circuit(qubits_count)->add(new qx::display());
               else if (words[0] == "display_binary")
                  current_sub_circuit(qubits_count)->add(new qx::display(true));
               else if (words[0] == "measure")
                  current_sub_circuit(qubits_count)->add(new qx::measure());
               else
                  print_syntax_error("unknown commad !");
               return 0;
            }

            // this do not handle correctly malformed code (ex : parallel gate 
            // definition when qubits number is not yet defined) 
            if (words.size() != 2) 
               if (pg != 0)
                  print_syntax_error("malformed code !");


            if (words[0] == "qubits")    // qubit definition
            {
               if (qubits_count) 
                  print_syntax_error("qubits number already defined !");

               qubits_count = atoi(words[1].c_str());

               // if ((qubits_count > 0) && (qubits_count < __max_qubits__))
               //{
               // println(" => qubits number: " << qubits_count);
               //}
               // else
               // print_syntax_error(" too much qubits (" << qubits_count << ") !");
            }
            else if (qubits_count == 0)
            {
               print_semantic_error(" qubits number must be defined first !");
            }
            else if (words[0] == "map") // definitions
            {
               strings params = word_list(words[1],",");
               uint32_t q     = 0;
               if (params[0][0] == 'q') 
                  q = qubit_id(params[0]);
               else if (params[0][0] == 'b')
                  q = bit_id(params[0]);
               else
                  print_semantic_error(" invalid qubit/bit identifier !");
               std::string qubit = params[0];
               std::string name  = params[1];
               if (q > (qubits_count-1))
                  print_semantic_error(" qubit out of range !");
               definitions[name] = qubit;
               // println(" => map qubit " << name << " to " << qubit);
            }
            else if (words[0] == "load_state")
            {
               std::string file = path+words[1];
               replace_all(file,"\"","");
               // println("[+] loading quantum state from '" << file << "' ...");
               qx::quantum_state_loader qsl(file,qubits_count);
               qsl.load();
               quantum_state_files.push_back(file);
               quantum_states.push_back(qsl.get_quantum_state());
               current_sub_circuit(qubits_count)->add(new qx::prepare(qsl.get_quantum_state()));
            }
            /**
             * error model
             */
            else if (words[0] == "error_model")   // operational errors
            {
               strings params = word_list(words[1],",");
               if (params.size() != 2)
                  print_syntax_error(" error mode should be specified according to the following syntax: 'error_model depolarizing_channel,0.01' ");
               if (params[0] == "depolarizing_channel")
               {
                  error_model = qx::__depolarizing_channel__;
                  error_probability = atof(params[1].c_str());
                  println("[!] noise simulation enabled : error model =" << params[0].c_str() << ", error probability =" << error_probability << ")");
               }
               else
                  print_semantic_error(" unknown error model !");
            }
            /**
             * noise 
             */
            else if (words[0] == "noise")   // operational errors
            {
               strings params = word_list(words[1],",");
               println(" => noise (theta=" << params[0].c_str() << ", phi=" << params[1].c_str() << ")");
            } 
            else if (words[0] == "decoherence")   // decoherence
            {
               println(" => decoherence (dt=" << words[1] << ")");
            }
            else if (words[0] == "qec")   // decoherence
            {
               println(" => quantum error correction scheme = " << words[1]);
            }
            /**
             * parallel gates
             */
            else if ((words[0] == "{") && (words[words.size()-1] == "}"))
            {
               std::string pg_line = line;
               replace_all(pg_line,"{","");
               replace_all(pg_line,"}","");
               strings gates = word_list(pg_line,"|");
               qx::parallel_gates * _pgs = new qx::parallel_gates();
               for (size_t i=0; i<gates.size(); ++i)
               {
                  // println(gates[i]);
                  process_line(gates[i],_pgs);
               }
               current_sub_circuit(qubits_count)->add(_pgs);
            }
            /**
             * sequential gates
             */
            else if (words[0] == "h")    // hadamard gate
            {
               uint32_t q = qubit_id(words[1]); // atoi(words[1].c_str());
               if (q > (qubits_count-1))
                  print_semantic_error(" target qubit out of range !");
               // println(" => hadamard gate on: " << q);
               if (pg) 
                  pg->add(new qx::hadamard(q));
               else
                  current_sub_circuit(qubits_count)->add(new qx::hadamard(q));
            } 
            else if (words[0] == "i")    // hadamard gate
            {
               uint32_t q = qubit_id(words[1]); // atoi(words[1].c_str());
               if (q > (qubits_count-1))
                  print_semantic_error(" target qubit out of range !");
               // println(" => hadamard gate on: " << q);
               if (pg) 
                  pg->add(new qx::id(q));
               else
                  current_sub_circuit(qubits_count)->add(new qx::hadamard(q));
            }
            else if (words[0] == "rx90")    // rx90 
            {
               uint32_t q = qubit_id(words[1]); // atoi(words[1].c_str());
               if (q > (qubits_count-1))
                  print_semantic_error(" target qubit out of range !");
               // println(" => hadamard gate on: " << q);
               if (pg) 
                  pg->add(new qx::rx(q,M_PI/2));
               else
                  current_sub_circuit(qubits_count)->add(new qx::rx(q,M_PI/2));
            }
            else if (words[0] == "mrx90")    // mrx90 
            {
               uint32_t q = qubit_id(words[1]); // atoi(words[1].c_str());
               if (q > (qubits_count-1))
                  print_semantic_error(" target qubit out of range !");
               // println(" => hadamard gate on: " << q);
               if (pg) 
                  pg->add(new qx::rx(q,-M_PI/2));
               else
                  current_sub_circuit(qubits_count)->add(new qx::rx(q,-M_PI/2));
            }
            else if (words[0] == "rx180")    // rx90 
            {
               uint32_t q = qubit_id(words[1]); // atoi(words[1].c_str());
               if (q > (qubits_count-1))
                  print_semantic_error(" target qubit out of range !");
               // println(" => hadamard gate on: " << q);
               if (pg) 
                  pg->add(new qx::rx(q,M_PI));
               else
                  current_sub_circuit(qubits_count)->add(new qx::rx(q,M_PI));
            }
            else if (words[0] == "ry90")    // rx90 
            {
               uint32_t q = qubit_id(words[1]); // atoi(words[1].c_str());
               if (q > (qubits_count-1))
                  print_semantic_error(" target qubit out of range !");
               // println(" => hadamard gate on: " << q);
               if (pg) 
                  pg->add(new qx::ry(q,M_PI/2));
               else
                  current_sub_circuit(qubits_count)->add(new qx::ry(q,M_PI/2));
            }
            else if (words[0] == "mry90")    // rx90 
            {
               uint32_t q = qubit_id(words[1]); // atoi(words[1].c_str());
               if (q > (qubits_count-1))
                  print_semantic_error(" target qubit out of range !");
               // println(" => hadamard gate on: " << q);
               if (pg) 
                  pg->add(new qx::ry(q,-M_PI/2));
               else
                  current_sub_circuit(qubits_count)->add(new qx::ry(q,-M_PI/2));
            }
            else if (words[0] == "ry180")    // rx90 
            {
               uint32_t q = qubit_id(words[1]); // atoi(words[1].c_str());
               if (q > (qubits_count-1))
                  print_semantic_error(" target qubit out of range !");
               // println(" => hadamard gate on: " << q);
               if (pg) 
                  pg->add(new qx::ry(q,M_PI));
               else
                  current_sub_circuit(qubits_count)->add(new qx::ry(q,M_PI));
            }

            else if (words[0] == "cnot") // cnot gate
            {
               strings params = word_list(words[1],",");
               uint32_t cq = qubit_id(params[0]);
               uint32_t tq = qubit_id(params[1]);
               if (cq > (qubits_count-1))
                  print_semantic_error(" control qubit out of range !");
               if (tq > (qubits_count-1))
                  print_semantic_error(" target qubit out of range !");
               // println(" => cnot gate : ctrl_qubit=" << cq << ", target_qubit=" << tq);
               if (pg) 
                  pg->add(new qx::cnot(cq,tq));
               else
                  current_sub_circuit(qubits_count)->add(new qx::cnot(cq,tq));
            } 
            else if (words[0] == "swap") // cnot gate
            {
               strings params = word_list(words[1],",");
               uint32_t q1 = qubit_id(params[0]);
               uint32_t q2 = qubit_id(params[1]);
               if ((q1 > (qubits_count-1)) || (q1 > (qubits_count-1)))
                  print_semantic_error(" target qubit out of range !");
               // println(" => swap gate : qubit_1=" << q1 << ", qubit_2=" << q2); 
               if (pg) 
                  pg->add(new qx::swap(q1,q2));
               else
                  current_sub_circuit(qubits_count)->add(new qx::swap(q1,q2));
            } 

            /**
             * controlled phase shift
             */
            else if (words[0] == "cr") 
            {
               strings params = word_list(words[1],",");
               uint32_t q1 = qubit_id(params[0]);
               uint32_t q2 = qubit_id(params[1]);
               if ((q1 > (qubits_count-1)) || (q1 > (qubits_count-1)))
                  print_semantic_error(" target qubit out of range !");
               // println(" => controlled phase shift gate : ctrl_qubit=" << q1 << ", target_qubit=" << q2); 
               if (pg) 
                  pg->add(new qx::ctrl_phase_shift(q1,q2));
               else
                  current_sub_circuit(qubits_count)->add(new qx::ctrl_phase_shift(q1,q2));
            } 
            /**
             * cphase 
             */
            else if (words[0] == "cphase") 
            {
               strings params = word_list(words[1],",");
               uint32_t q1 = qubit_id(params[0]);
               uint32_t q2 = qubit_id(params[1]);
               if ((q1 > (qubits_count-1)) || (q1 > (qubits_count-1)))
                  print_semantic_error(" target qubit out of range !");
               // println(" => controlled phase shift gate : ctrl_qubit=" << q1 << ", target_qubit=" << q2); 
               if (pg) 
                  pg->add(new qx::cphase(q1,q2));
               else
                  current_sub_circuit(qubits_count)->add(new qx::cphase(q1,q2));
            }

            /**
             * pauli gates
             */
            else if (words[0] == "x")   // x gate
            {
               uint32_t q = qubit_id(words[1]);
               if (q > (qubits_count-1))
                  print_semantic_error(" target qubit out of range !");
               // println(" => pauli x gate on: " << q);
               if (pg) 
                  pg->add(new qx::pauli_x(q));
               else
                  current_sub_circuit(qubits_count)->add(new qx::pauli_x(q));
            } 
            else if (words[0] == "cx")   // x gate
            {
               strings params = word_list(words[1],",");
               translate(params[0]);
               bool bit = is_bit(params[0]);
               uint32_t ctrl   = (bit ? bit_id(params[0]) : qubit_id(params[0]));
               uint32_t target = qubit_id(params[1]);

               if (ctrl > (qubits_count-1))
                  print_semantic_error(" ctrl qubit out of range !");
               if (target > (qubits_count-1))
                  print_semantic_error(" target qubit out of range !");
               if (bit)
               {
                  // println(" => binary controlled pauli_x gate (ctrl=" << ctrl << ", target=" << target << ")");
                  if (pg) 
                     pg->add(new qx::bin_ctrl(ctrl,new qx::pauli_x(target)));
                  else
                     current_sub_circuit(qubits_count)->add(new qx::bin_ctrl(ctrl,new qx::pauli_x(target)));
               } 
               else
               {
                  // println(" => controlled pauli_x gate (ctrl=" << ctrl << ", target=" << target << ")");
                  if (pg) 
                     pg->add(new qx::cnot(ctrl,target));
                  else
                     current_sub_circuit(qubits_count)->add(new qx::cnot(ctrl,target));
               }
            } 
            else if (words[0] == "c-x")   // c-x gate
            {
               strings params = word_list(words[1],",");
               for (uint32_t i=0; i<params.size(); ++i)
                  translate(params[i]);
               // target qubit processing
               uint32_t target = qubit_id(params[params.size()-1]);
               if (target > (qubits_count-1))
                  print_semantic_error(" target qubit out of range !");
               qx::gate * g = new qx::pauli_x(target);
               // control bit(s) processing
               for (uint32_t i=0; i<params.size()-1; ++i)
               {
                  if (!is_bit(params[i]))
                     print_semantic_error(" invalid control bit !");
                  uint32_t ctrl = bit_id(params[i]);
                  if (ctrl > (qubits_count-1))
                     print_semantic_error(" ctrl bit out of range !");
                  g = new qx::bin_ctrl(ctrl, g);
               } 
               if (pg) 
                  pg->add(g);
               else
                  current_sub_circuit(qubits_count)->add(g);
            }
            else if (words[0] == "y")   // y gate
            {
               uint32_t q = qubit_id(words[1]);
               if (q > (qubits_count-1))
                  print_semantic_error(" target qubit out of range !");
               // println(" => pauli y gate on: " << atoi(words[1].c_str()));
               qx::gate * g = new qx::pauli_y(q);
               if (pg) 
                  pg->add(g);
               else
                  current_sub_circuit(qubits_count)->add(g);
            } 
            else if (words[0] == "c-y")   // c-x gate
            {
               strings params = word_list(words[1],",");
               translate(params[0]);
               // target qubit processing
               uint32_t target = qubit_id(params[params.size()-1]);
               if (target > (qubits_count-1))
                  print_semantic_error(" target qubit out of range !");
               qx::gate * g = new qx::pauli_y(target);
               // control bit(s) processing
               for (uint32_t i=0; i<params.size()-1; ++i)
               {
                  if (!is_bit(params[i]))
                     print_semantic_error(" invalid control bit !");
                  uint32_t ctrl = bit_id(params[i]);
                  if (ctrl > (qubits_count-1))
                     print_semantic_error(" ctrl bit out of range !");
                  g = new qx::bin_ctrl(ctrl, g);
               } 
               if (pg) 
                  pg->add(g);
               else
                  current_sub_circuit(qubits_count)->add(g);
            }
            else if (words[0] == "z")   // z gate
            {
               uint32_t q = qubit_id(words[1]);
               if (q > (qubits_count-1))
                  print_semantic_error(" target qubit out of range !");
               // println(" => pauli z gate on: " << atoi(words[1].c_str()));
               if (pg) 
                  pg->add(new qx::pauli_z(q));
               else
                  current_sub_circuit(qubits_count)->add(new qx::pauli_z(q));
            }
            else if (words[0] == "qwait")   // z gate
            {
               size_t t = is_number(words[1]);
               if (pg) 
                  pg->add(new qx::qwait(t));
               else
                  current_sub_circuit(qubits_count)->add(new qx::qwait(t));
            }	
            else if (words[0] == "cz")   // z gate
            {
               strings params = word_list(words[1],",");
               translate(params[0]);
               bool bit = is_bit(params[0]);
               uint32_t ctrl   = (bit ? bit_id(params[0]) : qubit_id(params[0]));
               uint32_t target = qubit_id(params[1]);

               if (ctrl > (qubits_count-1))
                  print_semantic_error(" ctrl qubit out of range !");
               if (target > (qubits_count-1))
                  print_semantic_error(" target qubit out of range !");
               if (bit)
               {
                  // println(" => binary controlled pauli_z gate (ctrl=" << ctrl << ", target=" << target << ")");
                  if (pg) 
                     pg->add(new qx::bin_ctrl(ctrl,new qx::pauli_z(target)));
                  else
                     current_sub_circuit(qubits_count)->add(new qx::bin_ctrl(ctrl,new qx::pauli_z(target)));
               } 
               else
               {
                  if (pg) 
                     pg->add(new qx::cphase(ctrl,target));
                  else
                     current_sub_circuit(qubits_count)->add(new qx::cphase(ctrl,target));
                  //println(" => controlled pauli_z gate (ctrl=" << ctrl << ", target=" << target << ")");
                  //println("quantum controlled-z not implemented yet !");
               }
            } 
            else if (words[0] == "c-z")   // c-z gate
            {
               strings params = word_list(words[1],",");
               for (uint32_t i=0; i<params.size(); ++i)
                  translate(params[i]);
               // target qubit processing
               uint32_t target = qubit_id(params[params.size()-1]);
               if (target > (qubits_count-1))
                  print_semantic_error(" target qubit out of range !");
               qx::gate * g = new qx::pauli_z(target);
               // control bit(s) processing
               for (uint32_t i=0; i<params.size()-1; ++i)
               {
                  if (!is_bit(params[i]))
                     print_semantic_error(" invalid control bit !");
                  uint32_t ctrl = bit_id(params[i]);
                  if (ctrl > (qubits_count-1))
                     print_semantic_error(" ctrl bit out of range !");
                  g = new qx::bin_ctrl(ctrl, g);
               } 
               if (pg) 
                  pg->add(g);
               else
                  current_sub_circuit(qubits_count)->add(g);
            }
            /**
             * T gate
             */
            else if (words[0] == "t")   // T gate
            {
               uint32_t q = qubit_id(words[1]);
               if (q > (qubits_count-1))
                  print_semantic_error(" target qubit out of range !");
               // println(" => t gate on: " << q);
               if (pg) 
                  pg->add(new qx::t_gate(q));
               else
                  current_sub_circuit(qubits_count)->add(new qx::t_gate(q));
            }
            /**
             * Tdag gate
             */
            else if (words[0] == "tdag")   // Tdag gate
            {
               uint32_t q = qubit_id(words[1]);
               if (q > (qubits_count-1))
                  print_semantic_error(" target qubit out of range !");
               // println(" => t gate on: " << q);
               if (pg) 
                  pg->add(new qx::t_dag_gate(q));
               else
                  current_sub_circuit(qubits_count)->add(new qx::t_dag_gate(q));
            }

            /**
             * prepz
             */
            else if (words[0] == "prepz")   // prepz gate
            {
               uint32_t q = qubit_id(words[1]);
               if (q > (qubits_count-1))
                  print_semantic_error(" target qubit out of range !");
               // println(" => t gate on: " << q);
               if (pg) 
               {
                  // pg->add(new qx::measure(q));
                  // pg->add(new qx::bin_ctrl(q,new qx::pauli_x(q)));
                  pg->add(new qx::prepz(q));
               }
               else
               {
                  current_sub_circuit(qubits_count)->add(new qx::prepz(q));
               }
            }
            /**
             * rotations gates
             */
            else if (words[0] == "rx")   // rx gate
            {
               strings params = word_list(words[1],",");
               uint32_t q = qubit_id(params[0]);
               if (q > (qubits_count-1))
                  print_semantic_error(" target qubit out of range !");
               // println(" => rx gate on " << process_qubit(params[0]) << " (angle=" << params[1] << ")");
               if (pg) 
                  pg->add(new qx::rx(q,atof(params[1].c_str())));
               else
                  current_sub_circuit(qubits_count)->add(new qx::rx(q,atof(params[1].c_str())));
            }
            else if (words[0] == "ry")   // ry gate
            {
               strings params = word_list(words[1],",");
               uint32_t q = qubit_id(params[0]);
               if (q > (qubits_count-1))
                  print_semantic_error(" target qubit out of range !");
               // println(" => ry gate on " << process_qubit(params[0]) << " (angle=" << params[1] << ")");
               if (pg) 
                  pg->add(new qx::ry(q,atof(params[1].c_str())));
               else
                  current_sub_circuit(qubits_count)->add(new qx::ry(q,atof(params[1].c_str())));
            }
            else if (words[0] == "rz")   // rz gate
            {
               strings params = word_list(words[1],",");
               uint32_t q = qubit_id(params[0]);
               if (q > (qubits_count-1))
                  print_semantic_error(" target qubit out of range !");
               // println(" => rz gate on " << process_qubit(params[0]) << " (angle=" << params[1] << ")");
               if (pg) 
                  pg->add(new qx::rz(q,atof(params[1].c_str())));
               else
                  current_sub_circuit(qubits_count)->add(new qx::rz(q,atof(params[1].c_str())));
            }

            /**
             * phase 
             */
            else if (words[0] == "ph")   // phase shift gate
            {
               //strings params = word_list(words[1],",");
               uint32_t q = qubit_id(words[1]);
               if (q > (qubits_count-1))
                  print_semantic_error(" target qubit out of range !");
               // println(" => phase gate on " << process_qubit(words[1]));
               if (pg) 
                  pg->add(new qx::phase_shift(q));
               else
                  current_sub_circuit(qubits_count)->add(new qx::phase_shift(q));
            }
            else if (words[0] == "s")   // phase shift gate
            {
               //strings params = word_list(words[1],",");
               uint32_t q = qubit_id(words[1]);
               if (q > (qubits_count-1))
                  print_semantic_error(" target qubit out of range !");
               // println(" => phase gate on " << process_qubit(words[1]));
               if (pg) 
                  pg->add(new qx::phase_shift(q));
               else
                  current_sub_circuit(qubits_count)->add(new qx::phase_shift(q));
            }
            else if (words[0] == "not")   // classical not gate
            {
               translate(words[1]);
               if (!is_bit(words[1]))
                  print_semantic_error(" invalid target bit !");
               uint32_t b = bit_id(words[1]);
               if (b > (qubits_count-1))
                  print_semantic_error(" target bit out of range !");
               qx::gate * g = new qx::classical_not(b);
               if (pg) 
                  pg->add(g);
               else
                  current_sub_circuit(qubits_count)->add(g);
            }

            /**
             * measurement 
             */
            else if (words[0] == "measure")   // measurement
            {
               uint32_t q = qubit_id(words[1]);
               if (q > (qubits_count-1))
                  print_semantic_error(" target qubit out of range !");
               // println(" => measure qubit " << atoi(words[1].c_str()));
               // println(" => measure qubit " << q);
               if (pg) 
                  pg->add(new qx::measure(q));
               else
                  current_sub_circuit(qubits_count)->add(new qx::measure(q));
            } 
            /**
             * toffoli 
             */
            else if (words[0] == "toffoli")   // rx gate
            {
               strings params = word_list(words[1],",");
               if (params.size() != 3)
                  print_semantic_error(" toffoli gate requires 3 qubits !");
               uint32_t q0 = qubit_id(params[0]);
               uint32_t q1 = qubit_id(params[1]);
               uint32_t q2 = qubit_id(params[2]);
               if (q0 > (qubits_count-1)) print_semantic_error(" first control qubit out of range !");
               if (q1 > (qubits_count-1)) print_semantic_error(" scond control qubit out of range !");
               if (q2 > (qubits_count-1)) print_semantic_error(" target qubit out of range !");
               // println(" => toffoli gate on " << process_qubit(params[2]) << " (ctrl_q1=" << params[0] << ", ctrl_q2=" << params[1] << ")");
               if (pg) 
                  pg->add(new qx::toffoli(q0,q1,q2));
               else
                  current_sub_circuit(qubits_count)->add(new qx::toffoli(q0,q1,q2));
            }
            else if (words[0] == "print")   // print
            {
               std::string param = original_line;
               format_line(param);
               size_t is = param.find_first_of('"');
               if (is == std::string::npos)
                  print_semantic_error(" malformed string argument : string should start and end with '\"' !");
               size_t ie = param.find_last_of('"');
               if ((ie-is) == 0)
                  print_semantic_error(" malformed string argument : string should start and end with '\"' !");
               param = param.substr(is+1,ie-is-1);
               // format_line(param);
               // println("param : " << param);
               if (pg) 
                  pg->add(new qx::print_str(param));
               else
                  current_sub_circuit(qubits_count)->add(new qx::print_str(param));
            }
            else
               print_syntax_error(" unknown gate or command !");

            return 0;
         }

         /*
            uint32_t qubit_id(std::string& str)
            {
            std::string id = str.substr(1);
            println("id=" << id);
            return atoi(id.c_str());
            }
            */

      public:

         /**
          * \brief
          * \return error model type
          */
         qx::error_model_t get_error_model()
         {
            return error_model;
         }

         /**
          * \return error_probability
          */
         double get_error_probability()
         {
            return error_probability;
         }

         /**
          * \brief 
          * \return loaded circuits
          */
         circuits_t get_circuits()
         {
            return circuits;
         }



      private:

         /**
          * \brief check if <str> is a natural bit
          */
         bool is_bit(std::string& str)
         {
            return (str[0] == 'b');
         }

         std::string process_qubit(std::string& str)
         {
            // check validity
            if ((str[0] != 'q') && (str[0] != 'b'))
            {
               println("[x] syntax error: invalid qubit/bit identifier !");
               return "";
            }
            for (int i=1; i<(str.size()-1); i++)
            {
               if (!is_digit(str[i]))
               {
                  println("[x] syntax error: invalid qubit/bit identifier !");
                  return "";
               }
            }
            // valid qubit/bit identifier, process it ...
            std::string r="";
            r+=(is_bit(str) ? "bit " : "qubit "); 
            r+= int_to_str(qubit_id(str));
            return r;
         }

         /**
          * @brief dummy function : process gate            
          */
         void process_gate(strings &words)
         {
            //std::cout << "gate name: " << words[1] << std::endl;
         }

   };
}

#undef error
#undef println


#endif // QX_QUANTUM_CODE_LOADER

