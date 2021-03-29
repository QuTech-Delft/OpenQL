/** \file
 * Implementation for converting cQASM files to OpenQL's IR.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/ptr.h"
#include "ql/plat/platform.h"
#include "ql/ir/ir.h"

namespace ql {
namespace cqasm {

// Opaque forward declaration for the actual implementation of the reader, to
// keep the header file clean.
class ReaderImpl;

/**
 * Class for converting cQASM files to OpenQL circuits.
 */
class Reader {
private:
    utils::Opt<ReaderImpl> impl;
public:
    Reader(const plat::PlatformRef &platform, const ir::ProgramRef &program);
    Reader(const plat::PlatformRef &platform, const ir::ProgramRef &program, const utils::Json &gateset);
    Reader(const plat::PlatformRef &platform, const ir::ProgramRef &program, const utils::Str &gateset_fname);
    void string2circuit(const utils::Str &cqasm_str);
    void file2circuit(const utils::Str &cqasm_fname);
};

} // namespace cqasm
} // namespace ql
