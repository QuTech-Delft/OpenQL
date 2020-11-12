/** \file
 * Unitary matrix (decomposition) implementation.
 */

#pragma once

#include <complex>
#include "utils/str.h"
#include "utils/vec.h"
#include "gate.h"

namespace ql {

class unitary {
public:
    utils::Str name;
    utils::Vec<std::complex<double>> array;
    utils::Vec<std::complex<double>> SU;
    double delta; // JvS: is this even used?
    double alpha;
    double beta;
    double gamma;
    bool is_decomposed;
    utils::Vec<double> instructionlist;

    unitary();
    unitary(const utils::Str &name, const utils::Vec<std::complex<double>> &array);
    double size() const;
    void decompose();
    static bool is_decompose_support_enabled();
};

} // namespace ql
