/** \file
 * Qubit interaction matrix generator.
 */

#include "interactionMatrix.h"

namespace ql {

using namespace utils;

InteractionMatrix::InteractionMatrix(const circuit &ckt, UInt nqubits) {
    Size = nqubits;
    Matrix.resize(Size, Vec<UInt>(Size, 0));
    for (auto ins : ckt) {
        Str insName = ins->qasm();
        if (insName.find("cnot") != Str::npos) {
            // for now the interaction matrix only for cnot
            auto operands = ins->operands;
            if (operands.size() == 2) {
                UInt operand0 = operands[0];
                UInt operand1 = operands[1];
                Matrix[operand0][operand1] += 1;
                Matrix[operand1][operand0] += 1;
            }
        }
    }
}

Str InteractionMatrix::getString() const {
    StrStrm ss;

    // Use the following for properly aligned matrix print for visual inspection
    // This can be problematic of width not set properly to be processed by gnuplot script
#define ALIGNMENT (std::setw(4))

    // Use the following to print tabs which will not be visually appealing but it will
    // generate the columns properly for further processing by other tools
    // #define ALIGNMENT ("    ")

    ss << ALIGNMENT << " ";
    for (UInt c = 0; c < Size; c++) {
        ss << ALIGNMENT << "q" + to_string(c);
    }
    ss << std::endl;

    for (UInt p = 0; p < Size; p++) {
        ss << ALIGNMENT << "q" + to_string(p);
        for (UInt c = 0; c < Size; c++) {
            ss << ALIGNMENT << Matrix[p][c];
        }
        ss << std::endl;
    }
#undef ALIGNMENT

    return ss.str();
}

}