/** \file
 * Implementation for OpenQL's global options.
 */

#include "ql/com/types.h"

namespace ql {
namespace com {

/**
 * Stream operator for SchedulingDirection.
 */
std::ostream &operator<<(std::ostream &os, SchedulingDirection dir) {
    switch (dir) {
        case SchedulingDirection::FORWARD: os << "forward"; break;
        case SchedulingDirection::BACKWARD: os << "backward"; break;
    }
    return os;
}

} // namespace com
} // namespace ql
