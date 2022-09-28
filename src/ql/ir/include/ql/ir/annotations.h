/** \file
 * Defines common types and utility functions related to the statistics passes.
 */

#pragma once

#include "ql/utils/str.h"
#include "ql/utils/list.h"
#include "ql/ir/ir.h"
#include "ql/ir/compat/compat.h"

namespace ql {
namespace ir {

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
     * Attaches a statistic to the given block node.
     */
    static void push(const ir::BlockRef &block, const utils::Str &line);

    /**
     * Attaches a statistic to the given program node.
     */
    static void push(const ir::ProgramRef &program, const utils::Str &line);

    /**
     * Attaches a statistic to the given kernel node.
     */
    static void push(const ir::compat::KernelRef &kernel, const utils::Str &line);

    /**
     * Attaches a statistic to the given program node.
     */
    static void push(const ir::compat::ProgramRef &program, const utils::Str &line);

    /**
     * Pops all statistics annotations from the given kernel.
     */
    static utils::List<utils::Str> pop(const ir::BlockRef &block);

    /**
     * Pops all statistics annotations from the given program.
     */
    static utils::List<utils::Str> pop(const ir::ProgramRef &program);

};

} // namespace ir
} // namespace ql
