/** \file
 * Provides a wrapper for std::list that's safer to use and provides more
 * context when something goes wrong at runtime.
 */

#pragma once

#include <list>

namespace ql {
namespace utils {

// TODO: wrap properly
template <typename T, typename Alloc = std::allocator<T>>
using List = std::list<T, Alloc>;

} // namespace utils
} // namespace ql
