/**
 * @file   hardware_configuration.h
 * @date   07/2017
 * @author Nader Khammassi
 *         Imran Ashraf
 * @brief  hardware configuration loader
 */

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <typeinfo>
#include <cmath>

#include <sstream>
#include <algorithm>
#include <iterator>
#include <regex>

#include <instruction_map.h>
#include <json.h>
#include <exception.h>

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
    void load(ql::instruction_map_t& instruction_map, json& instruction_settings, json& hardware_settings,
              json& resources, json& topology, json& aliases )
    {
        json config;
        try
        {
            config = load_json(config_file_name);
        }
        catch (json::exception &e)
        {
            FATAL("failed to load the hardware config file : malformed json file: \n\t" << std::string(e.what()));
        }

        // load eqasm compiler backend
        if (config.count("eqasm_compiler") <= 0)
        {
            FATAL("'eqasm_compiler' is not specified in the hardware config file");
        }
        else
        {
            eqasm_compiler_name = config["eqasm_compiler"].get<std::string>();
        }

        // load hardware_settings
        if (config.count("hardware_settings") <= 0)
        {
            FATAL("'hardware_settings' section is not specified in the hardware config file");
        }
        else
        {
            hardware_settings = config["hardware_settings"];
        }

        // load instruction_settings
        if (config.count("instructions") <= 0)
        {
            FATAL("'instructions' section is not specified in the hardware config file");
        }
        else
        {
            instruction_settings = config["instructions"];
        }

        // load platform resources
        if (config.count("resources") <= 0)
        {
            FATAL("'resources' section is not specified in the hardware config file");
        }
        else
        {
            resources = config["resources"];
        }

        // load platform topology
        if (config.count("topology") <= 0)
        {
            FATAL("'topology' section is not specified in the hardware config file");
        }
        else
        {
            topology = config["topology"];
        }

        // load instructions
        const json &instructions = config["instructions"];
        static const std::regex comma_space_pattern("\\s*,\\s*");

        for (auto it = instructions.begin(); it != instructions.end(); ++it)
        {
            std::string name = it.key();
            json attr = *it; //.value();

            name = sanitize_instruction_name(name);
            name = std::regex_replace(name, comma_space_pattern, ",");

            // check for duplicate operations
            if (instruction_map.find(name) != instruction_map.end())
                WOUT("instruction '" << name << "' redefined : the old definition is overwritten !");

            // format in json.instructions:
            //  "^(\s)*token(\s)*[(\s)token(\s)*(,(\s)*token(\s*))*]$"
            //  so with a comma between any operands and possible spaces everywhere
            //
            // format of key and value (which is a custom_gate)'s name in instruction_map:
            //  "^(token|(token token(,token)*))$"
            //  so with a comma between any operands
            instruction_map[name] = load_instruction(name, attr);
            DOUT("instruction " << name << " loaded.");
        }

        // load optional section gate_decomposition
        // Examples:
        // - Parametrized gate-decomposition: "cl_2 %0": ["rxm90 %0", "rym90 %0"]
        // - Specialized gate-decomposition:  "rx180 q0" : ["x q0"]


        if (config.count("gate_decomposition") > 0)
        {
            const json &gate_decomposition = config["gate_decomposition"];
            for (auto it = gate_decomposition.begin(); it != gate_decomposition.end(); ++it)
            {
                // standardize instruction name
                std::string  comp_ins = it.key();
                DOUT("");
                DOUT("Adding composite instr : " << comp_ins);
                comp_ins = sanitize_instruction_name(comp_ins);
                comp_ins = std::regex_replace(comp_ins, comma_space_pattern, ",");
                DOUT("Adjusted composite instr : " << comp_ins);

                // format in json.instructions:
                //  "^(\s)*token(\s)+token(\s)*(,|\s)(\s)*token(\s*)$"
                //  so with a comma or a space between any operands and possible spaces everywhere
                //
                // format of key and value (which is a custom_gate)'s name in instruction_map:
                //  "^(token(\stoken)*))$"
                //  so with one space between any operands

                // check for duplicate operations
                if (instruction_map.find(comp_ins) != instruction_map.end())
                    WOUT("composite instruction '" << comp_ins << "' redefined : the old definition is overwritten !");

                // check that we're looking at array
                json sub_instructions = *it;
                if (!sub_instructions.is_array())
                    FATAL("ql::hardware_configuration::load() : 'gate_decomposition' section : gate '" << comp_ins << "' is malformed (not an array)");

                std::vector<gate *> gs;
                for (size_t i=0; i<sub_instructions.size(); i++)
                {
                    // standardize name of sub instruction
                    std::string sub_ins = sub_instructions[i];
                    DOUT("Adding sub instr: " << sub_ins);
                    sub_ins = sanitize_instruction_name(sub_ins);
                    sub_ins = std::regex_replace(sub_ins, comma_space_pattern, ",");
                    if ( instruction_map.find(sub_ins) != instruction_map.end() )
                    {
                        // using existing sub ins, e.g. "x q0" or "x %0"
                        DOUT("using existing sub instr : " << sub_ins);
                        gs.push_back( instruction_map[sub_ins] );
                    }
                    else if( sub_ins.find("%") != std::string::npos )               // parameterized composite gate? FIXME: no syntax check
                    {
                        // adding new sub ins if not already available, e.g. "x %0"
                        DOUT("adding new sub instr : " << sub_ins);
                        instruction_map[sub_ins] = new custom_gate(sub_ins);
                        gs.push_back( instruction_map[sub_ins] );
                    }
#if OPT_DECOMPOSE_WAIT_BARRIER   // allow wait/barrier, e.g. "barrier q2,q3,q4"
                    else
                    {
                        // FIXME: just save whatever we find as a *custom* gate (there is no better alternative)
                        // FIXME: also see additions (hacks) to kernel.h
                        DOUT("adding new sub instr : " << sub_ins);
                        instruction_map[sub_ins] = new custom_gate(sub_ins);
                        gs.push_back( instruction_map[sub_ins] );
                    }
#else
                    else
                    {
                        // for specialized custom instructions, raise error if instruction
                        // is not already available
                        FATAL("custom instruction not found for '" << sub_ins << "'");
                    }
#endif
                }
                instruction_map[comp_ins] = new composite_gate(comp_ins, gs);
            }
        }

        // FIXME: code commented out: unfinished alias support (TBC)
        // // load aliases
        // if (config.count("aliases") > 0)
        // {
        //    aliases = config["aliases"];
        //    for (json::iterator it = aliases.begin(); it != aliases.end(); ++it)
        //    {
        //       std::string  name = it.key();
        //       println("loading alias " << name);
        //       str::lower_case(name);
        //       json         attr = *it; //.value();
        //       // check for duplicate operations
        //       if (instruction_map.find(name) != instruction_map.end())
        //          println("[!] warning : ql::hardware_configuration::load() : composite instruction '" << name << "' redefined : the old definition is overwritten !");

        //       if (!attr.is_array())
        //          throw ql::exception("[x] error : ql::hardware_configuration::load() : 'gate_decomposition' section : gate '"+name+"' is malformed !",false);
        //       std::vector<gate *> gs;
        //       for (size_t i=0; i<attr.size(); ++i)
        //       {
        //          std::string instr_name = attr[i];
        //          // println("checking if instruction '" << instr_name << "' is already defined...");
        //          if (instruction_map.find(instr_name) == instruction_map.end())
        //             throw ql::exception("[x] error : ql::hardware_configuration::load() : 'gate_decomposition' section : instruction "+instr_name+" composing gate '"+name+"' is not defined !",false);
        //          gs.push_back(instruction_map[instr_name]);
        //       }
        //       instruction_map[name] = new composite_gate(name,gs);
        //    }
        // }
    }

private:
    /**
     * load_instruction
     */
    // FIXME: replace by gate.h::custom_gate(std::string& name, json& instr)
    ql::custom_gate * load_instruction(std::string name, json& instr)
    {
        custom_gate * g = new custom_gate(name);
        // skip alias fo now
        if (instr.count("alias") > 0)
        {
            // todo : look for the target aliased gate
            //        copy it with the new name
            WOUT("alias '" << name << "' detected but ignored (not supported yet : please define your instruction).");
            return g;
        }
        try
        {
            g->load(instr);
        }
        catch (ql::exception &e)
        {
            EOUT("error while loading instruction '" << name << "' : " << e.what());
            throw e;
            // ql::exception("[x] error : hardware_configuration::load_instruction() : error while loading instruction '" + name + "' : " + e.what(),false);
        }
        // g->print_info();
        return g;
    }

public:

    std::string       config_file_name;
    std::string       eqasm_compiler_name;

private:

    static const std::regex trim_pattern;
    static const std::regex multiple_space_pattern;

    /**
    * Sanitizes the name of an instruction by converting to lower case and removing the unnecessary spaces.
    */
    static std::string sanitize_instruction_name(std::string name)
    {
        str::lower_case(name);
        name = std::regex_replace(name, trim_pattern, "");
        name = std::regex_replace(name, multiple_space_pattern, " ");
        return name;
    }

};

const std::regex hardware_configuration::trim_pattern("^(\\s)+|(\\s)+$");
const std::regex hardware_configuration::multiple_space_pattern("(\\s)+");

}
