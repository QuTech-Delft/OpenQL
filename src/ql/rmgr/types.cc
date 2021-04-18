/** \file
 * Defines some basic types used by all resources.
 */

#include "ql/rmgr/types.h"

namespace ql {
namespace rmgr {

/**
 * Stream operator for Direction.
 */
std::ostream &operator<<(std::ostream &os, Direction dir) {
    switch (dir) {
        case Direction::FORWARD: os << "forward"; break;
        case Direction::BACKWARD: os << "backward"; break;
        case Direction::UNDEFINED: os << "undefined"; break;
    }
    return os;
}

} // namespace rmgr
} // namespace ql
