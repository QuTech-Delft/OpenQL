/** \file
 * JSON hardware configuration loader.
 */

#include "hardware_configuration.h"

#include <regex>

namespace ql {

using namespace utils;

static const std::regex trim_pattern("^(\\s)+|(\\s)+$");
static const std::regex multiple_space_pattern("(\\s)+");

/**
 * Sanitizes the name of an instruction by converting to lower case and removing
 * the unnecessary spaces.
 */
static Str sanitize_instruction_name(Str name) {
    name = to_lower(name);
    name = std::regex_replace(name, trim_pattern, "");
    name = std::regex_replace(name, multiple_space_pattern, " ");
    return name;
}

static custom_gate *load_instruction(const Str &name, Json &instr) {
    custom_gate *g = new custom_gate(name);
    // skip alias fo now
    if (instr.count("alias") > 0) {
        // todo : look for the target aliased gate
        //        copy it with the new name
        QL_WOUT("alias '" << name << "' detected but ignored (not supported yet : please define your instruction).");
        return g;
    }
    try {
        g->load(instr);
    } catch (Exception &e) {
        QL_EOUT("error while loading instruction '" << name << "' : " << e.what());
        throw;
        // ql::exception("[x] error : hardware_configuration::load_instruction() : error while loading instruction '" + name + "' : " + e.what(),false);
    }
    // g->print_info();
    return g;
}

hardware_configuration::hardware_configuration(
    const Str &config_file_name
) : config_file_name(config_file_name) {
}

void hardware_configuration::load(
    ql::instruction_map_t &instruction_map,
    Json &instruction_settings,
    Json &hardware_settings,
    Json &resources,
    Json &topology,
    Json &aliases
) {
    Json config;
    try {
        config = load_json(config_file_name);
    } catch (Json::exception &e) {
        QL_FATAL(
            "failed to load the hardware config file : malformed json file: \n\t"
                << Str(e.what()));
    }

    // load eqasm compiler backend
    if (config.count("eqasm_compiler") <= 0) {
        QL_FATAL("'eqasm_compiler' is not specified in the hardware config file");
    } else {
        eqasm_compiler_name = config["eqasm_compiler"].get<Str>();
    }

    // load hardware_settings
    if (config.count("hardware_settings") <= 0) {
        QL_FATAL(
            "'hardware_settings' section is not specified in the hardware config file");
    } else {
        hardware_settings = config["hardware_settings"];
    }

    // load instruction_settings
    if (config.count("instructions") <= 0) {
        QL_FATAL(
            "'instructions' section is not specified in the hardware config file");
    } else {
        instruction_settings = config["instructions"];
    }

    // load platform resources
    if (config.count("resources") <= 0) {
        QL_FATAL(
            "'resources' section is not specified in the hardware config file");
    } else {
        resources = config["resources"];
    }

    // load platform topology
    if (config.count("topology") <= 0) {
        QL_FATAL(
            "'topology' section is not specified in the hardware config file");
    } else {
        topology = config["topology"];
    }

    // load instructions
    const Json &instructions = config["instructions"];
    static const std::regex comma_space_pattern("\\s*,\\s*");

    for (auto it = instructions.begin(); it != instructions.end(); ++it) {
        Str name = it.key();
        Json attr = *it; //.value();

        name = sanitize_instruction_name(name);
        name = std::regex_replace(name, comma_space_pattern, ",");

        // check for duplicate operations
        if (instruction_map.find(name) != instruction_map.end()) {
            QL_WOUT("instruction '" << name
                                    << "' redefined : the old definition is overwritten !");
        }

        // format in json.instructions:
        //  "^(\s)*token(\s)*[(\s)token(\s)*(,(\s)*token(\s*))*]$"
        //  so with a comma between any operands and possible spaces everywhere
        //
        // format of key and value (which is a custom_gate)'s name in instruction_map:
        //  "^(token|(token token(,token)*))$"
        //  so with a comma between any operands
        instruction_map.set(name) = load_instruction(name, attr);
        QL_DOUT("instruction '" << name << "' loaded.");
    }

    // load optional section gate_decomposition
    // Examples:
    // - Parametrized gate-decomposition: "cl_2 %0": ["rxm90 %0", "rym90 %0"]
    // - Specialized gate-decomposition:  "rx180 q0" : ["x q0"]

    if (config.count("gate_decomposition") > 0) {
        const Json &gate_decomposition = config["gate_decomposition"];
        for (auto it = gate_decomposition.begin();
             it != gate_decomposition.end(); ++it) {
            // standardize instruction name
            Str comp_ins = it.key();
            QL_DOUT("");
            QL_DOUT("Adding composite instr : " << comp_ins);
            comp_ins = sanitize_instruction_name(comp_ins);
            comp_ins = std::regex_replace(comp_ins, comma_space_pattern, ",");
            QL_DOUT("Adjusted composite instr : " << comp_ins);

            // format in json.instructions:
            //  "^(\s)*token(\s)+token(\s)*(,|\s)(\s)*token(\s*)$"
            //  so with a comma or a space between any operands and possible spaces everywhere
            //
            // format of key and value (which is a custom_gate)'s name in instruction_map:
            //  "^(token(\stoken)*))$"
            //  so with one space between any operands

            // check for duplicate operations
            if (instruction_map.find(comp_ins) != instruction_map.end()) {
                QL_WOUT("composite instruction '" << comp_ins
                                                  << "' redefined : the old definition is overwritten !");
            }

            // check that we're looking at array
            Json sub_instructions = *it;
            if (!sub_instructions.is_array()) {
                QL_FATAL(
                    "ql::hardware_configuration::load() : 'gate_decomposition' section : gate '"
                        << comp_ins << "' is malformed (not an array)");
            }

            Vec<gate *> gs;
            for (UInt i = 0; i < sub_instructions.size(); i++) {
                // standardize name of sub instruction
                Str sub_ins = sub_instructions[i];
                QL_DOUT("Adding sub instr: " << sub_ins);
                sub_ins = sanitize_instruction_name(sub_ins);
                sub_ins = std::regex_replace(sub_ins, comma_space_pattern, ",");
                if (instruction_map.find(sub_ins) != instruction_map.end()) {
                    // using existing sub ins, e.g. "x q0" or "x %0"
                    QL_DOUT("using existing sub instr : " << sub_ins);
                    gs.push_back(instruction_map.at(sub_ins));
                } else if (sub_ins.find("cond(") !=
                           Str::npos) {              // conditional gate?
                    QL_FATAL("conditional gate not supported in gate_decomposition: '" << sub_ins << "'");
                } else if (sub_ins.find("%") !=
                           Str::npos) {              // parameterized composite gate? FIXME: no syntax check
                    // adding new sub ins if not already available, e.g. "x %0"
                    QL_DOUT("adding new sub instr : " << sub_ins);
                    instruction_map.set(sub_ins) = new custom_gate(sub_ins);
                    gs.push_back(instruction_map.at(sub_ins));
                } else {
#if OPT_DECOMPOSE_WAIT_BARRIER   // allow wait/barrier, e.g. "barrier q2,q3,q4"
                    // FIXME: just save whatever we find as a *custom* gate (there is no better alternative)
                    // FIXME: also see additions (hacks) to kernel.h
                    QL_DOUT("adding new sub instr : " << sub_ins);
                    instruction_map.set(sub_ins) = new custom_gate(sub_ins);
                    gs.push_back(instruction_map.at(sub_ins));
#else
                    // for specialized custom instructions, raise error if instruction
                    // is not already available
                    QL_FATAL("custom instruction not found for '" << sub_ins << "'");
#endif
                }
            }
            instruction_map.set(comp_ins) = new composite_gate(comp_ins, gs);
        }
    }
}

} // namespace ql
