/** \file
 * Custom instruction decomposition rule processing logic.
 */

#pragma once

#include "ql/ir/ir.h"

namespace ql {
namespace com {
namespace dec {

/**
 * Predicate function prototype.
 */
using RulePredicate = std::function<utils::Bool(const ir::DecompositionRef&)>;

/**
 * Recursively applies all available decomposition rules (that match the
 * predicate, if given) to the given block. Sub-blocks are not considered; in
 * case structured control-flow blocks exist inside the block and these need to
 * be handled as well, it is the responsibility of the callee to do so. If
 * ignore_schedule is set, the schedule of the decomposition rules is ignored,
 * and instead the statements in the rule are all given the same cycle number as
 * the original statement. If ignore_schedule is not set, the schedule is copied
 * from the decomposition rule, possibly resulting in instructions being
 * reordered.
 */
utils::UInt apply_decomposition_rules(
    const ir::BlockBaseRef &block,
    utils::Bool ignore_schedule = true,
    const RulePredicate &predicate = [](const ir::DecompositionRef&){ return true; }
);

} // namespace dec
} // namespace com
} // namespace ql
