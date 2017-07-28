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
	   void load(std::vector<ql::custom_gate*>& supported_gates, json& hardware_settings)
	   {
	      json config = load_json(config_file_name);
	      hardware_settings = config["hardware_settings"];
	      json instructions = config["instructions"];
	      // std::cout << instructions.dump(4) << std::endl;
	      for (json::iterator it = instructions.begin(); it != instructions.end(); ++it) 
	      {
		 std::string  name = it.key();
		 json         attr = *it; //.value();
		 supported_gates.push_back(load_instruction(name,attr));
		 // std::cout << it.key() << " : " << it.value() << "\n";
	      }
	   }

	   ql::custom_gate * load_instruction(std::string name, json& instr)
	   {
	      custom_gate * g = new custom_gate(name);
	      // skip alias fo now
	      if (!instr["alias"].is_null()) // != "null")
	      {
		 // todo : look for the target aliased gate 
		 //        copy it with the new name
		 println("[i] alias '" << name << "'detected and skipped.");
		 return g;
	      }
	      try 
	      {
		 g->load(instr);
	      } catch (json::exception e)
	      {
		 println("[e] error while loading instruction '" << name << "' : " << e.what());
	      }
	      g->print_info();
	      return g;
	   }


	 protected:

	   std::string config_file_name;

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
