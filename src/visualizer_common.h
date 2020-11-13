/**
 * @file   visualizer_common.h
 * @date   11/2020
 * @author Tim van der Meer
 * @brief  declaration of the visualizer's shared functionalities
 */

#pragma once

#ifdef WITH_VISUALIZER

#include "visualizer.h"

// These undefs are necessary to avoid name collisions between CImg and Lemon.
#undef cimg_use_opencv
#undef True

#undef False
#undef IN
#undef OUT

namespace ql {

// enum class VisualizationType {

//     /**
//      * Visualize the quantum program as a circuit composed of abstract gates, or
//      * waveforms acting on qubits and classical registers
//      */
//     CIRCUIT,

//     /**
//      * Visualize the quantum program as a qubit interaction graph, with the
//      * labels of edges between qubits indicating the number of interactions.
//      */
//     INTERACTION_GRAPH
// };

enum BitType {CLASSICAL, QUANTUM};

struct Position4 {
    long x0 = 0;
    long y0 = 0;
    long x1 = 0;
    long y1 = 0;
};

struct Position2 {
    long x = 0;
    long y = 0;
};

struct EndPoints {
    const int start;
    const int end;
};

struct Dimensions {
    const int width;
    const int height;
};

struct GateOperand {
    BitType bitType = QUANTUM;
    int index = 0;

    friend bool operator<(const GateOperand &lhs, const GateOperand &rhs) {
        if (lhs.bitType == QUANTUM && rhs.bitType == CLASSICAL) return true;
        if (lhs.bitType == CLASSICAL && rhs.bitType == QUANTUM) return false;
        return lhs.index < rhs.index;
    }

    friend bool operator>(const GateOperand &lhs, const GateOperand &rhs) {return operator<(rhs, lhs);}
    friend bool operator<=(const GateOperand &lhs, const GateOperand &rhs) {return !operator>(lhs, rhs);}
    friend bool operator>=(const GateOperand &lhs, const GateOperand &rhs) {return !operator<(lhs, rhs);}
};

struct GateProperties {
    std::string name;
    std::vector<int> operands;
    std::vector<int> creg_operands;
    int duration = 0;
    int cycle = 0;
    gate_type_t type = __custom_gate__;
    std::vector<int> codewords;
    std::string visual_type;
};

Layout parseConfiguration(const std::string &configPath);
void validateLayout(Layout &layout);

std::vector<GateProperties> parseGates(const ql::quantum_program* program);

int calculateAmountOfBits(const std::vector<GateProperties> gates, const std::vector<int> GateProperties::* operandType);

int calculateAmountOfGateOperands(const GateProperties gate);
std::vector<GateOperand> getGateOperands(const GateProperties gate);
std::pair<GateOperand, GateOperand> calculateEdgeOperands(const std::vector<GateOperand> operands, const int amountOfQubits);

void fixMeasurementOperands(std::vector<GateProperties> &gates);
bool isMeasurement(const GateProperties gate);

Dimensions calculateTextDimensions(const std::string &text, const int fontHeight);

void printGates(const std::vector<GateProperties> gates);
int safe_int_cast(const size_t argument);

} // namespace ql

#endif //WITH_VISUALIZER