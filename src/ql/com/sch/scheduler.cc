/** \file
 * Defines a basic resource-constrained ASAP/ALAP list scheduler for use as a
 * building block within more complex schedulers.
 */

#include "ql/com/sch/scheduler.h"

namespace ql {
namespace com {
namespace sch {

// Explicitly instantiate the common scheduler types to reduce compilation time.
template class Scheduler<TrivialHeuristic>;
template class Scheduler<CriticalPathHeuristic>;
template class Scheduler<DeepCriticality::Heuristic>;

} // namespace sch
} // namespace com
} // namespace ql
