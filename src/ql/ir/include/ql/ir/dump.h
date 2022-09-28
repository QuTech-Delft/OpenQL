/** \file
 * Defines the statistics reporting pass.
 */

#pragma once

#include <iosfwd>
#include "ql/ir/ir.h"

namespace ql {
namespace ir {

/**
 * Dumps basic statistics for the given block to the given output stream.
 */
void dump(
    const ir::Ref &ir,
    const ir::BlockRef &block,
    std::ostream &os = std::cout,
    const utils::Str &line_prefix = ""
);

/**
 * Dumps basic statistics for the given program to the given output stream. This
 * only dumps the global statistics, not the statistics for each individual
 * kernel.
 */
void dump(
    const ir::Ref &ir,
    const ir::ProgramRef &program,
    std::ostream &os = std::cout,
    const utils::Str &line_prefix = ""
);

/**
 * Dumps statistics for the given program and its top-level blocks to the given
 * output stream.
 */
void dump_all(
    const ir::Ref &ir,
    std::ostream &os = std::cout,
    const utils::Str &line_prefix = ""
);

} // namespace ir
} // namespace ql
