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
    utils::dump_str(os, line_prefix, R"(
    This pass completely discards the incoming program and replaces it with the
    program described by the given cQASM file.

    Because libqasm needs information about gate prototypes that does not
    currently exist in the platform configuration file, an additional
    configuration file is needed for this, specified using the gateset_file
    option. This must be a JSON file consisting of an array of objects, where
    each object has the following form.

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
    )");
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
