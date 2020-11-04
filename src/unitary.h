/**
 * @file   unitary.h
 * @date   12/2018
 * @author Imran Ashraf
 * @author Anneriet Krol
 * @brief  unitary matrix (decomposition) implementation
 */

#pragma once

#include <complex>
#include <string>

#include "gate.h"
#include "utils/exception.h"

namespace ql {

class unitary {
public:
    std::string name;
    std::vector<std::complex<double>> array;
    std::vector<std::complex<double>> SU;
    double delta; // JvS: is this even used?
    double alpha;
    double beta;
    double gamma;
    bool is_decomposed;
    std::vector<double> instructionlist;

    unitary();
    unitary(const std::string &name, const std::vector<std::complex<double>> &array);
    double size() const;
    void decompose();
    static bool is_decompose_support_enabled();
};

} // namespace ql
