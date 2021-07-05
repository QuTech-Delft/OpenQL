/** \file
 * Common IR implementation.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/list.h"
#include "ql/ir/gate.h"
#include "ql/ir/kernel.h"

namespace ql {
namespace ir {

/**
 * Cycle numbers in OpenQL, for some historic reason, start at 1 (see discussion
 * in PR #398).
 */
static const utils::UInt FIRST_CYCLE = 1;

/**
 * A bundle of gates that start in the same cycle.
 */
struct Bundle {

    /**
     * The start cycle for all gates in this bundle.
     */
    utils::UInt start_cycle = FIRST_CYCLE;

    /**
     * The maximum gate duration of the gates in this bundle.
     */
    utils::UInt duration_in_cycles = 0;

    /**
     * The list of parallel gates in this bundle.
     */
    utils::List<ir::GateRef> gates = {};

};

/**
* A list of bundles. Note that subsequent bundles can overlap in time.
 */
using Bundles = utils::List<Bundle>;

/**
 * Create a circuit with valid cycle values from the bundled internal
 * representation. The bundles are assumed to be ordered by cycle number.
 */
GateRefs circuiter(const Bundles &bundles);

/**
 * Create a bundled-qasm external representation from the bundled internal
 * representation.
 */
utils::Str qasm(const Bundles &bundles);

/**
 * Create a bundled internal representation from the given kernel with valid
 * cycle information.
 */
Bundles bundler(const KernelRef &kernel);

/**
 * Print the bundles with an indication (taken from 'at') from where this
 * function was called.
 */
void debug_bundles(const utils::Str &at, const Bundles &bundles);

} // namespace ir
} // namespace ql
