/** \file
 * API header for accessing the cQASM reader.
 */

#pragma once

#include "ql/pass/io/cqasm/read.h"
#include "ql/api/declarations.h"
#include "ql/api/platform.h"
#include "ql/api/program.h"

//============================================================================//
//                               W A R N I N G                                //
//----------------------------------------------------------------------------//
//         Docstrings in this file must manually be kept in sync with         //
//   python/openql.i! This should be automated at some point, but isn't yet.  //
//============================================================================//

namespace ql {
namespace api {

/**
 * cQASM reader interface.
 */
class cQasmReader {
private:

    /**
     * The wrapped cQASM reader.
     */
    ql::utils::Ptr<ql::pass::io::cqasm::read::Reader> cqasm_reader;

public:

    /**
     * The platform associated with the reader.
     */
    const Platform platform;

    /**
     * The program that the cQASM circuits will be added to.
     */
    const Program program;

    /**
     * Builds a cQASM reader for the given platform and program, optionally
     * using a custom instruction set configuration file. This is an old
     * interface; the platform argument is redundant.
     */
    cQasmReader(
        const Platform &platform,
        const Program &program,
        const std::string &gateset_fname = ""
    );

    /**
     * Builds a cQASM reader for the given program, optionally using a custom
     * instruction set configuration file.
     */
    cQasmReader(
        const Program &program,
        const std::string &gateset_fname = ""
    );

    /**
     * Interprets a string as cQASM file and adds its contents to the program
     * associated with this reader.
     */
    void string2circuit(const std::string &cqasm_str);

    /**
     * Interprets a cQASM file and adds its contents to the program associated
     * with this reader.
     */
    void file2circuit(const std::string &cqasm_file_path);

};

} // namespace api
} // namespace ql
