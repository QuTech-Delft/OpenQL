/** \file
 * Defines the cQASM reader pass.
 */

#include "ql/pass/io/cqasm/read.h"

#include "ql/utils/json.h"
#include "detail/cqasm_reader.h"

namespace ql {
namespace pass {
namespace io {
namespace cqasm {
namespace read {

/**
 * Constructs a cQASM reader with the default cQASM gateset and conversion
 * rules. This is here for backward compatibility; new code should use a JSON
 * file for the gateset and conversion rules, or take the JSON from the platform
 * configuration file.
 */
Reader::Reader(
    const ir::compat::PlatformRef &platform,
    const ir::compat::ProgramRef &program
) : impl(platform, program) {}

/**
 * Constructs a cQASM reader with a custom gateset from a JSON structure. The
 * JSON structure should be an array of objects, where every object represents
 * a cQASM gate (overload) and the rules for converting it to OpenQL gate(s).
 * The expected structure of these objects is described in
 * GateConverter::from_json().
 */
Reader::Reader(
    const ir::compat::PlatformRef &platform,
    const ir::compat::ProgramRef &program,
    const utils::Json &gateset
) : impl(platform, program) {
    impl->load_gateset(gateset);
}

/**
 * Constructs a cQASM reader with a custom gateset from a JSON file. The
 * structure of the JSON file should be an array of objects, where every object
 * represents a cQASM gate (overload) and the rules for converting it to OpenQL
 * gate(s). The expected structure of these objects is described in
 * GateConverter::from_json().
 */
Reader::Reader(
    const ir::compat::PlatformRef &platform,
    const ir::compat::ProgramRef &program,
    const utils::Str &gateset_fname
) : impl(platform, program) {
    impl->load_gateset(utils::load_json(gateset_fname));
}

/**
 * Parses a cQASM string using the gateset selected when the Reader is
 * constructed, converts the cQASM kernels to OpenQL kernels, and adds those
 * kernels to the selected OpenQL program.
 */
void Reader::string2circuit(const utils::Str &cqasm_str) {
    impl->string2circuit(cqasm_str);
}

/**
 * Parses a cQASM file using the gateset selected when the Reader is
 * constructed, converts the cQASM kernels to OpenQL kernels, and adds those
 * kernels to the selected OpenQL program.
 */
void Reader::file2circuit(const utils::Str &cqasm_fname) {
    impl->file2circuit(cqasm_fname);
}

/**
 * Reads a cQASM file. Its content are added to program. The number of qubits,
 * cregs, and/or bregs allocated in the program are increased as needed (if
 * possible for the current platform). The gateset parameter should be loaded
 * from a gateset configuration file or be alternatively initialized. If empty
 * or unspecified, a default set is used, that mimics the behavior of the reader
 * before it became configurable.
 */
void from_file(
    const ir::compat::ProgramRef &program,
    const utils::Str &cqasm_fname,
    const utils::Json &gateset
) {
    if (gateset.empty()) {
        Reader(program->platform, program).file2circuit(cqasm_fname);
    } else {
        Reader(program->platform, program, gateset).file2circuit(cqasm_fname);
    }
}

/**
 * Same as file(), be reads from a string instead.
 *
 * \see file()
 */
void from_string(
    const ir::compat::ProgramRef &program,
    const utils::Str &cqasm_body,
    const utils::Json &gateset
) {
    if (gateset.empty()) {
        Reader(program->platform, program).string2circuit(cqasm_body);
    } else {
        Reader(program->platform, program, gateset).string2circuit(cqasm_body);
    }
}

/**
 * Dumps docs for the cQASM reader.
 */
void ReadCQasmPass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix, R"(
    This pass completely discards the incoming program and replaces it with the
    program described by the given cQASM file.

    Because libqasm (the library used by this pass to parse cQASM files;
    see http://libqasm.readthedocs.io/) needs information about gate prototypes
    that does not currently exist in the platform configuration file, an
    additional configuration file is needed for this, specified using the
    `gateset_file` option. If specified, this must be a JSON file consisting of
    an array of objects, where each object has the following form.

        {
            "name": "<name>",               # mandatory
            "params": "<typespec>",         # mandatory, refer to cqasm::types::from_spec()
            "allow_conditional": <bool>,    # whether conditional gates of this type are accepted,
                                            #   defaults to true
            "allow_parallel": <bool>,       # whether parallel gates of this type are accepted,
                                            #   defaults to true
            "allow_reused_qubits": <bool>,  # whether reused qubit args for this type are accepted,
                                            #   defaults to false
            "ql_name": "<name>",            # defaults to "name"
            "ql_qubits": [                  # list or "all", defaults to the "Q" args
                0,                          # hardcoded qubit index
                "%0"                        # reference to argument 0, which can be a qubitref, bitref,
                                            #   or int
            ],
            "ql_cregs": [                   # list or "all", defaults to the "I" args
                0,                          # hardcoded creg index
                "%0"                        # reference to argument 0, which can be an int variable
                                            #   reference, or int for creg index
            ],
            "ql_bregs": [                   # list or "all", defaults to the "B" args
                0,                          # hardcoded breg index
                "%0"                        # reference to argument 0, which can be an int variable
                                            #   reference, or int for creg index
            ],
            "ql_duration": 0,               # duration; int to hardcode or "%i" to take from param i
                                            #   (must be of type int), defaults to 0
            "ql_angle": 0.0,                # angle; float to hardcode or "%i" to take from param i
                                            #   (must be of type int or real), defaults to first arg
                                            #   of type real or 0.0
            "ql_angle_type": "<type>",      # interpretation of angle arg; one of "rad" (radians),
                                            #   "deg" (degrees), or "pow2" (2pi/2^k radians), defaults
                                            #   to "rad"
            "implicit_sgmq": <bool>,        # if multiple qubit args are present, a single-qubit gate
                                            #   of this type should be replicated for these qubits
                                            #   (instead of a single gate with many qubits)
            "implicit_breg": <bool>         # the breg operand(s) that implicitly belongs to the qubit
                                            #   operand(s) in the gate should be added to the OpenQL
                                            #   operand list
        }
)" R"(
    If not specified, the reader defaults to the logic that was hardcoded before
    it was made configurable. This corresponds to the following JSON:

        [
            {"name": "measure",     "params": "Q",      "ql_name": "measz"},
            {"name": "measure",     "params": "QB",     "ql_name": "measz"},
            {"name": "measure_x",   "params": "Q",      "ql_name": "measx"},
            {"name": "measure_x",   "params": "QB",     "ql_name": "measx"},
            {"name": "measure_y",   "params": "Q",      "ql_name": "measy"},
            {"name": "measure_y",   "params": "QB",     "ql_name": "measy"},
            {"name": "measure_z",   "params": "Q",      "ql_name": "measz"},
            {"name": "measure_z",   "params": "QB",     "ql_name": "measz"},
            {"name": "prep",        "params": "Q",      "ql_name": "prepz"},
            {"name": "prep_x",      "params": "Q",      "ql_name": "prepx"},
            {"name": "prep_y",      "params": "Q",      "ql_name": "prepy"},
            {"name": "prep_z",      "params": "Q",      "ql_name": "prepz"},
            {"name": "i",           "params": "Q"},
            {"name": "h",           "params": "Q"},
            {"name": "x",           "params": "Q"},
            {"name": "y",           "params": "Q"},
            {"name": "z",           "params": "Q"},
            {"name": "s",           "params": "Q"},
            {"name": "sdag",        "params": "Q"},
            {"name": "t",           "params": "Q"},
            {"name": "tdag",        "params": "Q"},
            {"name": "x90",         "params": "Q",      "ql_name": "rx90"},
            {"name": "mx90",        "params": "Q",      "ql_name": "xm90"},
            {"name": "y90",         "params": "Q",      "ql_name": "ry90"},
            {"name": "my90",        "params": "Q",      "ql_name": "ym90"},
            {"name": "rx",          "params": "Qr"},
            {"name": "ry",          "params": "Qr"},
            {"name": "rz",          "params": "Qr"},
            {"name": "cnot",        "params": "QQ"},
            {"name": "cz",          "params": "QQ"},
            {"name": "swap",        "params": "QQ"},
            {"name": "cr",          "params": "QQr"},
            {"name": "crk",         "params": "QQi",    "ql_angle": "%2", "ql_angle_type": "pow2"},
            {"name": "toffoli",     "params": "QQQ"},
            {"name": "measure_all", "params": "",       "ql_qubits": "all", "implicit_sgmq": true},
            {"name": "display",     "params": ""},
            {"name": "wait",        "params": ""},
            {"name": "wait",        "params": "i"}
        ]
    )");
}

/**
 * Returns a user-friendly type name for this pass.
 */
utils::Str ReadCQasmPass::get_friendly_type() const {
    return "cQASM reader";
}

/**
 * Constructs a cQASM reader.
 */
ReadCQasmPass::ReadCQasmPass(
    const utils::Ptr<const pmgr::Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::ProgramTransformation(pass_factory, instance_name, type_name) {
    options.add_str(
        "cqasm_file",
        "cQASM file to read. Mandatory."
    );
    options.add_str(
        "gateset_file",
        "Optional JSON gateset configuration file path, if the default behavior "
        "is insufficient."
    );
}

/**
 * Runs the cQASM reader.
 */
utils::Int ReadCQasmPass::run(
    const ir::compat::ProgramRef &program,
    const pmgr::pass_types::Context &context
) const {

    // Empty the program completely.
    // TODO: there should be a common function to do this.
    program->kernels.reset();
    program->qubit_count = 0;
    program->creg_count = 0;
    program->breg_count = 0;

    // Read cQASM file.
    utils::Json gateset;
    if (options["gateset_file"].is_set()) {
        gateset = utils::load_json(options["gateset_file"].as_str());
    }
    from_file(
        program,
        options["cqasm_file"].as_str(),
        gateset
    );

    return 0;
}

} // namespace reader
} // namespace cqasm
} // namespace io
} // namespace pass
} // namespace ql
