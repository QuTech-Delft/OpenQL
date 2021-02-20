/** \file
 * Unitary matrix (decomposition) implementation.
 */

#pragma once

#include "utils/num.h"
#include "utils/str.h"
#include "utils/vec.h"
#include "gate.h"

namespace ql {

class unitary {
public:
    utils::Str name;
    utils::Vec<utils::Complex> array;
    utils::Vec<utils::Complex> SU;
    utils::Real delta; // JvS: is this even used?
    utils::Real alpha;
    utils::Real beta;
    utils::Real gamma;
    utils::Bool is_decomposed;
    utils::Vec<utils::Real> instructionlist;

    unitary();
    unitary(const utils::Str &name, const utils::Vec<utils::Complex> &array);
    utils::Real size() const;
    void decompose();
    static utils::Bool is_decompose_support_enabled();
};

} // namespace ql
