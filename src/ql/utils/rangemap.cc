/** \file
 * A map (and set) mapping from non-overlapping *ranges* of keys to values.
 */

#include "ql/utils/rangemap.h"

namespace ql {
namespace utils {

/**
 * String conversion for Empty.
 */
std::ostream &operator<<(std::ostream &os, const Nothing &) {
    return os;
}

/**
 * String conversion for RangeMatchType.
 */
std::ostream &operator<<(std::ostream &os, RangeMatchType rmt) {
    switch (rmt) {
        case RangeMatchType::NONE:      return os << "none";
        case RangeMatchType::PARTIAL:   return os << "partial";
        case RangeMatchType::MULTIPLE:  return os << "multiple";
        case RangeMatchType::SUPER:     return os << "super";
        case RangeMatchType::SUB:       return os << "sub";
        case RangeMatchType::EXACT:     return os << "exact";
        default:                        return os << "<UNKNOWN>";
    }
}

} // namespace utils
} // namespace ql
