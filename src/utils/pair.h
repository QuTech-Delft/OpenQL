/** \file
 * Provides a wrapper for std::vector that's safer to use and provides more
 * context when something goes wrong at runtime.
 */

#pragma once

#include <utility>

namespace ql {
namespace utils {

/**
 * Shorthand for pair.
 */
template <typename T1, typename T2>
using Pair = std::pair<T1, T2>;

} // namespace utils
} // namespace ql

/**
 * Stream << overload for Pair<>.
 */
template <class T1, class T2>
std::ostream &operator<<(std::ostream &os, const ::ql::utils::Pair<T1, T2> &pair) {
    os << "<" << pair.first << ", " << pair.second << ">";
    return os;
}
