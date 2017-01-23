/**
 * @file   instruction_map.h
 * @date   11/2016
 * @author Nader Khammassi
 * @brief  instruction map loading test
 */

#include <iostream>
#include <map>
#include <fstream>

#define println(x) std::cout << x << std::endl

#ifndef INSTR_MAP
#define INSTR_MAP "instructions.map"
#endif

typedef std::string qasm_inst_t;
typedef std::string ucode_inst_t;

typedef std::map<qasm_inst_t, ucode_inst_t> instruction_map_t;

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
inline void replace_all(std::string &str, std::string seq, std::string rep)
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

      if (!format_string(key))
      {
	 println("[+] syntax error at line " << i << " : invalid key format.");
	 return false;
      }

      if (!format_string(val))
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



int main(int argc, char ** argv)
{
   instruction_map_t imap;

   println("[+] loading instruction map ...");
   if (!load_instruction_map(INSTR_MAP,imap))
      println("[x] error : failed to load the instruction map !");
   else
   {
      println("[+] instruction map loaded successfully !");

      for (instruction_map_t::iterator i=imap.begin(); i!=imap.end(); i++)
	 println("[ " << (*i).first <<  " --> " << (*i).second << " ]");
   }

   return 0;
}
