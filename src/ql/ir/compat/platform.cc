/** \file
 * Platform header for target-specific compilation.
 */

#include "ql/ir/compat/platform.h"

#include <regex>
#include "ql/config.h"
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

      The `"eqasm_compiler"` key can take any of the following types of
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

      NOTE: older versions of OpenQL recognized and required the existence of
      many more keys, such as `"matrix"` and `"latency"`. All passes relying on
      this information have since been cleaned out as they were no longer in
      use, and all requirements on the existence of these keys have likewise
      been lifted.

)" R"(
      * `"cqasm_name"` key *

        Specifies an alternative name for the instruction when it is printed as
        cQASM or when read from cQASM. This must be a valid identifier. If not
        specified, it defaults to the normal instruction name.

      * `"prototype"` key *

        Specifies the amount and type of operands that the instruction expects.
        If specified, it must be an array of strings. Each of these strings
        represents an operand, and must be set to the name of its expected type,
        optionally prefixed with the access mode, separated by a colon. By
        default, the available types are:

         - `qubit` for qubits;
         - `bit` for classical bits;
         - `int` for 32-bit signed integers; and
         - `real` for floating-point numbers.

        The available access modes are:

         - `B` for barriers (DDG write, liveness ignore);
         - `W` for write access or qubit state preparation (DDG write, liveness
           kill);
         - `U` for read+write/update access or regular non-commuting qubit
           usage (DDG write, liveness use);
         - `R` for read-only access (DDG read, liveness use);
         - `L` for operands that must be literals;
         - `X` for qubit access that behaves like an X rotation (DDG X,
           liveness use);
         - `Y` for qubit access that behaves like an Y rotation (DDG Y,
           liveness use);
         - `Z` for qubit access that behaves like an Z rotation (DDG Z,
           liveness use);
         - `M` for a measurement of the qubit for which the result is written
           to its implicitly associated bit register (DDG W for the qubit and
           its bit, liveness use for the qubit, and liveness kill for the bit);
           and
         - `I` for operands that should be ignored (DDG ignore, liveness
           ignore).
)" R"(
        If the access mode is not specified for an operand, `U` is assumed, as
        it is the most pessimistic mode available. If no prototype is specified
        at all, it will be inferred based on the instruction name for backward
        compatibility, using the following rules (first regex-matching rule
        applies):

         - `move_init|prep(_?[xyz])?` -> `["W:qubit"]`
         - `h|i` -> `["U:qubit"]`
         - `rx` -> `["X:qubit", "L:real"]`
         - `(m|mr|r)?xm?[0-9]*` -> `["X:qubit"]`
         - `ry` -> `["Y:qubit", "L:real"]`
         - `(m|mr|r)?ym?[0-9]*` -> `["Y:qubit"]`
         - `rz` -> `["Z:qubit", "L:real"]`
         - `crz?` -> `["Z:qubit", "Z:qubit", "L:real"]`
         - `crk` -> `["Z:qubit", "Z:qubit", "L:int"]`
        - `[st](dag)?|(m|mr|r)?zm?[0-9]*` -> `["Z:qubit"]`
         - `meas(ure)?(_?[xyz])?(_keep)?` -> `["M:qubit"]` and
           `["U:qubit", "W:bit"]`
         - `(teleport)?(move|swap)` -> `["U:qubit", "U:qubit"]`
         - `cnot|cx` -> `["Z:qubit", "X:qubit"]`
         - `cphase|cz` -> `["Z:qubit", "Z:qubit"]`
         - `cz_park` -> `["Z:qubit", "Z:qubit", "I:qubit"]`
         - `toffoli` -> `["Z:qubit", "Z:qubit", "X:qubit"]`
         - no operands otherwise.

        Furthermore, when gates are added via the API or old IR that don't
        match an existing instruction due to prototype mismatch, and the
        prototype was inferred per the above rules, a clone is made of the
        instruction type with the prototype inferred by means of the actual
        operands, using the `U` access mode for reference operands and `R` for
        anything else. When a default gate is encountered in the old IR and
        needs to be converted to the new IR, the entire instruction type is
        inferred from the default gate. From now on, however, it is strongly
        recommended to explicitly specify prototypes and not rely on this
        inference logic.

        NOTE: it is possible to define multiple overloads for an instruction
        with the same name. Passes using the new IR will be able to distinguish
        between these overloads based on the types and writability of the
        operands, but be aware that any legacy pass will use one of the gate
        definitions at random (so differing duration or other attributes won't
        work right). The JSON syntax for this is rather awkward, since object
        keys must be unique; the best thing to do is to just append spaces for
        the key, since these spaces are cleaned up when the instruction type is
        parsed.
)" R"(
      * `"barrier"` key *

        An optional boolean that specifies that an instruction is to behave as
        a complete barrier, preventing it from being commuted with any other
        instruction during scheduling, and preventing optimizations on it. If
        not specified, the flag defaults to false.

      * `"duration"` or `"duration_cycles"` key *

        These keys specify the duration of the instruction in nanoseconds or
        cycles. It is illegal to specify both of them for a single instruction.
        If neither is specified, the duration defaults to a single cycle.

        NOTE: OpenQL currently only supports durations that are an integer
        number of nanoseconds, so any fractions will be rounded up to the
        nearest nanosecond. Furthermore, in almost all contexts, the duration of
        an instruction will be rounded up to the nearest integer cycle count.
)" R"(
      * `"decomposition"` key *

        May be used to specify one or more decomposition rules for the
        instruction type. Unlike the rules in the `"gate_decomposition"`
        section, these rules are normally only applied by an explicit
        decomposition pass, of which the predicate matches the name and/or
        additional JSON data for the rule. The value for the `"decomposition"`
        key can take a number of shapes:

         - a single string: treated as a single, anonymous decomposition rule
           of which the decomposition is defined by the string parsed as a
           single-line cQASM 1.2 block;
         - an array of strings: as above, but each string represents a new line
           in the cQASM block;
         - a single object: treated as a single decomposition specification; or
         - an array of objects: treated as multiple decomposition
           specifications.

        A decomposition specification object must have an `"into"` key that
        specifies the decomposition, which must be a single string or an array
        of strings as above. In addition, it may be given a name via the
        `"name"` key, or any number of other keys for passes to use to
        determine whether to apply a decomposition rule, or which decomposition
        rule to apply if multiple options are defined.
)" R"asdf(
        The cQASM 1.2 block must satisfy the following rules:

         - the version header must not be specified (it is added
           automatically);
         - subcircuits and goto statements are not supported;
         - the operands of the to-be-decomposed gate can be accessed using the
           `op(int) -> ...` function, where the integer specifies the operand
           index (which must constant-propagate to an integer literal); and
         - the duration of the decomposed block may not be longer than the
           duration of the to-be-decomposed instruction (either make the
           instruction duration long enough, or define the decomposition as a
           single bundle and (re)schedule after applying the decomposition).

        Other than that, the cQASM code is interpreted using the default
        cQASM 1.2 rules. Note that this also means that the cQASM name of an
        instruction must be used if said instructions has differing OpenQL and
        cQASM names. Refer to the documentation of the cQASM 1.2 reader pass
        (`io.cqasm.Read`) for more information.

        As an example, a CNOT gate with its usual decomposition might be
        specified as follows.

            {
                "prototype": ["Z:qubit", "X:qubit"],
                "duration_cycles": 4,
                "decomposition": {
                    "name": "to_cz",
                    "into": [
                        "ym90 op(1)",
                        "cz op(0), op(1)",
                        "skip 1",
                        "y90 op(1)"
                    ]
                }
            }

        Note that application of this decomposition rule would retain program
        validity with respect to schedule and data dependencies (if it was valid
        before application) for platforms where single-qubit rotations are
        single-cycle and the CZ gate is two-cycle, because the CNOT gate is
        defined to take four cycles, and the schedule of the decomposition is
        valid.

        NOTE: the `"decomposition"` key is only supported when the instruction
        prototype is explicitly specified using the `"prototype"` key.
        Without a fixed prototype, type checking the `op()` function would be
        impossible.

      * `"qubits"` key *

        This *must* map to a single qubit index or a list of qubit indices that
        corresponds to the qubits in the specialization. For generalized
        instructions, the list must either be empty or unspecified. The qubit
        indices can be specified as either a string of the form `"q<index>"` or
        an integer with just the index.

        NOTE: this field is obviously redundant. As such, it may be removed in
        the future, from which point onward it will be ignored.
)asdf" R"(
    * `"gate_decomposition"` section *

      This section specifies legacy decomposition rules for gates/instructions.
      They are applied in the following cases:

       - when a gate matching a decomposition rule is added to a kernel using
         the API;
       - immediately before a legacy pass (i.e. one that still operates on the
         old IR) is run; and
       - when a decomposition pass matching rules named "legacy" is run.

      Rules in this section support only a subset of what the new decomposition
      system supports. For example, scheduling information cannot be
      represented, the to-be-decomposed instruction can only have qubit
      operands, and the to-be-decomposed instruction can't exist in the old IR
      without being decomposed (preventing operations on it before
      decomposition). For these reasons, this decomposition system is
      deprecated, and only still exists for backward compatibility.

      If the section is specified, it must be an object, where each key
      represents the name of the to-be-decomposed gate, along with capture
      groups for the qubit operands. The keys must map to arrays of strings,
      wherein each string represents a gate in the decomposition.

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
      other decomposed gate definitions, are not supported. Behavior for this
      is undefined; the nested rules may or may not end up being expanded, and
      if they're not, internal compiler errors may result.

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
    QL_DOUT("compatibility load of configuration from json");
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

    QL_DOUT("compatibility platform load has determined architecture etc.,  now go for hardware_settings");

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

    QL_DOUT("compatibility platform load go for instruction_settings, resources and topology");

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

    QL_DOUT("compatibility platform load instructions");

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

    QL_DOUT("compatibility platform load gate_decomposition");

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

            QL_DOUT(" took sub_instructions from json for composite instr : " << comp_ins);
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
#ifdef OPT_DECOMPOSE_WAIT_BARRIER   // allow wait/barrier, e.g. "barrier q2,q3,q4"
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
            QL_DOUT(" added sub_instructions to 'gate references' for composite instr, replace composite in instruction_map : " << comp_ins);
            instruction_map.set(comp_ins).emplace<gate_types::Composite>(comp_ins, gs);
        }
    }
    QL_DOUT("compatibility load of configuration from json [DONE]");
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
