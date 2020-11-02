#include "interactionMatrix.h"

namespace ql {

InteractionMatrix::InteractionMatrix() : Size(0) {
}

InteractionMatrix::InteractionMatrix(circuit ckt, size_t nqubits) {
    Size = nqubits;
    Matrix.resize(Size, std::vector<size_t>(Size, 0));
    for (auto ins : ckt) {
        std::string insName = ins->qasm();
        if (insName.find("cnot") != std::string::npos) {
            // for now the interaction matrix only for cnot
            auto operands = ins->operands;
            if (operands.size() == 2) {
                size_t operand0 = operands[0];
                size_t operand1 = operands[1];
                Matrix[operand0][operand1] += 1;
                Matrix[operand1][operand0] += 1;
            }
        }
    }
}

std::string InteractionMatrix::getString() const {
    std::stringstream ss;

    // Use the following for properly aligned matrix print for visual inspection
    // This can be problematic of width not set properly to be processed by gnuplot script
#define ALIGNMENT (std::setw(4))

    // Use the following to print tabs which will not be visually appealing but it will
    // generate the columns properly for further processing by other tools
    // #define ALIGNMENT ("    ")

    ss << ALIGNMENT << " ";
    for (size_t c = 0; c < Size; c++) {
        ss << ALIGNMENT << "q" + std::to_string(c);
    }
    ss << std::endl;

    for (size_t p = 0; p < Size; p++) {
        ss << ALIGNMENT << "q" + std::to_string(p);
        for (size_t c = 0; c < Size; c++) {
            ss << ALIGNMENT << Matrix[p][c];
        }
        ss << std::endl;
    }
#undef ALIGNMENT

    return ss.str();
}

}