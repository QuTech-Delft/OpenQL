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

#ifndef println
#define println(x) std::cout << x << std::endl
#endif // println


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

} // ql

#endif // QL_INSTRUCTION_MAP

