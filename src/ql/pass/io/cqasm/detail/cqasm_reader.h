/** \file
 * Implementation for converting cQASM files to OpenQL's IR.
 *
 * FIXME: this file and its cc file are a bit messy because I originall didn't
 *  want to change the C++ interface for the Reader class. This is no longer the
 *  case; it's in a completely different namespace now anyway. The reader code
 *  should be broken up into multiple files, with proper source/header class
 *  definitions.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/ptr.h"
#include "ql/plat/platform.h"
#include "ql/ir/ir.h"

namespace ql {
namespace pass {
namespace io {
namespace cqasm {
namespace read {
namespace detail {

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

} // namespace detail
} // namespace read
} // namespace cqasm
} // namespace io
} // namespace pass
} // namespace ql
