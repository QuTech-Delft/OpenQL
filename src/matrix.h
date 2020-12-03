/** \file
 * Unitary matrix implementation, originally taken from qx simulator.
 */

#pragma once

#include <iostream>
#include <iomanip>
#include "utils/num.h"

namespace ql {

template <typename T, utils::UInt N>
class matrix {
public:

    T m[N * N];

    matrix() {
        for (utils::UInt i = 0; i < N * N; ++i) {
            m[i] = 0;
        }
    }

    matrix(const T *pm) {
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

typedef matrix<utils::Complex, 2> cmat_t;

} // namespace ql
