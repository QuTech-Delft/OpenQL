/** \file
 * Defines the old cQASM reader logic that's still used within the API.
 */

#include "ql/ir/compat/cqasm_reader.h"

#include "ql/utils/json.h"
#include "detail/cqasm_reader.h"

namespace ql {
namespace ir {
namespace compat {
namespace cqasm_reader {

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
 * Same as from_file(), be reads from a string instead.
 *
 * \see from_file()
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

} // namespace cqasm_reader
} // namespace compat
} // namespace ir
} // namespace ql
