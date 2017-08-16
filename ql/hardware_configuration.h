/**
 * @file   hardware_configuration.h
 * @date   07/2017
 * @author Nader Khammassi
 * @brief  hardware configuration loader 
 */

#include <iostream>
#include <fstream>
#include <string> 
#include <map>

#include <typeinfo>

#include <cmath>

#include <ql/openql.h>
#include <ql/json.h>
#include <ql/gate.h>



namespace ql
{

      typedef std::map<std::string,ql::custom_gate *> instruction_map_t;

      /**
       * loading hardware configuration
       */
      class hardware_configuration
      {

	 public:

	  /** 
	   * ctor
	   */
	   hardware_configuration(std::string config_file_name) : config_file_name(config_file_name)
	   {
	   }

	  /**
	   * load
	   */
	   void load(ql::instruction_map_t& instruction_map, json& instruction_settings, json& hardware_settings)
	   {
	      json config = load_json(config_file_name);

	      // load eqasm compiler backend
	      if (config["eqasm_compiler"].is_null())
	      {
		 println("[x] error : eqasm compiler backend is not specified in the hardware configuration file !");
		 throw std::exception();
	      }
	      else
		 eqasm_compiler_name = config["eqasm_compiler"];

	      // load hardware_settings 
	      if (config["hardware_settings"].is_null())
	      {
		 println("[x] error : hardware settings are not specified in the hardware configuration file !");
		 throw std::exception();
	      }
	      else
	      {
		 hardware_settings    = config["hardware_settings"];
	      }

	      // load instruction_settings
	      if (config["instructions"].is_null())
	      {
		 println("[x] error : instructions settings are not specified in the hardware configuration file !");
		 throw std::exception();
	      }
	      else
	      {
		 instruction_settings = config["instructions"];
	      }


	      json instructions = config["instructions"];
	      // std::cout << instructions.dump(4) << std::endl;
	      for (json::iterator it = instructions.begin(); it != instructions.end(); ++it) 
	      {
		 std::string  name = it.key();
		 str::lower_case(name);
		 // println("'" << name << "'");
		 json         attr = *it; //.value();
		 // supported_gates.push_back(load_instruction(name,attr));
		 // check for duplicate operations
		 if (instruction_map.find(name) != instruction_map.end())
		    println("[!] warning : instruction '" << name << "' redefined : the old definition is overwritten !");
		 instruction_map[name] = load_instruction(name,attr);
		 // std::cout << it.key() << " : " << it.value() << "\n";
	      }
	   }

	   /**
	    * load_instruction
	    */
	   ql::custom_gate * load_instruction(std::string name, json& instr)
	   {
	      custom_gate * g = new custom_gate(name);
	      // skip alias fo now
	      if (!instr["alias"].is_null()) // != "null")
	      {
		 // todo : look for the target aliased gate 
		 //        copy it with the new name
		 println("[!] alias '" << name << "' detected but skipped (not supported yet).");
		 return g;
	      }
	      try 
	      {
		 g->load(instr);
	      } catch (json::exception e)
	      {
		 println("[e] error while loading instruction '" << name << "' : " << e.what());
	      }
	      // g->print_info();
	      return g;
	   }

	 public:

	   std::string       config_file_name;
	   std::string       eqasm_compiler_name;    

	 private:

/*
	   int load_instructions(std::map<std::string, custom_gate *>& instruction_map, std::string file_name="instructions.json")
	   {
	      json instructions = load_json(file_name);
	      // std::cout << instructions.dump(4) << std::endl;
	      for (json::iterator it = instructions.begin(); it != instructions.end(); ++it) 
	      {
		 // std::cout << it.key() << " : " << it.value() << "\n";
		 std::string instruction_name = it.key();
		 json instr = it.value();
		 custom_gate * g = new custom_gate(instruction_name);
		 g->name = instruction_name; // instr["name"];
		 g->parameters = instr["parameters"];
		 ucode_sequence_t ucs = instr["qumis"];
		 g->qumis.assign(ucs.begin(), ucs.end());
		 std::string t = instr["type"];
		 instruction_type_t type = (t == "rf" ? rf_t : flux_t );
		 g->operation_type = type;
		 g->duration = instr["duration"];
		 g->latency = instr["latency"];
		 strings_t hdw = instr["hardware"];
		 g->used_hardware.assign(hdw.begin(), hdw.end());
		 auto mat = instr["matrix"];
		 g->m.m[0] = complex_t(mat[0][0], mat[0][1]);
		 g->m.m[1] = complex_t(mat[1][0], mat[1][1]);
		 g->m.m[2] = complex_t(mat[2][0], mat[2][1]);
		 g->m.m[3] = complex_t(mat[3][0], mat[3][1]);
		 instruction_map[instruction_name] = g;
	      }
	      return 0;
	   }
*/
      };
}
