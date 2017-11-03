/**
 * @file    quantum_state_loader.h
 * @author   Nader Khammassi 
 * @date      06-04-16
 * @brief   quantum state loader (qx)
 */


#ifndef QX_QUANTUM_STATE_LOADER
#define QX_QUANTUM_STATE_LOADER

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <complex>

#include <map>

#include <stdint.h>

#include <ql/str.h>
// #include <core/linalg.h>

using namespace str;
// using namespace qx::linalg;

// #define println(x) std::cout << x << std::endl;
#define error(x) std::cout << "[x] error : " << x << std::endl;

#define __max_qubits__ 32

#ifndef println
#define println(x) std::cout << x << std::endl
#endif

namespace qx
{
   typedef uint64_t                          basis_state_t;
   typedef std::complex<double>              complex_t;
   typedef std::map<basis_state_t,complex_t> quantum_state_t;

   /**
    * quantum_code parser
    */
   class quantum_state_loader
   {
      public:

         quantum_state_loader(std::string& file_name, size_t qubits_count) : file_name(file_name), qubits_count(qubits_count), line_index(0), syntax_error(false)
         {
            state = new quantum_state_t;
         }

         int32_t load()
         {
            // open file
            line_index   = 0;
            syntax_error = false;
            println("[-] loading quantum_state file '" << file_name << "'...");
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
               if (syntax_error)
               {
                  exit(-1);
               }
               println("[+] code loaded successfully. ");
               return 0;
            }
            else 
            {
               error("cannot open file " << file_name << ", the specified file does not exist !");
               exit(-1);
               return -1;
            }
         }

         quantum_state_t * get_quantum_state()
         {
            return state;
         }

         double parse_double(std::string& val)
         {
            return atof(val.c_str());
         }

         basis_state_t parse_basis_state(std::string& s)
         {
            replace_all(s,"|","");
            replace_all(s,">","");
            if (s.length() != qubits_count)
            {
               error(" in '" << file_name << "' at line " << line_index << " : qubits number of the state basis does not match the defined qubits number ! ");
               exit(-1);
            }
            return std::bitset<__max_qubits__>(s).to_ulong();
         }

         int32_t process_line(std::string& line)
         {
            // entry structure:
            // 0.00000 0.00000 |000000>
            format_line(line);
            format_line(line);
            if (str::is_empty(line))
               return 0;
            if (line[0] == '#') // skip comments
               return 0;

            strings words = word_list(line, " ");
            if (words.size() != 3)
            {
               println("[x] error : malformed quantum state file !");
               syntax_error = true;
               return 1;
            }
            double        real   = parse_double(words[0]);
            double        img    = parse_double(words[1]);
            basis_state_t st     = parse_basis_state(words[2]);
            complex_t     c(real,img);
            // println("[#] [ " << c << " -> " << st << " ]");
            quantum_state_t& rstate = *state; 
            rstate[st] = c;
            return 0;
         }

      private:

         std::string       file_name;
         quantum_state_t * state;
         uint32_t          qubits_count;
         uint32_t          line_index;
         bool              syntax_error;

   };
}

#undef println

#endif // QX_QUANTUM_STATE_LOADER




