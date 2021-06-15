/** \file
 * cQASM 1.2 writer logic as human-readable complement of the IR.
 */

#pragma once

#include "ql/ir/ir.h"

namespace ql {
namespace ir {
namespace cqasm {

/**
 * Writes a cQASM 1.2 representation of the IR to the given stream with the
 * given line prefix.
 */
void write(
    const Ref &ir,
    std::ostream &os = std::cout,
    const utils::Str &line_prefix = ""
);

} // namespace cqasm
} // namespace ir
} // namespace ql
