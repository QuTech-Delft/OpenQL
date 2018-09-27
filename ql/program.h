/**
 * @file   program.h
 * @date   04/2017
 * @author Nader Khammassi
 *         Imran Ashraf
 * @brief  openql program
 */

#ifndef QL_PROGRAM_H
#define QL_PROGRAM_H

#include <ql/utils.h>
#include <ql/options.h>
#include <ql/platform.h>
#include <ql/kernel.h>
#include <ql/interactionMatrix.h>
#include <ql/eqasm_compiler.h>
#include <ql/arch/cbox/cbox_eqasm_compiler.h>
#include <ql/arch/cc_light/cc_light_eqasm_compiler.h>
#include <ql/arch/quantumsim_eqasm_compiler.h>
#include <ql/arch/cc/eqasm_backend_cc.h>

static unsigned long phi_node_count = 0;

namespace ql
{

extern bool initialized;

/**
 * quantum_program_
 */
class quantum_program
{
   protected:
      bool                        default_config;
      std::string                 config_file_name;
      std::vector<float>          sweep_points;
      std::vector<quantum_kernel> kernels;

   public:
      std::string           name;
      ql::quantum_platform  platform;
      size_t                qubit_count;
      size_t                creg_count;
      std::string           eqasm_compiler_name;
      ql::eqasm_compiler *  backend_compiler;


   public:
      quantum_program(std::string n, quantum_platform platf, size_t nqubits, size_t ncregs)
            : name(n), platform(platf), qubit_count(nqubits), creg_count(ncregs)
      {
         default_config = true;
         eqasm_compiler_name = platform.eqasm_compiler_name;
	      backend_compiler    = NULL;
         if (eqasm_compiler_name =="")
         {
            EOUT("eqasm compiler name must be specified in the hardware configuration file !");
            throw std::exception();
         }
         else if (eqasm_compiler_name == "none")
         {

         }
         else if (eqasm_compiler_name == "qx")
         {
            // at the moment no qx specific thing is done
         }
         else if (eqasm_compiler_name == "qumis_compiler")
         {
            backend_compiler = new ql::arch::cbox_eqasm_compiler();
         }
	      else if (eqasm_compiler_name == "cc_light_compiler" )
	      {
            backend_compiler = new ql::arch::cc_light_eqasm_compiler();
	      }
         else if (eqasm_compiler_name == "quantumsim_compiler" )
         {
            backend_compiler = new ql::arch::quantumsim_eqasm_compiler();
         }
         else if (eqasm_compiler_name == "eqasm_backend_cc" )
         {
            backend_compiler = new ql::arch::eqasm_backend_cc();
         }
         else
         {
            EOUT("the '" << eqasm_compiler_name << "' eqasm compiler backend is not suported !");
            throw std::exception();
         }

         if(qubit_count > platform.qubit_number)
         {
            EOUT("number of qubits requested in program '" + std::to_string(qubit_count) + "' is greater than the qubits available in platform '" + std::to_string(platform.qubit_number) + "'" );
            throw ql::exception("[x] error : number of qubits requested in program '"+std::to_string(qubit_count)+"' is greater than the qubits available in platform '"+std::to_string(platform.qubit_number)+"' !",false);
         }
      }

      void add(ql::quantum_kernel &k)
      {
         // check sanity of supplied qubit/classical operands for each gate
         ql::circuit& kc = k.get_circuit();
         for( auto & g : kc )
         {
            auto & gate_operands = g->operands;
            auto & gname = g->name;
            auto gtype = g->type();
            for(auto & op : gate_operands)
            {
               if(
                  ((gtype == __classical_gate__) && (op >= creg_count)) ||
                  ((gtype != __classical_gate__) && (op >= qubit_count))
                 )
               {
                   EOUT("Out of range operand(s) for operation: '" << gname << "'");
                   EOUT("FIXME: creg_count=" << creg_count << ", qubit_count" << qubit_count);
                   throw ql::exception("Out of range operand(s) for operation: '"+gname+"' !",false);
               }
            }
         }

         // if sane, now add kernel to list of kernels
         kernels.push_back(k);
      }

      void add_program(ql::quantum_program p)
      {
         for(auto & k : p.kernels)
         {
            add(k);
         }
      }

      void add_if(ql::quantum_kernel &k, ql::operation & cond)
      {
         // phi node
         ql::quantum_kernel kphi1(k.name+"_if", platform, qubit_count, creg_count);
         kphi1.set_kernel_type(ql::kernel_type_t::IF_START);
         kphi1.set_condition(cond);
         kernels.push_back(kphi1);

         add(k);

         // phi node
         ql::quantum_kernel kphi2(k.name+"_if_end", platform, qubit_count, creg_count);
         kphi2.set_kernel_type(ql::kernel_type_t::IF_END);
         kphi2.set_condition(cond);
         kernels.push_back(kphi2);
      }

      void add_if(ql::quantum_program p, ql::operation & cond)
      {
         // phi node
         ql::quantum_kernel kphi1(p.name+"_if", platform, qubit_count, creg_count);
         kphi1.set_kernel_type(ql::kernel_type_t::IF_START);
         kphi1.set_condition(cond);
         kernels.push_back(kphi1);

         add_program(p);

         // phi node
         ql::quantum_kernel kphi2(p.name+"_if_end", platform, qubit_count, creg_count);
         kphi2.set_kernel_type(ql::kernel_type_t::IF_END);
         kphi2.set_condition(cond);
         kernels.push_back(kphi2);
      }

      void add_if_else(ql::quantum_kernel &k_if, ql::quantum_kernel &k_else, ql::operation & cond)
      {
         ql::quantum_kernel kphi1(k_if.name+"_if"+ std::to_string(phi_node_count), platform, qubit_count, creg_count);
         kphi1.set_kernel_type(ql::kernel_type_t::IF_START);
         kphi1.set_condition(cond);
         kernels.push_back(kphi1);

         add(k_if);

         // phi node
         ql::quantum_kernel kphi2(k_if.name+"_if"+ std::to_string(phi_node_count) +"_end", platform, qubit_count, creg_count);
         kphi2.set_kernel_type(ql::kernel_type_t::IF_END);
         kphi2.set_condition(cond);
         kernels.push_back(kphi2);


         // phi node
         ql::quantum_kernel kphi3(k_else.name+"_else" + std::to_string(phi_node_count), platform, qubit_count, creg_count);
         kphi3.set_kernel_type(ql::kernel_type_t::ELSE_START);
         kphi3.set_condition(cond);
         kernels.push_back(kphi3);

         add(k_else);

         // phi node
         ql::quantum_kernel kphi4(k_else.name+"_else" + std::to_string(phi_node_count)+"_end", platform, qubit_count, creg_count);
         kphi4.set_kernel_type(ql::kernel_type_t::ELSE_END);
         kphi4.set_condition(cond);
         kernels.push_back(kphi4);

         phi_node_count++;
      }

      void add_if_else(ql::quantum_program &p_if, ql::quantum_program &p_else, ql::operation & cond)
      {
         ql::quantum_kernel kphi1(p_if.name+"_if"+ std::to_string(phi_node_count), platform, qubit_count, creg_count);
         kphi1.set_kernel_type(ql::kernel_type_t::IF_START);
         kphi1.set_condition(cond);
         kernels.push_back(kphi1);

         add_program(p_if);

         // phi node
         ql::quantum_kernel kphi2(p_if.name+"_if"+ std::to_string(phi_node_count) +"_end", platform, qubit_count, creg_count);
         kphi2.set_kernel_type(ql::kernel_type_t::IF_END);
         kphi2.set_condition(cond);
         kernels.push_back(kphi2);


         // phi node
         ql::quantum_kernel kphi3(p_else.name+"_else" + std::to_string(phi_node_count), platform, qubit_count, creg_count);
         kphi3.set_kernel_type(ql::kernel_type_t::ELSE_START);
         kphi3.set_condition(cond);
         kernels.push_back(kphi3);

         add_program(p_else);

         // phi node
         ql::quantum_kernel kphi4(p_else.name+"_else" + std::to_string(phi_node_count)+"_end", platform, qubit_count, creg_count);
         kphi4.set_kernel_type(ql::kernel_type_t::ELSE_END);
         kphi4.set_condition(cond);
         kernels.push_back(kphi4);

         phi_node_count++;
      }

      void add_do_while(ql::quantum_kernel &k, ql::operation & cond)
      {
         // phi node
         ql::quantum_kernel kphi1(k.name+"_do_while"+ std::to_string(phi_node_count) +"_start", platform, qubit_count, creg_count);
         kphi1.set_kernel_type(ql::kernel_type_t::DO_WHILE_START);
         kphi1.set_condition(cond);
         kernels.push_back(kphi1);

         add(k);

         // phi node
         ql::quantum_kernel kphi2(k.name+"_do_while" + std::to_string(phi_node_count), platform, qubit_count, creg_count);
         kphi2.set_kernel_type(ql::kernel_type_t::DO_WHILE_END);
         kphi2.set_condition(cond);
         kernels.push_back(kphi2);
         phi_node_count++;
      }

      void add_do_while(ql::quantum_program p, ql::operation & cond)
      {
         // phi node
         ql::quantum_kernel kphi1(p.name+"_do_while"+ std::to_string(phi_node_count) +"_start", platform, qubit_count, creg_count);
         kphi1.set_kernel_type(ql::kernel_type_t::DO_WHILE_START);
         kphi1.set_condition(cond);
         kernels.push_back(kphi1);

         add_program(p);

         // phi node
         ql::quantum_kernel kphi2(p.name+"_do_while" + std::to_string(phi_node_count), platform, qubit_count, creg_count);
         kphi2.set_kernel_type(ql::kernel_type_t::DO_WHILE_END);
         kphi2.set_condition(cond);
         kernels.push_back(kphi2);
         phi_node_count++;
      }

      void add_for(ql::quantum_kernel &k, size_t iterations)
      {
         // phi node
         ql::quantum_kernel kphi1(k.name+"_for"+ std::to_string(phi_node_count) +"_start", platform, qubit_count, creg_count);
         kphi1.set_kernel_type(ql::kernel_type_t::FOR_START);
         kphi1.iterations = iterations;
         kernels.push_back(kphi1);

         k.iterations = iterations;
         add(k);

         // phi node
         ql::quantum_kernel kphi2(k.name+"_for" + std::to_string(phi_node_count) +"_end", platform, qubit_count, creg_count);
         kphi2.set_kernel_type(ql::kernel_type_t::FOR_END);
         kernels.push_back(kphi2);
         phi_node_count++;
      }

      void add_for(ql::quantum_program p, size_t iterations)
      {
         bool nested_for = false;
         for(auto & k : p.kernels)
         {
            if(k.type == ql::kernel_type_t::FOR_START)
               nested_for = true;
         }
         if(nested_for)
         {
            EOUT("Nested for not yet implemented !");
            throw ql::exception("Error: Nested for not yet implemented !",false);
         }

         if(iterations>0) // as otherwise it will be optimized away
         {
            // phi node
            ql::quantum_kernel kphi1(p.name+"_for"+ std::to_string(phi_node_count) +"_start", platform, qubit_count, creg_count);
            kphi1.set_kernel_type(ql::kernel_type_t::FOR_START);
            kphi1.iterations = iterations;
            kernels.push_back(kphi1);

            // phi node
            ql::quantum_kernel kphi2(p.name, platform, qubit_count, creg_count);
            kphi2.set_kernel_type(ql::kernel_type_t::STATIC);
            kernels.push_back(kphi2);

            add_program(p);

            // phi node
            ql::quantum_kernel kphi3(p.name+"_for" + std::to_string(phi_node_count) +"_end", platform, qubit_count, creg_count);
            kphi3.set_kernel_type(ql::kernel_type_t::FOR_END);
            kernels.push_back(kphi3);
            phi_node_count++;
         }
      }

      void set_config_file(std::string file_name)
      {
         config_file_name = file_name;
         default_config   = false;
      }

      std::string qasm()
      {
         std::stringstream ss;
         ss << "# this file has been automatically generated by the OpenQL compiler please do not modify it manually.\n";
         ss << "qubits " << qubit_count << "\n";
         for (size_t k=0; k<kernels.size(); ++k)
            ss <<'\n' << kernels[k].qasm();
	/*
         ss << ".cal0_1\n";
         ss << "   prepz q0\n";
         ss << "   measure q0\n";
         ss << ".cal0_2\n";
         ss << "   prepz q0\n";
         ss << "   measure q0\n";
         ss << ".cal1_1\n";
         ss << "   prepz q0\n";
         ss << "   x q0\n";
         ss << "   measure q0\n";
         ss << ".cal1_2\n";
         ss << "   prepz q0\n";
         ss << "   x q0\n";
         ss << "   measure q0\n";
	 */
         return ss.str();
      }

      std::string microcode()
      {
         std::stringstream ss;
         ss << "# this file has been automatically generated by the OpenQL compiler please do not modify it manually.\n";
         ss << uc_header();
         for (size_t k=0; k<kernels.size(); ++k)
            ss <<'\n' << kernels[k].micro_code();
	 /*
         ss << "     # calibration points :\n";  // wait for 100us
         // calibration points for |0>
         for (size_t i=0; i<2; ++i)
         {
            ss << "     waitreg r0               # prepz q0 (+100us) \n";  // wait for 100us
            ss << "     waitreg r0               # prepz q0 (+100us) \n";  // wait for 100us
            // measure
            ss << "     wait 60 \n";  // wait
            ss << "     pulse 0000 1111 1111 \n";  // pulse
            ss << "     wait 50\n";
            ss << "     measure\n";  // measurement discrimination
         }

         // calibration points for |1>
         for (size_t i=0; i<2; ++i)
         {
            // prepz
            ss << "     waitreg r0               # prepz q0 (+100us) \n";  // wait for 100us
            ss << "     waitreg r0               # prepz q0 (+100us) \n";  // wait for 100us
            // X
            ss << "     pulse 1001 0000 1001     # X180 \n";
            ss << "     wait 10\n";
            // measure
            ss << "     wait 60\n";  // wait
            ss << "     pulse 0000 1111 1111\n";  // pulse
            ss << "     wait 50\n";
            ss << "     measure\n";  // measurement discrimination
         }
	 */

         ss << "     beq  r3,  r3, loop   # infinite loop";
         return ss.str();
      }

      void set_platform(quantum_platform & platform)
      {
         this->platform = platform;
      }

      std::string uc_header()
      {
         std::stringstream ss;
         ss << "# auto-generated micro code from rb.qasm by OpenQL driver, please don't modify it manually \n";
         ss << "mov r11, 0       # counter\n";
         ss << "mov r3,  10      # max iterations\n";
         ss << "mov r0,  20000   # relaxation time / 2\n";
         // ss << name << ":\n";
         ss << "loop:\n";
         return ss.str();
      }

      int compile()
      {
         IOUT("compiling ...");

         if (kernels.empty())
            return -1;

         if( ql::options::get("optimize") == "yes" )
         {
            IOUT("optimizing quantum kernels...");
            for (size_t k=0; k<kernels.size(); ++k)
               kernels[k].optimize();
         }

         auto tdopt = ql::options::get("decompose_toffoli");
         if( tdopt == "AM" || tdopt == "NC" )
         {
            IOUT("Decomposing Toffoli ...");
            for (size_t k=0; k<kernels.size(); ++k)
               kernels[k].decompose_toffoli();
         }
         else if( tdopt == "no" )
         {
            IOUT("Not Decomposing Toffoli ...");
         }
         else
         {
            EOUT("Unknown option '" << tdopt << "' set for decompose_toffoli");
            throw ql::exception("Error: Unknown option '"+tdopt+"' set for decompose_toffoli !",false);
         }

         std::stringstream ss_qasm;
         ss_qasm << ql::options::get("output_dir") << "/" << name << ".qasm";
         std::string s = qasm();

         IOUT("writing un-scheduled qasm to '" << ss_qasm.str() << "' ...");
         ql::utils::write_file(ss_qasm.str(), s);

         schedule();

         if (backend_compiler == NULL)
         {
            WOUT("no eqasm compiler has been specified in the configuration file, only qasm code has been compiled.");
            return 0;
         }
         else
         {
            if (eqasm_compiler_name == "cc_light_compiler" || eqasm_compiler_name == "eqasm_backend_cc")
            {
               backend_compiler->compile(name, kernels, platform);
            }
            else
            {
               // FIXME(WJV): I would suggest to move the fusing to a backend that wants it, and then:
               // - always call:  backend_compiler->compile(name, kernels, platform);
               // - remove from eqasm_compiler.h: compile(std::string prog_name, ql::circuit& c, ql::quantum_platform& p);

               IOUT("fusing quantum kernels...");
               ql::circuit fused;
               for (size_t k=0; k<kernels.size(); ++k)
               {
                  ql::circuit& kc = kernels[k].get_circuit();
                  for(size_t i=0; i<kernels[k].iterations; i++)
                  {
                     fused.insert(fused.end(), kc.begin(), kc.end());
                  }
               }

               try
               {
                  IOUT("compiling eqasm code...");
                  backend_compiler->compile(name, fused, platform);
               }
               catch (ql::exception e)
               {
                  EOUT("[x] error : eqasm_compiler.compile() : compilation interrupted due to fatal error.");
                  throw e;
               }

               IOUT("writing eqasm code to '" << ( ql::options::get("output_dir") + "/" + name+".asm"));
               backend_compiler->write_eqasm( ql::options::get("output_dir") + "/" + name + ".asm");

               IOUT("writing traces to '" << ( ql::options::get("output_dir") + "/trace.dat"));
               backend_compiler->write_traces( ql::options::get("output_dir") + "/trace.dat");
            }
         }

         if (sweep_points.size())
         {
            std::stringstream ss_swpts;
            ss_swpts << "{ \"measurement_points\" : [";
            for (size_t i=0; i<sweep_points.size()-1; i++)
               ss_swpts << sweep_points[i] << ", ";
            ss_swpts << sweep_points[sweep_points.size()-1] << "] }";
            std::string config = ss_swpts.str();
            if (default_config)
            {
               std::stringstream ss_config;
               ss_config << ql::options::get("output_dir") << "/" << name << "_config.json";
               std::string conf_file_name = ss_config.str();
               IOUT("writing sweep points to '" << conf_file_name << "'...");
               ql::utils::write_file(conf_file_name, config);
            }
            else
            {
               std::stringstream ss_config;
               ss_config << ql::options::get("output_dir") << "/" << config_file_name;
               std::string conf_file_name = ss_config.str();
               IOUT("writing sweep points to '" << conf_file_name << "'...");
               ql::utils::write_file(conf_file_name, config);
            }
         }
         else
         {
            EOUT("cannot write sweepoint file : sweep point array is empty !");
         }

	      IOUT("compilation of program '" << name << "' done.");

         return 0;
      }

      void schedule()
      {
         std::string sched_qasm;
         sched_qasm += "qubits " + std::to_string(qubit_count) + "\n";

         IOUT("scheduling the quantum program");

         sched_qasm = "qubits " + std::to_string(qubit_count) + "\n";
         for (auto k : kernels)
         {
            std::string kernel_sched_qasm;
            std::string kernel_sched_dot;
            k.schedule(platform, kernel_sched_qasm, kernel_sched_dot);
            sched_qasm += kernel_sched_qasm + '\n';
            // disabled generation of dot file for each kernel
            // string fname = ql::options::get("output_dir") + "/" + k.get_name() + scheduler + ".dot";
            // IOUT("writing scheduled qasm to '" << fname << "' ...");
            // ql::utils::write_file(fname, kernel_sched_dot);
         }

         string fname = ql::options::get("output_dir") + "/" + name + "_scheduled.qasm";
         IOUT("writing scheduled qasm to '" << fname << "' ...");
         ql::utils::write_file(fname, sched_qasm);
      }

      void print_interaction_matrix()
      {
         IOUT("printing interaction matrix...");

         for (auto k : kernels)
         {
            InteractionMatrix imat( k.get_circuit(), qubit_count);
            string mstr = imat.getString();
            std::cout << mstr << std::endl;
         }
      }

      void write_interaction_matrix()
      {
         for (auto k : kernels)
         {
            InteractionMatrix imat( k.get_circuit(), qubit_count);
            string mstr = imat.getString();

            string fname = ql::options::get("output_dir") + "/" + k.get_name() + "InteractionMatrix.dat";
            IOUT("writing interaction matrix to '" << fname << "' ...");
            ql::utils::write_file(fname, mstr);
         }
      }

      void set_sweep_points(float * swpts, size_t size)
      {
         sweep_points.clear();
         for (size_t i=0; i<size; ++i)
            sweep_points.push_back(swpts[i]);
      }
};

} // ql

#endif //QL_PROGRAM_H
