/** \file
 * Provides a wrapper for std::set that's safer to use and provides more
 * context when something goes wrong at runtime.
 *
 * TODO: still needs to be implemented. Currently just a typedef to set.
 */

#pragma once

#include <set>

namespace ql {
namespace utils {

template <class T, class Compare = std::less<T>, class Allocator = std::allocator<T>>
using Set = std::set<T, Compare, Allocator>;

} // namespace utils
} // namespace ql
