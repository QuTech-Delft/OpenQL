/** \file
 * Unitary matrix implementation, originally taken from qx simulator.
 */

#pragma once

#include <iostream>
#include <iomanip>
#include "ql/utils/num.h"

namespace ql {
namespace ir {

template <typename T, utils::UInt N>
class Matrix {
public:

    T m[N * N];

    Matrix() {
        for (utils::UInt i = 0; i < N * N; ++i) {
            m[i] = 0;
        }
    }

    Matrix(const T *pm) {
        for (utils::UInt i = 0; i < N * N; ++i) {
            m[i] = pm[i];
        }
    }

    T &operator()(utils::UInt r, utils::UInt c) {
        return m[r*N + c];
    }

    utils::UInt size() const {
        return N;
    }

    void dump() const {
        std::cout << "[i] ---[matrix]-----------------------------------------------------" << std::endl;
        std::cout << std::fixed;
        for (utils::UInt r = 0; r < N; ++r) {
            for (utils::UInt c = 0; c < N; ++c) {
                std::cout << std::showpos << std::setw(5) << m[r * N + c] << "\t";
            }
            std::cout << std::endl;
        }
        std::cout << "[i] ----------------------------------------------------------------" << std::endl;
    }

};

typedef Matrix<utils::Complex, 2> Complex2by2Matrix;

} // namespace ir
} // namespace ql
