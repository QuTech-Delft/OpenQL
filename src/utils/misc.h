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

template<typename T>
int sign_of(T val) {
    return (T(0) < val) - (val < T(0));
}

} // namespace utils
} // namespace ql
