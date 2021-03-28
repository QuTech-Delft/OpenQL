/** \file
 * Utilities for working with pairs.
 */

#pragma once

#include <ostream>
#include <utility>

namespace ql {
namespace utils {

/**
 * Shorthand for pair.
 */
template <typename T1, typename T2>
using Pair = std::pair<T1, T2>;

/**
 * Stream << overload for Pair<>.
 */
template <class T1, class T2>
std::ostream &operator<<(std::ostream &os, const ::ql::utils::Pair<T1, T2> &pair) {
    os << "<" << pair.first << ", " << pair.second << ">";
    return os;
}

} // namespace utils
} // namespace ql
