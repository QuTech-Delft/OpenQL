/** \file
 * Qubit interaction matrix generator.
 */

#pragma once

#include "utils/num.h"
#include "utils/str.h"
#include "utils/vec.h"

#include "circuit.h"

namespace ql {

class InteractionMatrix {
private:
    utils::Vec<utils::Vec<size_t>> Matrix;
    utils::UInt Size;

public:
    InteractionMatrix(const circuit &ckt, utils::UInt nqubits);
    utils::Str getString() const;
};

} // namespace ql
