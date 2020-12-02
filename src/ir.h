/** \file
 * Common IR implementation.
 */

#pragma once

#include "utils/num.h"
#include "utils/str.h"
#include "utils/list.h"
#include "gate.h"
#include "circuit.h"

namespace ql {
namespace ir {

typedef utils::List<gate *>section_t;

class bundle_t {
public:
    utils::UInt start_cycle;                         // start cycle for all gates in parallel_sections
    utils::UInt duration_in_cycles;                  // the maximum gate duration in parallel_sections
    utils::List<section_t> parallel_sections;
};

typedef utils::List<bundle_t> bundles_t;          // note that subsequent bundles can overlap in time

/**
 * Create a circuit with valid cycle values from the bundled internal
 * representation.
 */
circuit circuiter(const bundles_t &bundles);

/**
 * Create a bundled-qasm external representation from the bundled internal
 * representation.
 */
utils::Str qasm(const bundles_t &bundles);

/**
 * Create a bundled internal representation from the circuit with valid cycle
 * information.
 *
 * assumes gatep->cycle attribute reflects the cycle assignment;
 * assumes circuit being a vector of gate pointers is ordered by this cycle value;
 * create bundles in a single scan over the circuit, using currBundle and currCycle as state:
 *  - currBundle: bundle that is being put together; currBundle copied into output bundles when the bundle has been done
 *  - currCycle: cycle at which currBundle will be put; equals cycle value of all contained gates
 *
 * FIXME HvS cycles_valid must be true before each call to this bundler
 */
bundles_t bundler(const circuit &circ, utils::UInt cycle_time);

/**
 * Print the bundles with an indication (taken from 'at') from where this
 * function was called.
 */
void DebugBundles(const utils::Str &at, const bundles_t &bundles);

} // namespace ir
} // namespace ql
