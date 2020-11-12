/** \file
 * Unitary matrix implementation, originally taken from qx simulator.
 */

#pragma once

#include <iostream>
#include <iomanip>
#include <complex>

namespace ql {

template <typename T, size_t N>
class matrix {
public:

    T m[N * N];

    matrix() {
        for (size_t i = 0; i < N * N; ++i) {
            m[i] = 0;
        }
    }

    matrix(const T *pm) {
        for (size_t i = 0; i < N * N; ++i) {
            m[i] = pm[i];
        }
    }

    T &operator()(uint32_t r, uint32_t c) {
        return m[r*N + c];
    }

    uint32_t size() const {
        return N;
    }

    void dump() const {
        std::cout << "[i] ---[matrix]-----------------------------------------------------" << std::endl;
        std::cout << std::fixed;
        for (int32_t r = 0; r < N; ++r) {
            for (int32_t c = 0; c < N; ++c) {
                std::cout << std::showpos << std::setw(5) << m[r * N + c] << "\t";
            }
            std::cout << std::endl;
        }
        std::cout << "[i] ----------------------------------------------------------------" << std::endl;
    }

};

typedef std::complex<double> complex_t;
typedef matrix<complex_t, 2> cmat_t;

} // namespace ql
