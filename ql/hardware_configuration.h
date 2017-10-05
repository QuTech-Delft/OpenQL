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

#include <sstream>
#include <algorithm>
#include <iterator>

#include <ql/openql.h>
#include <ql/exception.h>
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
    void load(ql::instruction_map_t& instruction_map, json& instruction_settings, json& hardware_settings,
              json& resources, json& topology  ) throw (ql::exception)
    {
        json config;
        try
        {
            config = load_json(config_file_name);
        }
        catch (json::exception e)
        {
            throw ql::exception("[x] error : ql::hardware_configuration::load() :  failed to load the hardware config file : malformed json file ! : \n\t"+
                                std::string(e.what()),false);
        }

        // load eqasm compiler backend
        if (config["eqasm_compiler"].is_null())
        {
            println("[x] error : ql::hardware_configuration::load() : eqasm compiler backend is not specified in the hardware config file !");
            // throw std::exception();
            throw ql::exception("[x] error : ql::hardware_configuration::load() : eqasm compiler backend is not specified in the hardware config file !",false);
        }
        else
        {
            eqasm_compiler_name = config["eqasm_compiler"];
        }

        // load hardware_settings
        if (config["hardware_settings"].is_null())
        {
            println("[x] error : ql::hardware_configuration::load() : 'hardware_settings' section is not specified in the hardware config file !");
            // throw std::exception();
            throw ql::exception("[x] error : ql::hardware_configuration::load() : 'hardware_settings' section is not specified in the hardware config file !",false);
        }
        else
        {
            hardware_settings    = config["hardware_settings"];
        }

        // load instruction_settings
        if (config["instructions"].is_null())
        {
            println("[x] error : ql::hardware_configuration::load() : 'instructions' section is not specified in the hardware config file !");
            throw ql::exception("[x] error : ql::hardware_configuration::load() : 'instructions' section is not specified in the hardware config file !",false);
        }
        else
        {
            instruction_settings = config["instructions"];
        }

        // create the control store

        // load platform resources
        if (config["resources"].is_null())
        {
            println("[x] error : ql::hardware_configuration::load() : 'resources' section is not specified in the hardware config file !");
            throw ql::exception("[x] error : ql::hardware_configuration::load() : 'resources' section is not specified in the hardware config file !",false);
        }
        else
        {
            resources = config["resources"];
        }

        // load platform topology
        if (config["topology"].is_null())
        {
            println("[x] error : ql::hardware_configuration::load() : 'topology' section is not specified in the hardware config file !");
            throw ql::exception("[x] error : ql::hardware_configuration::load() : 'topology' section is not specified in the hardware config file !",false);
        }
        else
        {
            topology = config["topology"];
        }

        // load instructions
        json instructions = config["instructions"];
        // std::cout << instructions.dump(4) << std::endl;
        for (json::iterator it = instructions.begin(); it != instructions.end(); ++it)
        {
            std::string  name = it.key();
            str::lower_case(name);
            json         attr = *it; //.value();

            // check for duplicate operations
            if (instruction_map.find(name) != instruction_map.end())
                println("[!] warning : ql::hardware_configuration::load() : instruction '" << name << "' redefined : the old definition is overwritten !");

            instruction_map[name] = load_instruction(name, attr);
            println("instruction " << name << " loaded.");
        }

        // load gate decomposition/aliases
        if (!config["gate_decomposition"].is_null())
        {
            json gate_decomposition = config["gate_decomposition"];
            for (json::iterator it = gate_decomposition.begin(); it != gate_decomposition.end(); ++it)
            {
                std::string  comp_ins = it.key();
                str::lower_case(comp_ins);
                std::cout << "\n[GD] Adding composite instr : " << comp_ins << std::endl;
                std::replace( comp_ins.begin(), comp_ins.end(), ',', ' ');                
                std::cout << "[GD] Adjusted composite instr : " << comp_ins << std::endl;

                // check for duplicate operations
                if (instruction_map.find(comp_ins) != instruction_map.end())
                    println("[!] warning : ql::hardware_configuration::load() : composite instruction '" << comp_ins << "' redefined : the old definition is overwritten !");

                json sub_instructions = *it;
                if (!sub_instructions.is_array())
                    throw ql::exception("[x] error : ql::hardware_configuration::load() : 'gate_decomposition' section : gate '"+comp_ins+"' is malformed !",false);

                std::vector<gate *> gs;
                for (size_t i=0; i<sub_instructions.size(); i++)
                {
                    std::string sub_ins = sub_instructions[i];
                    str::lower_case(sub_ins);
                    std::cout << "[GD] Adding sub instr: " << sub_ins << std::endl;

                    std::replace( sub_ins.begin(), sub_ins.end(), ',', ' ');

                    std::istringstream iss(sub_ins);
                    std::vector<std::string> tokens{ std::istream_iterator<std::string>{iss},
                                     std::istream_iterator<std::string>{} };

                    std::string sub_ins_adjusted = tokens[0] + " ";
                    for(size_t t=1; t<tokens.size()-1; t++)
                    {
                        sub_ins_adjusted += "%" + std::to_string(t-1) + " ";
                    }
                    if(tokens.size() > 1)
                    {
                        sub_ins_adjusted += "%" + std::to_string(tokens.size()-2);
                    }

                    std::cout << "[GD] Adjusted sub instr: " << sub_ins_adjusted << std::endl;

                    if ( instruction_map.find(sub_ins_adjusted) != instruction_map.end() )
                    {
                        // using existing sub ins
                        std::cout << "[GD] using existing sub instr : " << sub_ins_adjusted << std::endl;
                        gs.push_back( instruction_map[sub_ins_adjusted] );
                    }
                    else
                    {
                        // adding new sub ins
                        std::cout << "[GD] adding new sub instr : " << sub_ins_adjusted << std::endl;
                        instruction_map[sub_ins_adjusted] = new composite_gate(sub_ins_adjusted);
                        gs.push_back( instruction_map[sub_ins_adjusted] );
                    }
                }
                instruction_map[comp_ins] = new composite_gate(comp_ins, gs);
            }
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
            println("[!] warning : hardware_configuration::load() : alias '" << name << "' detected but ignored (not supported yet : please define your instruction).");
            return g;
        }
        try
        {
            g->load(instr);
        }
        catch (ql::exception e)
        {
            println("[e] error while loading instruction '" << name << "' : " << e.what());
            throw e;
            // ql::exception("[x] error : hardware_configuration::load_instruction() : error while loading instruction '" + name + "' : " + e.what(),false);
        }
        // g->print_info();
        return g;
    }

public:

    std::string       config_file_name;
    std::string       eqasm_compiler_name;

};
}
