/** \file
 * Defines the cQASM reader pass.
 */

#pragma once

#include "ql/utils/str.h"
#include "ql/utils/json.h"
#include "ql/utils/opt.h"
#include "ql/pmgr/pass_types/specializations.h"

namespace ql {
namespace pass {
namespace io {
namespace cqasm {
namespace read {

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
 * Same as file(), be reads from a string instead.
 *
 * \see file()
 */
void from_string(
    const ir::compat::ProgramRef &program,
    const utils::Str &cqasm_body,
    const utils::Json &gateset={}
);

/**
 * cQASM reader pass.
 */
class ReadCQasmPass : public pmgr::pass_types::ProgramTransformation {
protected:

    /**
     * Dumps docs for the cQASM reader.
     */
    void dump_docs(
        std::ostream &os,
        const utils::Str &line_prefix
    ) const override;

public:

    /**
     * Returns a user-friendly type name for this pass.
     */
    utils::Str get_friendly_type() const override;

    /**
     * Constructs a cQASM reader.
     */
    ReadCQasmPass(
        const utils::Ptr<const pmgr::Factory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Runs the cQASM reader.
     */
    utils::Int run(
        const ir::compat::ProgramRef &program,
        const pmgr::pass_types::Context &context
    ) const override;

};

/**
 * Shorthand for referring to the pass using namespace notation.
 */
using Pass = ReadCQasmPass;

} // namespace reader
} // namespace cqasm
} // namespace io
} // namespace pass
} // namespace ql
