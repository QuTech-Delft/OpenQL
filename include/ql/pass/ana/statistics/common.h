/** \file
 * Defines common types and utility functions related to the statistics passes.
 */

#pragma once

#include "ql/utils/str.h"
#include "ql/utils/list.h"
#include "ql/ir/ir.h"

namespace ql {
namespace pass {
namespace ana {
namespace statistics {

/**
 * Annotation object that may be used by other passes to attach additional
 * statistics to the program and/or kernel nodes. These will then be printed
 * and removed by the next statistics reporting pass, or discarded by a
 * statistics cleaning pass.
 */
struct AdditionalStats {

    /**
     * Additional lines for the statistics report.
     */
    utils::List<utils::Str> stats;

    /**
     * Attaches a statistic to the given kernel node.
     */
    static void push(const ir::KernelRef &kernel, const utils::Str &line);

    /**
     * Attaches a statistic to the given program node.
     */
    static void push(const ir::ProgramRef &program, const utils::Str &line);

    /**
     * Pops all statistics annotations from the given kernel.
     */
    static utils::List<utils::Str> pop(const ir::KernelRef &kernel);

    /**
     * Pops all statistics annotations from the given program.
     */
    static utils::List<utils::Str> pop(const ir::ProgramRef &program);

};

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
} // namespace ana
} // namespace pass
} // namespace ql
