/** \file
 * Implementation for converting cQASM files to OpenQL's IR.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/opt.h"
#include "kernel.h"
#include "platform.h"
#include "program.h"

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
    Reader(const quantum_platform &platform, ir::Program &program);
    Reader(const quantum_platform &platform, ir::Program &program, const utils::Json &gateset);
    Reader(const quantum_platform &platform, ir::Program &program, const utils::Str &gateset_fname);
    void string2circuit(const utils::Str &cqasm_str);
    void file2circuit(const utils::Str &cqasm_fname);
};

} // namespace cqasm

// TODO: backward-compatibility for now, should probably just be removed.
using cqasm_reader = cqasm::Reader;

} // namespace ql
