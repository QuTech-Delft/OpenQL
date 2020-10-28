/**
 * @file   interactionMatrix.h
 * @date   06/2017
 * @author Imran Ashraf
 * @brief  qubit interaction matrix
 */

#pragma once

#include <string>
#include <vector>

#include "utils.h"
#include "circuit.h"

namespace ql {

class InteractionMatrix
{
private:
    std::vector<std::vector<size_t>> Matrix;
    size_t Size;

public:
    InteractionMatrix();
    InteractionMatrix(ql::circuit ckt, size_t nqubits);
    std::string getString() const;
};

} // namespace ql
