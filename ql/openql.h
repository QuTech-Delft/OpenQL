/**
 * @file   openql.h
 * @date   11/2016
 * @author Nader Khammassi
 * @brief  main openql header
 */
#ifndef OPENQL_H
#define OPENQL_H

#include "optimizer.h"
#include "circuit.h"
#include "platform.h"
#include "transmon.h"
#include "dependenceGraph.h"


#include <fstream>
#include <map>

namespace ql
{
   /**
    * openql types
    */

    typedef std::vector<float> sweep_points_t;
    typedef std::stringstream  str_t;

    typedef std::string qasm_inst_t;
    typedef std::string ucode_inst_t;

    typedef std::map<qasm_inst_t, ucode_inst_t> instruction_map_t;

    namespace utils
    {
       bool format_string(std::string& s);
       void replace_all(std::string &str, std::string seq, std::string rep);
    }

    bool load_instruction_map(std::string file_name, instruction_map_t& imap);


    /**
     * configurable instruction map
     */
    /* static */ instruction_map_t instruction_map;
    static bool              initialized = false;
    static ql_platform_t     target_platform;

    void init(ql_platform_t platform, std::string instruction_map_file="")
    {
       target_platform = platform;

       if (instruction_map_file != "")
       {
	  if (!load_instruction_map(instruction_map_file,instruction_map))
	     println("[x] error : failed to load the instruction map !");
       }
       else
       {
	  /**
	   * predefined setups : transmon, starmon..
	   */
	  if (target_platform == transmon_platform)
	  {
	     instruction_map["rx90" ]   = ucode_inst_t("     pulse 1011 0000 1011\n     wait 10");
	     instruction_map["mrx90"]   = ucode_inst_t("     pulse 1101 0000 1101\n     wait 10");
	     instruction_map["rx180"]   = ucode_inst_t("     pulse 1001 0000 1001\n     wait 10");
	     instruction_map["ry90" ]   = ucode_inst_t("     pulse 1100 0000 1100\n     wait 10");
	     instruction_map["mry90"]   = ucode_inst_t("     pulse 1110 0000 1110\n     wait 10");
	     instruction_map["ry180"]   = ucode_inst_t("     pulse 1010 0000 1010\n     wait 10");
	     instruction_map["prepz"]   = ucode_inst_t("     waitreg r0\n     waitreg r0\n");
	     instruction_map["measure"] = ucode_inst_t("     wait 60\n     pulse 0000 1111 1111\n     wait 50\n     measure\n");
	  } else if (target_platform == starmon_platform)
	  {
	  }

       }
       initialized = true;
    }





    /**
     * load instruction map from a file
     */
    bool load_instruction_map(std::string file_name, instruction_map_t& imap)
    {
       std::ifstream file(file_name);

       std::string line;
       int i=0;

       while (std::getline(file, line))
       {
	  #ifdef __debug__
	  println("[+] line " << i << " : " << line);
	  #endif
	  size_t p = line.find(":");
	  if (line.size() < 3) continue;
	  if (p == std::string::npos)
	  {
	     println("[+] syntax error at line " << i << " : invalid syntax.");
	     return false;
	  }
	  std::string key = line.substr(0,p);
	  std::string val = line.substr(p+1,line.size()-p-1);

	  if (!utils::format_string(key))
	  {
	     println("[+] syntax error at line " << i << " : invalid key format.");
	     return false;
	  }

	  if (!utils::format_string(val))
	  {
	     println("[+] syntax error at line " << i << " : invalid value format.");
	     return false;
	  }

	  #ifdef __debug__
	  println(" --> key : " << key);
	  println(" --> val : " << val);
	  #endif
	  imap[key] = val;
       }
       file.close();
       #ifdef __debug__
       for (instruction_map_t::iterator i=imap.begin(); i!=imap.end(); i++)
	  println("[ " << (*i).first <<  " --> " << (*i).second << " ]");
       #endif // __debug__

       return true;
    }


   /**
    * quantum_kernel
    */
   class quantum_kernel
   {
      public:

        quantum_kernel(std::string name) : name(name), iterations(1)
	{
	}

	void loop(size_t it)
	{
	   iterations = it;
	}

	void hadamard(size_t qubit)
	{
	   // qubit not used (default : q0)
	   c.push_back(new ql::ry90(qubit));
	   c.push_back(new ql::rx180(qubit));
	}

	void x(size_t qubit)
	{
	   // qubit not used (default : q0)
	   c.push_back(new ql::rx180(qubit));
	}

	void y(size_t qubit)
	{
	   // qubit not used (default : q0)
	   c.push_back(new ql::ry180(qubit));
	}

	void z(size_t qubit)
	{
	   // qubit not used (default : q0)
	   c.push_back(new ql::ry180(qubit));
	   c.push_back(new ql::rx180(qubit));
	}


	void rx90(size_t qubit)
	{
	   // qubit not used (default : q0)
	   c.push_back(new ql::rx90(qubit));
	}

	void mrx90(size_t qubit)
	{
	   // qubit not used (default : q0)
	   c.push_back(new ql::mrx90(qubit));
	}

	void rx180(size_t qubit)
	{
	   // qubit not used (default : q0)
	   c.push_back(new ql::rx180(qubit));
	}

	void ry90(size_t qubit)
	{
	   // qubit not used (default : q0)
	   c.push_back(new ql::ry90(qubit));
	}

	void mry90(size_t qubit)
	{
	   // qubit not used (default : q0)
	   c.push_back(new ql::mry90(qubit));
	}

	void ry180(size_t qubit)
	{
	   // qubit not used (default : q0)
	   c.push_back(new ql::ry180(qubit));
	}

	void measure(size_t qubit)
	{
	   // qubit not used (default : q0)
	   c.push_back(new ql::measure(qubit));
	}

	void prepz(size_t qubit)
	{
	   // qubit not used (default : q0)
	   c.push_back(new ql::prepz(qubit));
	}

    void cnot(size_t qubit1, size_t qubit2)
	{
        // qubit1 and qubit2 not used (default : q0, q1)
	    c.push_back(new ql::cnot(qubit1, qubit2));
	}

	/**
	 * add clifford
	 */
	void clifford(int id, size_t qubit=0)
	{
	   switch (id)
	   {
	      case 0 : break;                                  //  ['I']
	      case 1 : ry90(0); rx90(0); break;                //  ['Y90', 'X90']
	      case 2 : mrx90(0); mry90(0); break;              //  ['mX90', 'mY90']
	      case 3 : rx180(0); break;                        //  ['X180']
	      case 4 : mry90(0); mrx90(0); break;              //  ['mY90', 'mX90']
	      case 5 : rx90(0); mry90(0); break;               //  ['X90', 'mY90']
	      case 6 : ry180(0); break;                        //  ['Y180']
	      case 7 : mry90(0); rx90(0); break;               //  ['mY90', 'X90']
	      case 8 : rx90(0); ry90(0); break;                //  ['X90', 'Y90']
	      case 9 : rx180(0); ry180(0); break;              //  ['X180', 'Y180']
	      case 10: ry90(0); mrx90(0); break;               //  ['Y90', 'mX90']
	      case 11: mrx90(0); ry90(0); break;               //  ['mX90', 'Y90']
	      case 12: ry90(0); rx180(0); break;               //  ['Y90', 'X180']
	      case 13: mrx90(0); break;                        //  ['mX90']
	      case 14: rx90(0); mry90(0); mrx90(0); break;     //  ['X90', 'mY90', 'mX90']
	      case 15: mry90(0); break;                        //  ['mY90']
	      case 16: rx90(0); break;                         //  ['X90']
	      case 17: rx90(0); ry90(0); rx90(0); break;       //  ['X90', 'Y90', 'X90']
	      case 18: mry90(0); rx180(0); break;              //  ['mY90', 'X180']
	      case 19: rx90(0); ry180(0); break;               //  ['X90', 'Y180']
	      case 20: rx90(0); mry90(0); rx90(0); break;      //  ['X90', 'mY90', 'X90']
	      case 21: ry90(0); break;                         //  ['Y90']
	      case 22: mrx90(0); ry180(0); break;              //  ['mX90', 'Y180']
	      case 23: rx90(0); ry90(0); mrx90(0); break;      //  ['X90', 'Y90', 'mX90']
	      default: break;
	   }
	}



	/**
	 * qasm
	 */
	std::string qasm()
	{
	   std::stringstream ss;
	   ss << "." << name;
	   if (iterations > 1)
	      ss << "(" << iterations << ") \n";
	   else
	      ss << "\n";
	   for (size_t i=0; i<c.size(); ++i)
	   {
	      ss << c[i]->qasm() << "\n";
	      // std::cout << c[i]->qasm() << std::endl;
	   }
	   return ss.str();
	}

	/**
	 * micro code
	 */
	std::string micro_code()
	{
	   std::stringstream ss;
	   // ss << "." << name;
	   // if (iterations > 1)
	      // ss << "(" << iterations << ")\n";
	   // else
	      // ss << "\n";
	   for (size_t i=0; i<c.size(); ++i)
	   {
	      ss << c[i]->micro_code() << "\n";
	      // std::cout << c[i]->qasm() << std::endl;
	   }
	   return ss.str();
	}


	void optimize()
	{
	   ql::rotations_merging rm;
	   if (contains_measurements(c))
	   {
	      // decompose the circuit
	      std::vector<circuit*> cs = split_circuit(c);
	      std::vector<circuit > cs_opt;
	      for (int i=0; i<cs.size(); ++i)
	      {
		 if (!contains_measurements(*cs[i]))
		 {
		    circuit opt = rm.optimize(*cs[i]);
		    cs_opt.push_back(opt);
		 }
		 else
		    cs_opt.push_back(*cs[i]);
	      }
	      // for (int i=0; i<cs_opt.size(); ++i)
		 // print(cs_opt[i]);
	      c.clear( );
	      for (int i=0; i<cs_opt.size(); ++i)
		 for (int j=0; j<cs_opt[i].size(); j++)
		    c.push_back(cs_opt[i][j]);
	   }
	   else
	   {
	      c = rm.optimize(c);
	   }

	}

	void schedule(size_t nqubits)
	{
		std::cout << "Scheduling the quantum kernel" << std::endl;
		DependGraph dg;
		dg.Init(c,nqubits);
		// dg.Print();
        // dg.PrintMatrix();
        // dg.PrintDot();

        // dg.PrintScheduleASAP();
        // dg.PrintDotScheduleASAP();
        dg.PrintQASMScheduledASAP();

        // dg.PrintScheduleALAP();
        dg.PrintQASMScheduledALAP();
	}

	std::vector<circuit*> split_circuit(circuit x, bool verbose=false)
	{
	   if (verbose) println("[+] circuit decomposition in basic blocks ... ");
	   std::vector<circuit*> cs;
	   cs.push_back(new circuit());
	   for (size_t i=0; i<x.size(); i++)
	   {
	      if ((x[i]->type() == __prepz_gate__) || (x[i]->type() == __measure_gate__))
	      {
		 cs.push_back(new circuit());
		 cs.back()->push_back(x[i]);
		 cs.push_back(new circuit());
	      }
	      else
	      {
		 cs.back()->push_back(x[i]);
	      }
	   }
	   if (verbose) println("[+] circuit decomposion done (" << cs.size() << ").");
	   /*
	   for (int i=0; i<cs.size(); ++i)
	   {
	      println(" |-- circuit " << i);
	      print(*(cs[i]));
	   }
	   */
	   return cs;
	}

	bool contains_measurements(circuit x)
	{
	   for (size_t i=0; i<x.size(); i++)
	   {
	      if (x[i]->type() == __measure_gate__)
		 return true;
	      if (x[i]->type() == __prepz_gate__)
		 return true;
	   }
	   return false;
	}



      protected:

	std::string name;
	circuit     c;
        size_t      iterations;
   };


   /**
    * quantum_program_
    */
   class quantum_program
   {
      public:

        quantum_program(std::string name, size_t qubits) : name(name), qubits(qubits), output_path("output/"), default_config(true)
	{
	}

	void add(ql::quantum_kernel k)
	{
	   kernels.push_back(k);
	}

	void set_config_file(std::string file_name)
	{
	   config_file_name = file_name;
	   default_config   = false;
	}

	void set_ouput_path(std::string dir="output/")
	{
	   output_path = dir;
	}

	std::string qasm()
	{
	   std::stringstream ss;
	   ss << "# this file has been automatically generated by the OpenQL compiler please do not modify it manually.\n";
	   ss << "qubits " << qubits << "\n";
	   for (size_t k=0; k<kernels.size(); ++k)
	      ss <<'\n' << kernels[k].qasm();
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
	   return ss.str();
	}

	std::string microcode()
	{
	   std::stringstream ss;
	   ss << "# this file has been automatically generated by the OpenQL compiler please do not modify it manually.\n";
	   ss << uc_header();
	   for (size_t k=0; k<kernels.size(); ++k)
	      ss <<'\n' << kernels[k].micro_code();
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

	   ss << "     beq  r3,  r3, loop   # infinite loop";
	   return ss.str();
	}

	void set_platform(ql_platform_t platform)
	{
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


	void write_file(std::string file_name, std::string& content)
	{
	   std::ofstream file;
	   file.open(file_name);
	   file << content;
	   file.close();
	}

	int compile(bool verbose=false)
	{
	   if (!ql::initialized)
	   {
	      println("[x] error : openql should initialized for the target platform before compilation !");
	      return -1;
	   }
	   if (verbose) println("[+] compiling ...");
	   if (kernels.empty())
	      return -1;
	   #ifdef ql_optimize
	   for (size_t k=0; k<kernels.size(); ++k)
	      kernels[k].optimize();
	   #endif // ql_optimize


	   std::stringstream ss_qasm;
	   ss_qasm << output_path << name << ".qasm";
	   std::string s = qasm();

	   if (verbose) println("[+] writing qasm to '" << ss_qasm.str() << "' ...");
	   write_file(ss_qasm.str(),s);

	   // println("[+] sweep_points : ");
	   // for (int i=0; i<sweep_points.size(); i++) println(sweep_points[i]);

	   std::stringstream ss_asm;
	   ss_asm << output_path << name << ".asm";
	   std::string uc = microcode();

	   if (verbose) println("[+] writing transmon micro-code to '" << ss_asm.str() << "' ...");
	   write_file(ss_asm.str(),uc);

	   std::stringstream ss_swpts;
	   ss_swpts << "{ \"measurement_points\" : [";
	   for (int i=0; i<sweep_points.size()-1; i++)
	      ss_swpts << sweep_points[i] << ", ";
	   ss_swpts << sweep_points[sweep_points.size()-1] << "] }";
	   std::string config = ss_swpts.str();
	   if (default_config)
	   {
	      std::stringstream ss_config;
	      ss_config << output_path << name << "_config.json";
	      std::string conf_file_name = ss_config.str();
	      if (verbose) println("[+] writing sweep points to '" << conf_file_name << "'...");
	      write_file(conf_file_name, config);
	   }
	   else
	   {
	      std::stringstream ss_config;
	      ss_config << output_path << config_file_name;
	      std::string conf_file_name = ss_config.str();
	      if (verbose) println("[+] writing sweep points to '" << conf_file_name << "'...");
	      write_file(conf_file_name, config);
	   }


	   return 0;
	}

	void schedule()
	{
		std::cout << "Scheduling the quantum program" << std::endl;
		for (auto k : kernels)
			k.schedule(qubits);
	}

	void set_sweep_points(float * swpts, size_t size)
	{
	   sweep_points.clear();
	   for (size_t i=0; i<size; ++i)
	      sweep_points.push_back(swpts[i]);
	}

      protected:

	ql_platform_t               platform;
	std::vector<quantum_kernel> kernels;
	std::vector<float>          sweep_points;
	std::string                 name;
	std::string                 output_path;
	std::string                 config_file_name;
	bool                        default_config;
	size_t                      qubits;
   };

   /**
    * utils
    */
   std::string qasm(ql::circuit c)
   {
      std::stringstream ss;
      for (size_t i=0; i<c.size(); ++i)
      {
	 ss << c[i]->qasm() << "\n";
	 std::cout << c[i]->qasm() << std::endl;
      }
      return ss.str();
   }

   namespace utils
   {
      /**
       * @param str
       *    string to be processed
       * @param seq
       *    string to be replaced
       * @param rep
       *    string used to replace seq
       * @brief
       *    replace recursively seq by rep in str
       */
      void replace_all(std::string &str, std::string seq, std::string rep)
      {
	 int index = str.find(seq);
	 while (index < str.size())
	 {
	    str.replace(index, seq.size(), rep);
	    index = str.find(seq);
	 }
      }

      /**
       * string starts with " and end with "
       * return the content of the string between the commas
       */
      bool format_string(std::string& s)
      {
	 replace_all(s,"\\n","\n");
	 size_t pf = s.find("\"");
	 if (pf == std::string::npos)
	    return false;
	 size_t ps = s.find_last_of("\"");
	 if (ps == std::string::npos)
	    return false;
	 if (ps == pf)
	    return false;
	 s = s.substr(pf+1,ps-pf-1);
	 return true;
      }
   }

}

#endif // OPENQL_H
