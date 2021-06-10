/** \file
 * Platform header for target-specific compilation.
 */

#include "ql/ir/compat/platform.h"

#include <regex>
#include "ql/utils/filesystem.h"
#include "ql/rmgr/manager.h"
#include "ql/arch/factory.h"

namespace ql {
namespace ir {
namespace compat {

/**
 * Dumps the documentation for the platform configuration file structure.
 */
void Platform::dump_docs(std::ostream &os, const utils::Str &line_prefix) {
    utils::dump_str(os, line_prefix, R"(
    The platform configuration JSON file (or JSON data, as it's not necessarily
    always in file form) represents a complete description of what the target
    platform looks like, and optionally how to compile for it. At the top level,
    the structure is a JSON object, with the following keys recognized by
    OpenQL's platform-agnostic logic, customarily written in the following
    order.

     - `"eqasm_compiler"`: an optional description of how to compile for this
       platform.
     - `"hardware_settings"`: contains basic descriptors for the hardware, such
       as qubit count and cycle time.
     - `"topology"`: optionally provides a more in-depth description of how the
       qubits are organized.
     - `"resources"`: optionally provides information about scheduling
       constraints, for example due to a number of qubits sharing a single
       waveform generator.
     - `"instructions"`: lists the instruction set supported by the platform.
     - `"gate_decomposition"`: optionally lists a set of decomposition rules
       that are immediately applied when a gate with a particular name is added.

    NOTE: the plan is to move away from on-the-fly gate decomposition, and
    instead make a gate decomposition pass. The exact design for this has not
    been made yet, but it's possible that the gate decomposition section will
    change in minor ways in the future, or will be deprecated in favor of an
    entirely new configuration file section.

    Depending on the architecture being compiled for, as specified through the
    `"eqasm_compiler"` key or via the compiler configuration file override
    during platform construction, additional sections or keys may be optional or
    required, or entire sections may even be generated. Refer to the
    architecture documentation for details to this end. The `"none"`
    architecture by definition does not do any of this, and can thus always be
    used as an override of sorts for this behavior; the only thing that the
    architecture variant specification does is provide better defaults for the
    platform and compiler configuration, so everything can always be specified
    using the `"none"` architecture if need be. The remainder of this page thus
    describes the "universal" structure as used by `"none"`, while the
    architecture documentation may make achitecture-specific addenda.

    In addition, passes are allowed to make use of additional structures in the
    configuration file. This implies that the common OpenQL code will *not*
    check for or warn you about unrecognized keys: it assumes that these will
    be read by something it is not aware of.
)" R"(
    * `"eqasm_compiler"` section *

      The `"eqasm_compiler"` key can take assume of the following types of
      values.

       - No value/unspecified: the defaults for the "none" architecture will
         implicitly be used.
       - A string matching one of the available architectures or architecture
         variants, for example `"cc"` or `"cc_light.s7"`: the defaults for that
         architecture (variant) will be used. Legacy values, such as
         `"eqasm_compiler_cc"` or `"qx"` may also be used. Refer to the
         architecture documentation for a full and up-to-date list of recognized
         values.
       - A filename: the specified file will be interpreted as a compiler
         configuration file, fully specifying what the compiler looks like.
       - A JSON object: the contents of the object will be interpreted as a
         compiler configuration file, again fully specifying what the compiler
         looks like, but without the extra file indirection.

      This key can also be completely overridden by explicitly specifying a
      compiler configuration file during platform construction, thus allowing
      the platform and compiler configuration files to be completely disjoint,
      if this is preferred for the intended application.
)" R"(
    * `"hardware_settings"` section *

      This must map to an object containing the basic parameters that describe
      the platform. OpenQL's common code recognizes the following.

       - `"qubit_number"`: must map to an integer specifying the total number
         of qubits in the platform. Qubit indices start at zero, so all indices
         must be in the range 0..N-1, where N is this value.
       - `"creg_number"`: optionally specifies the number of 32-bit integer
         classical registers available in the platform. If not specified, the
         value will be inferred from the constructor of `ql.Program`.
       - `"breg_number"`: optionally specifies the number of single-bit
         classical registers available in the platform, used for receiving
         measurement results and predicates. If not specified, the value will be
         inferred from the constructor of `ql.Program`.
       - `"cycle_time"`: optionally specifies the cycle time used by the
         platform in nanoseconds. Currently this must be an integer value. If
         not specified, 1 will be used as a default, thus equating the
         nanosecond values to cycle values.

    * `"topology"` section *

    )");
    com::Topology::dump_docs(os, line_prefix + "  ");
    os << line_prefix << std::endl;
    utils::dump_str(os, line_prefix, R"(
    * `"resources"` section *

    )");
    rmgr::Manager::dump_docs(os, line_prefix + "  ");
    os << line_prefix << std::endl;
    utils::dump_str(os, line_prefix, R"(
    * `"instructions"` section *

      This section specifies the instruction set of the architecture. It must
      be an object, where each key represents the name of a gate, and the value
      is again an object, containing any semantical information needed to
      describe the instruction.

      NOTE: OpenQL currently derives much of the semantics from the gate name.
      For example, the cQASM writer determines whether it should emit a gate
      angle parameter based on whether the name of the gate equals a set of
      gate names that would logically have an angle. This is behavior is very
      much legacy, and is to be replaced with checking for keys in the
      instruction definition (though of course using appropriate defaults for
      backward compatibility).

      OpenQL supports two classes of instructions: *generalized gates* and
      *specialized gates*. For generalized gates, the gate name (i.e. the JSON
      object key) must be a single identifier matching `[a-zA-Z_][a-zA-Z0-9_]*`.
      This means that the gate can be applied to any set of operands.
      Specialized gates, on the other hand, have a fixed set of qubit operands.
      Their JSON key must be of the form `<name> <qubits>`, where `<name>` is
      as above, and `<qubits>` is a comma-separated list (without spaces) of
      qubit indices, each of the form `q<index>`. For example, a correct name
      for a specialized two-qubit gate would be `cz q0,q1`. Specialized gates
      allow different semantical parameters to be specified for each possible
      set of qubit operands: for example, the duration for a particular gate on
      a particular architecture may depend on the operands.

      NOTE: if you're using the mapper, and thus the input to your program is
      unmapped, OpenQL will also use the gate specializations when the gates
      still operate on virtual qubits. Therefore, you *must* specify a
      specialization for all possible qubit operand combinations of a particular
      gate, even if the gate does not exist for a particular combination due to
      for example connectivity constraints. Alternatively, you may specify a
      fallback using a generalized gate, as OpenQL will favor specialized gates
      when they exist.

      Within the instruction definition object, OpenQL's
      architecture/pass-agnostic logic currently only recognizes the following
      keys.

       - `"duration"` or `"duration_cycles"`: specifies the duration of the
         instruction in nanoseconds or cycles. OpenQL currently only supports
         durations that are an integer number of nanoseconds, so any fractions
         will be rounded up to the nearest nanosecond. Furthermore, in almost
         all contexts, the duration of an instruction will be rounded up to the
         nearest integer cycle count.
       - `"qubits"`: this *must* map to a single qubit index or a list of qubit
         indices that corresponds to the qubits in the specialization. For
         generalized instructions, the list must either be empty or unspecified.
         This field will be removed in the future as it is redundant, from which
         point onward it will be ignored. The qubit indices themselves can be
         specified as either a string of the form `"q<index>"` or an integer
         with just the index.

      NOTE: older versions of OpenQL recognized and required the existence of
      many more keys, such as `"matrix"` and `"latency"`. All passes relying on
      this information have since been cleaned out as they were no longer in
      use, and all requirements on the existence of these keys have likewise
      been lifted.
)" R"(
    * `"gate_decomposition"` section *

      This section specifies the decomposition rules for gates/instructions that
      are applied immediately when a gate is constructed. If specified, it must
      be an object, where each key represents the name of the gate, along with
      capture groups for the qubit operands. The keys must map to arrays of
      strings, wherein each string represents a gate in the decomposition.

      Examples of two decompositions are shown below. `%0` and `%1` refer to the
      first argument and the second argument. This means according to the
      decomposition on line 2, `rx180 %0` will allow us to decompose `rx180 q0`
      to `x q0`. Similarly, the decomposition on line 3 will allow us to
      decompose `cnot q2, q0` to three instructions, namely: `ry90 q0`,
      `cz q2, q0`, and `ry90 q0`.

      ```
      "gate_decomposition": {
          "rx180 %0" : ["x %0"],
          "cnot %0,%1" : ["ry90 %1","cz %0,%1","ry90 %1"]
      }
      ```

      These decompositions are simple macros (in-place substitutions) which
      allow programmer to manually specify a decomposition. These take place at
      the time of creation of a gate in a kernel. This means the scheduler
      will schedule decomposed instructions.

      NOTE: decomposition rules may only refer to custom gates that have already
      been defined in the instruction set.

      NOTE: recursive decomposition rules, i.e. decompositions that make use of
      other decomposed gate definitions, are not supported.

      NOTE: these decomposition rules are intended to be replaced by a more
      powerful system in the future.
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

static GateRef load_instruction(
    const utils::Str &name,
    utils::Json &instr,
    utils::UInt num_qubits,
    utils::UInt cycle_time
) {
    auto g = GateRef::make<gate_types::Custom>(name);
    // skip alias fo now
    if (instr.count("alias") > 0) {
        // todo : look for the target aliased gate
        //        copy it with the new name
        QL_WOUT("alias '" << name << "' detected but ignored (not supported yet : please define your instruction).");
        return g;
    }
    try {
        g.as<gate_types::Custom>()->load(instr, num_qubits, cycle_time);
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
    utils::Json &platform_cfg,
    const utils::Str &platform_config_fname,
    const utils::Str &compiler_config
) {
    platform_config = platform_cfg;
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
            auto fname = s;
            if (!platform_config_fname.empty()) {
                fname = utils::path_relative_to(utils::dir_name(platform_config_fname), s);
            }
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
        } else {
            auto s = it->get<utils::Str>();
            architecture = arch_factory.build_from_namespace(s);
            if (!architecture.has_value()) {
                throw utils::Exception("unknown architecture name " + s);
            }
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

            GateRefs gs;
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
                    instruction_map.set(sub_ins).emplace<gate_types::Custom>(sub_ins);
                    gs.add(instruction_map.at(sub_ins));
                } else {
#if OPT_DECOMPOSE_WAIT_BARRIER   // allow wait/barrier, e.g. "barrier q2,q3,q4"
                    // FIXME: just save whatever we find as a *custom* gate (there is no better alternative)
                    // FIXME: also see additions (hacks) to kernel.h
                    QL_DOUT("adding new sub instr : " << sub_ins);
                    instruction_map.set(sub_ins).emplace<gate_types::Custom>(sub_ins);
                    gs.add(instruction_map.at(sub_ins));
#else
                    // for specialized custom instructions, raise error if instruction
                    // is not already available
                    QL_FATAL("custom instruction not found for '" << sub_ins << "'");
#endif
                }
            }
            instruction_map.set(comp_ins).emplace<gate_types::Composite>(comp_ins, gs);
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
    utils::Str platform_config_fname;
    architecture = arch_factory.build_from_namespace(platform_config);
    if (architecture.has_value()) {
        std::istringstream is{architecture->get_default_platform()};
        config = utils::parse_json(is);
        platform_config_fname = "";
    } else {
        try {
            config = utils::load_json(platform_config);
        } catch (utils::Json::exception &e) {
            QL_FATAL(
                "failed to load the hardware config file : malformed json file: \n\t"
                    << utils::Str(e.what()));
        }
        platform_config_fname = platform_config;
    }

    load(config, platform_config_fname, compiler_config);
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
 * Constructs a platform from the given configuration filename.
 */
PlatformRef Platform::build(
    const utils::Str &name,
    const utils::Str &platform_config,
    const utils::Str &compiler_config
) {
    PlatformRef ref;
    ref.set(std::shared_ptr<Platform>(new Platform(name, platform_config, compiler_config)));
    ref->architecture->post_process_platform(ref);
    return ref;
}

/**
 * Constructs a platform from the given configuration *data*.
 */
PlatformRef Platform::build(
    const utils::Str &name,
    const utils::Json &platform_config,
    const utils::Str &compiler_config
) {
    PlatformRef ref;
    ref.set(std::shared_ptr<Platform>(new Platform(name, platform_config, compiler_config)));
    ref->architecture->post_process_platform(ref);
    return ref;
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

} // namespace compat
} // namespace ir
} // namespace ql
