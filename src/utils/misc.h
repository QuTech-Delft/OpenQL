/** \file
 * Provides miscellaneous utilities.
 */

#pragma once

// get the number of elements in an array
#define ELEM_CNT(x) (sizeof(x)/sizeof(x[0]))

namespace ql {
namespace utils {

// from https://stackoverflow.com/questions/9146395/reset-c-int-array-to-zero-the-fastest-way
template<typename T, size_t SIZE>
inline void zero(T(&arr)[SIZE]) {
    memset(arr, 0, SIZE * sizeof(T));
}

} // namespace utils
} // namespace ql
