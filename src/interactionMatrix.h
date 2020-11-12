/** \file
 * Qubit interaction matrix generator.
 */

#pragma once

#include "utils/str.h"
#include "utils/vec.h"

#include "circuit.h"

namespace ql {

class InteractionMatrix {
private:
    utils::Vec<utils::Vec<size_t>> Matrix;
    size_t Size;

public:
    InteractionMatrix(const circuit &ckt, size_t nqubits);
    utils::Str getString() const;
};

} // namespace ql
