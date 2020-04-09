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

#include <compile_options.h>
#include "utils.h"
#include "gate.h"

namespace ql
{

#if OPT_MICRO_CODE
typedef std::string qasm_inst_t;
typedef std::string ucode_inst_t;
#endif

typedef std::map<std::string, ql::custom_gate *> instruction_map_t;
#if OPT_MICRO_CODE
typedef std::map<qasm_inst_t, ucode_inst_t> dep_instruction_map_t;
#endif


namespace utils
{
    bool format_string(std::string& s);
    void replace_all(std::string &str, std::string seq, std::string rep);
}

// FIXME: move to hardware_configuration.h
inline json load_json(std::string file_name)
{
    std::ifstream fs(file_name);
    json j;
    if (fs.is_open())
    {
        std::stringstream stripped;     // file contents with comments stripped
        std::string line;

        // strip comments
        while (getline(fs, line))
        {
            std::string::size_type n = line.find("//");
            if (n != std::string::npos) line.erase(n);
            std::istringstream iss(line);
            stripped << line;
        }

        try
        {
            stripped >> j;  // pass stripped line to json. NB: the whole file must be passed in 1 go
        }
        // treat parse errors separately to give the user a clue about what's wrong
        catch (json::parse_error &e)
        {
            EOUT("error parsing JSON file : \n\t" << e.what());
            if(e.byte != 0)
            {
                // go through file once again to find error position
                unsigned int lineNr = 1;
                size_t absPos = 0;
                fs.clear();
                fs.seekg(0, std::ios::beg);
                while (getline(fs, line)) {
                    std::string::size_type n = line.find("//");
                    if (n != std::string::npos) line.erase(n);
                    if(e.byte >= absPos && e.byte < absPos+line.size()) {
                        unsigned int relPos = e.byte-absPos;
                        str::replace_all(line, "\t", " ");                                      // make a TAB take one position
                        FATAL("in line " << lineNr <<
                              " at position " << relPos << ":" << std::endl <<
                              line << std::endl <<                                              // print offending line
                              std::string(relPos>0 ? relPos-1 : 0, ' ') << "^" << std::endl);   // print marker
                        break;
                    }
                    lineNr++;
                    absPos += line.size();
                }
                FATAL("error position " << e.byte << " points beyond last file position " << absPos);
            } else {
                FATAL("no information on error position");
            }
        }
        catch (json::exception &e)
        {
            FATAL("malformed JSON file : \n\t" << e.what());
        }
    }
    else
    {
        FATAL("failed to open file '" << file_name << "'");
    }
    return j;
}

#if OPT_CUSTOM_GATE_LOAD    // FIXME: unused and similar to custom_gate::load()
inline int load_instructions(instruction_map_t& instruction_map, std::string file_name="instructions.json")
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
#if OPT_MICRO_CODE
        ucode_sequence_t ucs = instr["qumis"];
        g->qumis.assign(ucs.begin(), ucs.end());
#endif
        std::string t = instr["type"];
        instruction_type_t type = (t == "rf" ? rf_t : flux_t );
        g->operation_type = type;
        g->duration = instr["duration"];
#if OPT_USED_HARDWARE
        strings_t hdw = instr["hardware"];
        g->used_hardware.assign(hdw.begin(), hdw.end());
#endif
        auto mat = instr["matrix"];
        g->m.m[0] = complex_t(mat[0][0], mat[0][1]);
        g->m.m[1] = complex_t(mat[1][0], mat[1][1]);
        g->m.m[2] = complex_t(mat[2][0], mat[2][1]);
        g->m.m[3] = complex_t(mat[3][0], mat[3][1]);
        instruction_map[instruction_name] = g;
    }
    return 0;
}
#endif

} // ql

#endif // QL_INSTRUCTION_MAP

