/** \file
 * Platform header for target-specific compilation.
 */

#include "ql/plat/platform.h"

#include <regex>
#include "ql/utils/filesystem.h"
#include "ql/arch/factory.h"

namespace ql {
namespace plat {

/**
 * Dumps the documentation for the platform configuration file structure.
 */
void Platform::dump_docs(std::ostream &os, const utils::Str &line_prefix) {
    utils::dump_str(os, line_prefix, R"(
    TODO
    )");
}

static const std::regex trim_pattern("^(\\s)+|(\\s)+$");
static const std::regex multiple_space_pattern("(\\s)+");

/**
 * Sanitizes the name of an instruction by converting to lower case and removing
 * the unnecessary spaces.
 */
static utils::Str sanitize_instruction_name(utils::Str name) {
    name = utils::to_lower(name);
    name = std::regex_replace(name, trim_pattern, "");
    name = std::regex_replace(name, multiple_space_pattern, " ");
    return name;
}

static CustomGateRef load_instruction(
    const utils::Str &name,
    utils::Json &instr,
    utils::UInt num_qubits,
    utils::UInt cycle_time
) {
    auto g = CustomGateRef::make<ir::gate_types::Custom>(name);
    // skip alias fo now
    if (instr.count("alias") > 0) {
        // todo : look for the target aliased gate
        //        copy it with the new name
        QL_WOUT("alias '" << name << "' detected but ignored (not supported yet : please define your instruction).");
        return g;
    }
    try {
        g->load(instr, num_qubits, cycle_time);
    } catch (utils::Exception &e) {
        QL_EOUT("error while loading instruction '" << name << "' : " << e.what());
        throw;
        // ql::exception("[x] error : hardware_configuration::load_instruction() : error while loading instruction '" + name + "' : " + e.what(),false);
    }
    // g->print_info();
    return g;
}

/**
 * Loads the platform members from the given JSON data and optional
 * auxiliary compiler configuration file.
 */
void Platform::load(
    utils::Json &platform_config,
    const utils::Str &compiler_config
) {
    arch::Factory arch_factory = {};

    // Load compiler configuration.
    if (!compiler_config.empty()) {

        // Override file specified for compiler settings. Load it instead of
        // using "eqasm_compiler".
        compiler_settings = utils::load_json(compiler_config);

    } else if (platform_config.count("eqasm_compiler") <= 0) {

        // Let's be lenient. We have sane defaults regardless of what's
        // specified here.
        architecture = arch_factory.build_from_namespace("none");
        compiler_settings = "\"\""_json;

    } else if (platform_config["eqasm_compiler"].type() == utils::Json::value_t::object) {

        // Inline configuration object.
        compiler_settings = platform_config["eqasm_compiler"];

    } else if (platform_config["eqasm_compiler"].type() == utils::Json::value_t::string) {

        // Figure out what kind of string this is.
        utils::Str s = platform_config["eqasm_compiler"].get<utils::Str>();
        architecture = arch_factory.build_from_eqasm_compiler(s);
        if (!architecture.has_value()) {

            // String is unrecognized, but it could be a filename to a JSON
            // configuration file (try relative to the platform JSON file or
            // fall back to relative to the working directory).
            auto fname = utils::path_relative_to(utils::dir_name(platform_config), s);
            if (utils::path_exists(fname)) {
                compiler_settings = utils::load_json(fname);
            } else if (utils::path_exists(s)) {
                compiler_settings = utils::load_json(s);
            } else {

                // Hmmm. Not sure what this is.
                if (utils::ends_with(".json", s)) {
                    QL_FATAL("'eqasm_compiler' looks like a filename, but the file was not found");
                } else {
                    QL_FATAL("'eqasm_compiler' doesn't look like anything supported at this time");
                }

            }

        }

    } else {
        QL_FATAL("'eqasm_compiler' must be a string or an object");
    }

    // If eqasm_compiler was either an inline compiler configuration, a
    // reference to a configuration file, or a configuration file override was
    // specified, detect the architecture from it instead.
    if (!architecture.has_value()) {
        auto it = compiler_settings.find("architecture");
        if (it == compiler_settings.end() || it->type() != utils::Json::value_t::string) {
               architecture = arch_factory.build_from_namespace("none");
        }
        auto s = it->get<utils::Str>();
        architecture = arch_factory.build_from_namespace(s);
        if (!architecture.has_value()) {
            throw utils::Exception("unknown architecture name " + s);
        }
    }

    // If this fails, the above logic failed to make an architecture. It should
    // always at least generate the default no-op architecture!
    QL_ASSERT(architecture.has_value());

    // Do architecture-specific preprocessing before anything else.
    architecture->preprocess_platform(platform_config);

    // load hardware_settings
    if (platform_config.count("hardware_settings") <= 0) {
        QL_FATAL("'hardware_settings' section is not specified in the hardware config file");
    } else {
        hardware_settings = platform_config["hardware_settings"];
        if (hardware_settings.count("qubit_number") <= 0) {
            QL_FATAL("qubit number of the platform is not specified in the configuration file !");
        } else {
            qubit_count = hardware_settings["qubit_number"];
        }
        if (hardware_settings.count("creg_number") <= 0) {
            creg_count = 0;
            compat_implicit_creg_count = true;
        } else {
            creg_count = hardware_settings["creg_number"];
            compat_implicit_creg_count = false;
        }
        if (hardware_settings.count("breg_number") <= 0) {
            breg_count = 0;
            compat_implicit_breg_count = true;
        } else {
            breg_count = hardware_settings["breg_number"];
            compat_implicit_breg_count = false;
        }
        if (hardware_settings.count("cycle_time") <= 0) {
            QL_WOUT("hardware_settings.cycle_time is not specified in the configuration file; assuming 1 \"ns\" for ease of calculation");
            cycle_time = 1;
        } else {
            cycle_time = hardware_settings["cycle_time"];
        }
    }

    // load instruction_settings
    if (platform_config.count("instructions") <= 0) {
        QL_FATAL("'instructions' section is not specified in the hardware config file");
    } else {
        instruction_settings = platform_config["instructions"];
    }

    // load platform resources
    if (platform_config.count("resources") <= 0) {
        QL_WOUT("'resources' section is not specified in the hardware config file; assuming that there are none");
        resources = "{}"_json;
    } else {
        resources = platform_config["resources"];
    }

    // load platform topology
    if (platform_config.count("topology") <= 0) {
        QL_WOUT("'topology' section is not specified in the hardware config file; a fully-connected topology will be generated");
        topology.emplace(qubit_count, "{}"_json);
    } else {
        topology.emplace(qubit_count, platform_config["topology"]);
    }

    // load instructions
    const utils::Json &instructions = platform_config["instructions"];
    static const std::regex comma_space_pattern("\\s*,\\s*");

    for (auto it = instructions.begin(); it != instructions.end(); ++it) {
        utils::Str instr_name = it.key();
        utils::Json attr = *it; //.value();

        instr_name = sanitize_instruction_name(instr_name);
        instr_name = std::regex_replace(instr_name, comma_space_pattern, ",");

        // check for duplicate operations
        if (instruction_map.find(instr_name) != instruction_map.end()) {
            QL_WOUT(
                "instruction '" << instr_name << "' redefined: "
                "the old definition will be overwritten!"
            );
        }

        // format in json.instructions:
        //  "^(\s)*token(\s)*[(\s)token(\s)*(,(\s)*token(\s*))*]$"
        //  so with a comma between any operands and possible spaces everywhere
        //
        // format of key and value (which is a custom_gate)'s name in instruction_map:
        //  "^(token|(token token(,token)*))$"
        //  so with a comma between any operands
        instruction_map.set(instr_name) = load_instruction(instr_name, attr, qubit_count, cycle_time);
        QL_DOUT("instruction '" << instr_name << "' loaded.");
    }

    // load optional section gate_decomposition
    // Examples:
    // - Parametrized gate-decomposition: "cl_2 %0": ["rxm90 %0", "rym90 %0"]
    // - Specialized gate-decomposition:  "rx180 q0" : ["x q0"]

    if (platform_config.count("gate_decomposition") > 0) {
        const utils::Json &gate_decomposition = platform_config["gate_decomposition"];
        for (auto it = gate_decomposition.begin();
             it != gate_decomposition.end(); ++it) {
            // standardize instruction name
            utils::Str comp_ins = it.key();
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
                QL_WOUT(
                    "composite instruction '" << comp_ins << "' redefined: "
                    "the old definition will be overwritten!"
                );
            }

            // check that we're looking at array
            utils::Json sub_instructions = *it;
            if (!sub_instructions.is_array()) {
                QL_FATAL(
                    "gate decomposition rule for '" << comp_ins << "' is "
                    "malformed (not an array)"
                );
            }

            ir::GateRefs gs;
            for (utils::UInt i = 0; i < sub_instructions.size(); i++) {
                // standardize name of sub instruction
                utils::Str sub_ins = sub_instructions[i];
                QL_DOUT("Adding sub instr: " << sub_ins);
                sub_ins = sanitize_instruction_name(sub_ins);
                sub_ins = std::regex_replace(sub_ins, comma_space_pattern, ",");
                if (instruction_map.find(sub_ins) != instruction_map.end()) {
                    // using existing sub ins, e.g. "x q0" or "x %0"
                    QL_DOUT("using existing sub instr : " << sub_ins);
                    gs.add(instruction_map.at(sub_ins));
                } else if (sub_ins.find("cond(") !=
                           utils::Str::npos) {              // conditional gate?
                    QL_FATAL("conditional gate not supported in gate_decomposition: '" << sub_ins << "'");
                } else if (sub_ins.find("%") !=
                           utils::Str::npos) {              // parameterized composite gate? FIXME: no syntax check
                    // adding new sub ins if not already available, e.g. "x %0"
                    QL_DOUT("adding new sub instr : " << sub_ins);
                    instruction_map.set(sub_ins).emplace<ir::gate_types::Custom>(sub_ins);
                    gs.add(instruction_map.at(sub_ins));
                } else {
#if OPT_DECOMPOSE_WAIT_BARRIER   // allow wait/barrier, e.g. "barrier q2,q3,q4"
                    // FIXME: just save whatever we find as a *custom* gate (there is no better alternative)
                    // FIXME: also see additions (hacks) to kernel.h
                    QL_DOUT("adding new sub instr : " << sub_ins);
                    instruction_map.set(sub_ins).emplace<ir::gate_types::Custom>(sub_ins);
                    gs.add(instruction_map.at(sub_ins));
#else
                    // for specialized custom instructions, raise error if instruction
                    // is not already available
                    QL_FATAL("custom instruction not found for '" << sub_ins << "'");
#endif
                }
            }
            instruction_map.set(comp_ins).emplace<ir::gate_types::Composite>(comp_ins, gs);
        }
    }

}

/**
 * Constructs a platform from the given configuration filename.
 */
Platform::Platform(
    const utils::Str &name,
    const utils::Str &platform_config,
    const utils::Str &compiler_config
) : name(name) {

    arch::Factory arch_factory = {};

    // If the configuration filename itself is a recognized architecture name,
    // query the default configuration for that architecture. Otherwise
    // interpret it as a filename, which it's historically always been.
    utils::Json config;
    architecture = arch_factory.build_from_namespace(platform_config);
    if (architecture.has_value()) {
        std::istringstream is{architecture->get_default_platform()};
        config = utils::parse_json(is);
    } else {
        try {
            config = utils::load_json(platform_config);
        } catch (utils::Json::exception &e) {
            QL_FATAL(
                "failed to load the hardware config file : malformed json file: \n\t"
                    << utils::Str(e.what()));
        }
    }

    load(config, compiler_config);
}

/**
 * Constructs a platform from the given configuration *data*. The dummy
 * argument only serves to differentiate from the previous constructor.
 */
Platform::Platform(
    const utils::Str &name,
    const utils::Json &platform_config,
    const utils::Str &compiler_config
) : name(name) {
    utils::Json platform_config_mut = platform_config;
    load(platform_config_mut, compiler_config);
}

/**
 * Dumps some basic info about the platform to the given stream.
 */
void Platform::dump_info(std::ostream &os, utils::Str line_prefix) const {
    os << line_prefix << "[+] platform name      : " << name << "\n";
    os << line_prefix << "[+] qubit number       : " << qubit_count << "\n";
    os << line_prefix << "[+] creg number        : " << creg_count << "\n";
    os << line_prefix << "[+] breg number        : " << breg_count << "\n";
    os << line_prefix << "[+] architecture       : " << architecture->get_friendly_name() << "\n";
    os << line_prefix << "[+] supported instructions:" << "\n";
    for (const auto &i : instruction_map) {
        os << line_prefix << "  |-- " << i.first << "\n";
    }
}

/**
 * Returns the JSON data for a custom gate, throwing a semi-useful
 * exception if the instruction is not found.
 */
const utils::Json &Platform::find_instruction(const utils::Str &iname) const {
    // search the JSON defined instructions, to prevent JSON exception if key does not exist
    if (!QL_JSON_EXISTS(instruction_settings, iname)) {
        QL_FATAL("JSON file: instruction not found: '" << iname << "'");
    }
    return instruction_settings[iname];
}

/**
 * Returns the JSON data for all instructions as a JSON map.
 *
 * FIXME: something like this is needed, but the structure should already
 *  have been parsed rather than be in JSON form.
 */
const utils::Json &Platform::get_instructions() const {
    return instruction_settings;
}

/**
 * Converts the given time in nanoseconds to cycles.
 */
utils::UInt Platform::time_to_cycles(utils::Real time_ns) const {
    return ceil(time_ns / cycle_time);
}

} // namespace plat
} // namespace ql
