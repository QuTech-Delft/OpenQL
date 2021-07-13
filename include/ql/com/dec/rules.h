/** \file
 * Custom instruction decomposition rule processing logic.
 */

#pragma once

#include "ql/ir/ir.h"

namespace ql {
namespace com {
namespace dec {

/**
 * Recursively applies all available decomposition rules (that match the
 * predicate, if given) to the given block. If ignore_schedule is set, the
 * schedule of the decomposition rules is ignored, and instead the statements
 * in the rule are all given the same cycle number as the original statement.
 * If ignore_schedule is not set, the schedule is copied from the decomposition
 * rule, possibly resulting in instructions being reordered. Returns the number
 * of decomposition rules that were applied.
 */
utils::UInt apply_decomposition_rules(
    const ir::Ref &ir,
    const ir::BlockBaseRef &block,
    utils::Bool ignore_schedule = true,
    const std::function<utils::Bool(const ir::DecompositionRef&)> &predicate
        = [](const ir::DecompositionRef&){ return true; }
);

} // namespace dec
} // namespace com
} // namespace ql
