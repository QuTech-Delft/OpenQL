/**
 * @file   utils.h
 * @date   04/2017
 * @author Nader Khammassi
 * @brief  string utils (from qx)
 */

#ifndef QL_UTILS_H
#define QL_UTILS_H

#include <string>

namespace ql
{
   /**
    * utils
    */
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

   } // utils
} // ql

#endif //QL_UTILS_H

