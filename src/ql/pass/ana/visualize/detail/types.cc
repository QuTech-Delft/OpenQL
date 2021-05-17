/** \file
 * Declaration of the common types used throughout the visualizer.
 */

#ifdef WITH_VISUALIZER

#include "types.h"

namespace ql {
namespace pass {
namespace ana {
namespace visualize {
namespace detail {

void assertPositive(utils::Int parameterValue, const utils::Str &parameterName) {
    if (parameterValue < 0) QL_FATAL(parameterName << " is negative. Only positive values are allowed!");
}

void assertPositive(utils::Real parameterValue, const utils::Str &parameterName) {
    if (parameterValue < 0) QL_FATAL(parameterName << " is negative. Only positive values are allowed!");
}

} // namespace detail
} // namespace visualize
} // namespace ana
} // namespace pass
} // namespace ql

#endif //WITH_VISUALIZER
