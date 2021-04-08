/** \file
 * Utility functions for dumping statistics to a stream.
 */

#pragma once

#include "ql/utils/str.h"
#include "ql/ir/ir.h"

namespace ql {
namespace com {
namespace statistics {

/**
 * Dumps basic statistics for the given kernel to the given output stream.
 */
void dump(
    const ir::KernelRef &kernel,
    std::ostream &os = std::cout,
    const utils::Str &line_prefix = ""
);

/**
 * Dumps basic statistics for the given program to the given output stream. This
 * only dumps the global statistics, not the statistics for each individual
 * kernel.
 */
void dump(
    const ir::ProgramRef &program,
    std::ostream &os = std::cout,
    const utils::Str &line_prefix = ""
);

} // namespace statistics
} // namespace com
} // namespace ql
