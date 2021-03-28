/** \file
 * Qubit interaction matrix generator.
 */

#pragma once

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/vec.h"

#include "circuit.h"

namespace ql {

class InteractionMatrix {
private:
    utils::Vec<utils::Vec<utils::UInt>> Matrix;
    utils::UInt Size;

public:
    InteractionMatrix(const ir::Circuit &ckt, utils::UInt nqubits);
    utils::Str getString() const;
};

} // namespace ql
