/** \file
 * cQASM 1.2 reader logic as human-readable complement of the IR.
 */

#pragma once

#include "ql/utils/str.h"
#include "ql/ir/ir.h"

namespace ql {
namespace ir {
namespace cqasm {

/**
 * Reads a cQASM 1.2 file into the IR. If reading is successful, ir->program is
 * completely replaced. data represents the cQASM file contents, fname specifies
 * the filename if one exists for the purpose of generating better error
 * messages.
 */
void read(const Ref &ir, const utils::Str &data, const utils::Str &fname = "<unknown>");

} // namespace cqasm
} // namespace ir
} // namespace ql
