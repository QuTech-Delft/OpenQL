/**
 * @file   instruction_map.h
 * @date   04/2017
 * @author Nader Khammassi
 * @brief  instruction map 
 */

#ifndef QL_INSTRUCTION_MAP
#define QL_INSTRUCTION_MAP

#include <iostream>
#include <fstream>
#include <map>

#include "utils.h"
#include "gate.h"

namespace ql
{

    typedef std::string qasm_inst_t;
    typedef std::string ucode_inst_t;

    typedef std::map<qasm_inst_t, ucode_inst_t> instruction_map_t;
    
    namespace utils
    {
       bool format_string(std::string& s);
       void replace_all(std::string &str, std::string seq, std::string rep);
    }

    // bool load_instruction_map(std::string file_name, instruction_map_t& imap);

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
	  println("line " << i << " : " << line);
	  #endif
	  size_t p = line.find(":");
	  if (line.size() < 3) continue;
	  if (p == std::string::npos)
	  {
	     println("syntax error at line " << i << " : invalid syntax.");
	     return false;
	  }
	  std::string key = line.substr(0,p);
	  std::string val = line.substr(p+1,line.size()-p-1);

	  if (!utils::format_string(key))
	  {
	     println("syntax error at line " << i << " : invalid key format.");
	     return false;
	  }

	  if (!utils::format_string(val))
	  {
	     println("syntax error at line " << i << " : invalid value format.");
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

    inline json load_json(std::string file_name)
    {
       std::ifstream fs(file_name);
       json j;
       if (fs.is_open())
       {
	  try 
	  {
	     fs >> j;
	  } catch (std::exception e)
	  {
	     println("[x] error : malformed json file : \n\t" << e.what());
	  }
       }
       else
	  println("[x] error : failed to open file '" << file_name << "' !");
      return j;
    }


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
	  // g->latency = instr["latency"];
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

} // ql

#endif // QL_INSTRUCTION_MAP

