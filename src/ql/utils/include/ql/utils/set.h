/** \file
 * Provides a wrapper for std::set that's safer to use and provides more
 * context when something goes wrong at runtime.
 *
 * TODO: still needs to be implemented. Currently just a typedef to set.
 */

#pragma once

#include <set>
#include <iostream>

namespace ql {
namespace utils {

template <class T, class Compare = std::less<T>, class Allocator = std::allocator<T>>
using Set = std::set<T, Compare, Allocator>;

} // namespace utils
} // namespace ql

// FIXME: need a *reasonable* solution for this...
namespace std {

/**
 * Stream << overload for Set<>.
 */
template <class T>
std::ostream &operator<<(std::ostream &os, const std::set<T> &set) {
    os << "[";
    bool first = true;
    for (const auto &it : set) {
        if (first) {
            first = false;
        } else {
            os << ", ";
        }
        os << it;
    }
    os << "]";
    return os;
}

} // namespace std