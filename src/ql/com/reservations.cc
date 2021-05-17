/** \file
 * Bookkeeping class for tracking cycle range reservations.
 *
 * Primarily intended to be used by resources.
 */

#include "ql/com/reservations.h"

namespace ql {
namespace com {
namespace reservations {

/**
 * String conversion for Empty.
 */
std::ostream &operator<<(std::ostream &os, Empty e) {
    return os << "reserved";
}

} // namespace reservations
} // namespace com
} // namespace ql
