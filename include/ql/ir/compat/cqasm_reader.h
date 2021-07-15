/** \file
 * Defines the old cQASM reader logic that's still used within the API.
 */

#pragma once

#include "ql/utils/str.h"
#include "ql/utils/json.h"
#include "ql/utils/opt.h"
#include "ql/pmgr/pass_types/specializations.h"

namespace ql {
namespace ir {
namespace compat {
namespace cqasm_reader {

// Opaque forward declaration for the actual implementation of the reader, to
// keep the header file clean.
namespace detail {
class ReaderImpl;
}

/**
 * Class for converting cQASM files to OpenQL circuits.
 */
class Reader {
private:
    utils::Opt<detail::ReaderImpl> impl;
public:
    Reader(const ir::compat::PlatformRef &platform, const ir::compat::ProgramRef &program);
    Reader(const ir::compat::PlatformRef &platform, const ir::compat::ProgramRef &program, const utils::Json &gateset);
    Reader(const ir::compat::PlatformRef &platform, const ir::compat::ProgramRef &program, const utils::Str &gateset_fname);
    void string2circuit(const utils::Str &cqasm_str);
    void file2circuit(const utils::Str &cqasm_fname);
};

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
    const utils::Json &gateset={}
);

/**
 * Same as from_file(), be reads from a string instead.
 *
 * \see from_file()
 */
void from_string(
    const ir::compat::ProgramRef &program,
    const utils::Str &cqasm_body,
    const utils::Json &gateset={}
);

} // namespace cqasm_reader
} // namespace compat
} // namespace ir
} // namespace ql
