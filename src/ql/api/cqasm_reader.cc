/** \file
 * API header for accessing the cQASM reader.
 */

#include "ql/api/cqasm_reader.h"

//============================================================================//
//                               W A R N I N G                                //
//----------------------------------------------------------------------------//
//         Docstrings in this file must manually be kept in sync with         //
//   cqasm_reader.i! This should be automated at some point, but isn't yet.   //
//============================================================================//

namespace ql {
namespace api {

/**
 * Builds a cQASM reader for the given platform and program, optionally
 * using a custom instruction set configuration file. This is an old
 * interface; the platform argument is redundant.
 */
cQasmReader::cQasmReader(
    const Platform &platform,
    const Program &program,
    const std::string &gateset_fname
) :
    platform(platform),
    program(program)
{
    if (platform.platform.get_ptr() != program.program->platform.get_ptr()) {
        throw ql::utils::Exception(
            "Mismatch between the given platform and the platform "
            "associated with the given program"
        );
    }
    if (gateset_fname.empty()) {
        cqasm_reader.emplace(platform.platform, program.program);
    } else {
        cqasm_reader.emplace(platform.platform, program.program, gateset_fname);
    }
}

/**
 * Builds a cQASM reader for the given program, optionally using a custom
 * instruction set configuration file.
 */
cQasmReader::cQasmReader(
    const Program &program,
    const std::string &gateset_fname
) :
    platform(program.platform),
    program(program)
{
    if (gateset_fname.empty()) {
        cqasm_reader.emplace(platform.platform, program.program);
    } else {
        cqasm_reader.emplace(platform.platform, program.program, gateset_fname);
    }
}

/**
 * Interprets a string as cQASM file and adds its contents to the program
 * associated with this reader.
 */
void cQasmReader::string2circuit(const std::string &cqasm_str) {
    cqasm_reader->string2circuit(cqasm_str);
}

/**
 * Interprets a cQASM file and adds its contents to the program associated
 * with this reader.
 */
void cQasmReader::file2circuit(const std::string &cqasm_file_path) {
    cqasm_reader->file2circuit(cqasm_file_path);
}

} // namespace api
} // namespace ql
