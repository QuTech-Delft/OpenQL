/** \file
 * Qubit interaction matrix generator.
 */

#include "ql/com/interaction_matrix.h"

#include <iomanip>

namespace ql {
namespace com {

using namespace utils;

InteractionMatrix::InteractionMatrix(
    const ir::KernelRef &kernel
) : size(kernel->qubit_count) {
    matrix.resize(size, Vec<UInt>(size, 0));
    for (auto ins : kernel->c) {
        Str insName = ins->qasm();
        if (insName.find("cnot") != Str::npos) {
            // for now the interaction matrix only for cnot
            auto operands = ins->operands;
            if (operands.size() == 2) {
                UInt operand0 = operands[0];
                UInt operand1 = operands[1];
                matrix[operand0][operand1] += 1;
                matrix[operand1][operand0] += 1;
            }
        }
    }
}

/**
 * Returns the embedded matrix.
 */
const InteractionMatrix::Matrix &InteractionMatrix::get_matrix() const {
    return matrix;
}

/**
 * Returns the matrix as a string.
 */
Str InteractionMatrix::get_string() const {
    StrStrm ss;

    // Use the following for properly aligned matrix print for visual inspection
    // This can be problematic of width not set properly to be processed by gnuplot script
#define ALIGNMENT (std::setw(4))

    // Use the following to print tabs which will not be visually appealing but it will
    // generate the columns properly for further processing by other tools
    // #define ALIGNMENT ("    ")

    ss << ALIGNMENT << " ";
    for (UInt c = 0; c < size; c++) {
        ss << ALIGNMENT << "q" + to_string(c);
    }
    ss << std::endl;

    for (UInt p = 0; p < size; p++) {
        ss << ALIGNMENT << "q" + to_string(p);
        for (UInt c = 0; c < size; c++) {
            ss << ALIGNMENT << matrix[p][c];
        }
        ss << std::endl;
    }
#undef ALIGNMENT

    return ss.str();
}

} // namespace com
} // namespace ql
