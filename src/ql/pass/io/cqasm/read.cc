/** \file
 * Defines the cQASM reader pass.
 */

#include "ql/pass/io/cqasm/read.h"

#include "detail/cqasm_reader.h"

namespace ql {
namespace pass {
namespace io {
namespace cqasm {
namespace read {

/**
 * Reads a cQASM file. Its content are added to program. The number of qubits,
 * cregs, and/or bregs allocated in the program are increased as needed (if
 * possible for the current platform). The gateset parameter should be loaded
 * from a gateset configuration file or be alternatively initialized. If empty
 * or unspecified, a default set is used, that mimics the behavior of the reader
 * before it became configurable.
 */
void from_file(
    const ir::ProgramRef &program,
    const utils::Str &cqasm_fname,
    const utils::Json &gateset
) {
    if (gateset.empty()) {
        detail::Reader(program->platform, program).file2circuit(cqasm_fname);
    } else {
        detail::Reader(program->platform, program, gateset).file2circuit(cqasm_fname);
    }
}

/**
 * Same as file(), be reads from a string instead.
 *
 * \see file()
 */
void from_string(
    const ir::ProgramRef &program,
    const utils::Str &cqasm_body,
    const utils::Json &gateset
) {
    if (gateset.empty()) {
        detail::Reader(program->platform, program).string2circuit(cqasm_body);
    } else {
        detail::Reader(program->platform, program, gateset).string2circuit(cqasm_body);
    }
}

/**
 * Dumps docs for the cQASM reader.
 */
void ReadCQasmPass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    os << line_prefix << "This pass completely discards the incoming program and replaces it" << std::endl;
    os << line_prefix << "with the program described by the given cQASM file." << std::endl;
    os << line_prefix << std::endl;
    os << line_prefix << "Because libqasm needs information about gate prototypes that does" << std::endl;
    os << line_prefix << "not currently exist in the platform configuration file, an additional" << std::endl;
    os << line_prefix << "configuration file is needed for this, specified using the" << std::endl;
    os << line_prefix << "gateset_file option. This must be a JSON file consisting of an array" << std::endl;
    os << line_prefix << "of objects, where each object has the following form." << std::endl;
    os << line_prefix << std::endl;
    os << line_prefix << "{" << std::endl;
    os << line_prefix << "    \"name\": \"<name>\",               # mandatory" << std::endl;
    os << line_prefix << "    \"params\": \"<typespec>\",         # mandatory, refer to cqasm::types::from_spec()" << std::endl;
    os << line_prefix << "    \"allow_conditionl\": <bool>,    # whether conditional gates of this type are accepted, defaults to true" << std::endl;
    os << line_prefix << "    \"allow_parallel\": <bool>,       # whether parallel gates of this type are accepted, defaults to true" << std::endl;
    os << line_prefix << "    \"allow_reused_qubits\": <bool>,  # whether reused qubit args for this type are accepted, defaults to false" << std::endl;
    os << line_prefix << "    \"ql_name\": \"<name>\",            # defaults to \"name\"" << std::endl;
    os << line_prefix << "    \"ql_qubits\": [                  # list or \"all\", defaults to the \"Q\" args" << std::endl;
    os << line_prefix << "        0,                          # hardcoded qubit index" << std::endl;
    os << line_prefix << "        \"%0\"                        # reference to argument 0, which can be a qubitref, bitref, or int" << std::endl;
    os << line_prefix << "    ]," << std::endl;
    os << line_prefix << "    \"ql_cregs\": [                   # list or \"all\", defaults to the \"I\" args" << std::endl;
    os << line_prefix << "        0,                          # hardcoded creg index" << std::endl;
    os << line_prefix << "        \"%0\"                        # reference to argument 0, which can be an int variable reference, or int for creg index" << std::endl;
    os << line_prefix << "    ]," << std::endl;
    os << line_prefix << "    \"ql_bregs\": [                   # list or \"all\", defaults to the \"B\" args" << std::endl;
    os << line_prefix << "        0,                          # hardcoded breg index" << std::endl;
    os << line_prefix << "        \"%0\"                        # reference to argument 0, which can be an int variable reference, or int for creg index" << std::endl;
    os << line_prefix << "    ]," << std::endl;
    os << line_prefix << "    \"ql_duration\": 0,               # duration; int to hardcode or \"%i\" to take from param i (must be of type int), defaults to 0" << std::endl;
    os << line_prefix << "    \"ql_angle\": 0.0,                # angle; float to hardcode or \"%i\" to take from param i (must be of type int or real), defaults to first arg of type real or 0.0" << std::endl;
    os << line_prefix << "    \"ql_angle_type\": \"<type>\",      # interpretation of angle arg; one of \"rad\" (radians), \"deg\" (degrees), or \"pow2\" (2pi/2^k radians), defaults to \"rad\"" << std::endl;
    os << line_prefix << "    \"implicit_sgmq\": <bool>,        # if multiple qubit args are present, a single-qubit gate of this type should be replicated for these qubits (instead of a single gate with many qubits)" << std::endl;
    os << line_prefix << "    \"implicit_breg\": <bool>         # the breg operand(s) that implicitly belongs to the qubit operand(s) in the gate should be added to the OpenQL operand list" << std::endl;
    os << line_prefix << "}" << std::endl;

    // TODO: describe configuration file format here.
}

/**
 * Constructs a cQASM reader.
 */
ReadCQasmPass::ReadCQasmPass(
    const utils::Ptr<const pmgr::PassFactory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::ProgramTransformation(pass_factory, instance_name, type_name) {
    options.add_str(
        "cqasm_file",
        "cQASM file to read. Mandatory."
    );
    options.add_str(
        "gateset_file",
        "JSON gateset configuration file path. Mandatory."
    );
}

/**
 * Runs the cQASM reader.
 */
utils::Int ReadCQasmPass::run(
    const ir::ProgramRef &program,
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
