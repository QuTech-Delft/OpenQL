/**
 * @file   ir.h
 * @date   02/2018
 * @author Imran Ashraf
 * @brief  common IR implementation
 */

#pragma once

#include "gate.h"
#include "circuit.h"

#include <vector>
#include <list>

namespace ql {
namespace ir {

typedef std::list<ql::gate *>section_t;

class bundle_t {
public:
    size_t start_cycle;                         // start cycle for all gates in parallel_sections
    size_t duration_in_cycles;                  // the maximum gate duration in parallel_sections
    std::list<section_t> parallel_sections;
};

typedef std::list<bundle_t> bundles_t;          // note that subsequent bundles can overlap in time

/**
 * Create a circuit with valid cycle values from the bundled internal
 * representation.
 */
ql::circuit circuiter(const bundles_t &bundles);

/**
 * Create a bundled-qasm external representation from the bundled internal
 * representation.
 */
std::string qasm(const bundles_t &bundles);

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
bundles_t bundler(const ql::circuit &circ, size_t cycle_time);

/**
 * Print the bundles with an indication (taken from 'at') from where this
 * function was called.
 */
void DebugBundles(const std::string &at, const bundles_t &bundles);

} // namespace ir
} // namespace ql
