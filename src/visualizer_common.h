/** \file
 * Declaration of the visualizer's shared functionalities.
 */

#pragma once

#ifdef WITH_VISUALIZER

#include "utils/num.h"
#include "utils/str.h"
#include "utils/vec.h"
#include "utils/pair.h"
#include "visualizer.h"

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
    utils::Int x0;
    utils::Int y0;
    utils::Int x1;
    utils::Int y1;

    Position4() = delete;
};

struct Position2 {
    utils::Int x;
    utils::Int y;

    Position2() = delete;
};

struct EndPoints {
    const utils::Int start;
    const utils::Int end;
};

struct Dimensions {
    const utils::Int width;
    const utils::Int height;
};

struct GateOperand {
    BitType bitType;
    utils::Int index;

    friend bool operator<(const GateOperand &lhs, const GateOperand &rhs) {
        if (lhs.bitType == QUANTUM && rhs.bitType == CLASSICAL) return true;
        if (lhs.bitType == CLASSICAL && rhs.bitType == QUANTUM) return false;
        return lhs.index < rhs.index;
    }

    friend bool operator>(const GateOperand &lhs, const GateOperand &rhs) {return operator<(rhs, lhs);}
    friend bool operator<=(const GateOperand &lhs, const GateOperand &rhs) {return !operator>(lhs, rhs);}
    friend bool operator>=(const GateOperand &lhs, const GateOperand &rhs) {return !operator<(lhs, rhs);}

    GateOperand() = delete;
};

struct GateProperties {
    utils::Str name;
    utils::Vec<utils::Int> operands;
    utils::Vec<utils::Int> creg_operands;
    utils::Int duration;
    utils::Int cycle;
    gate_type_t type;
    utils::Vec<utils::Int> codewords;
    utils::Str visual_type;

    GateProperties() = delete;
};

Layout parseConfiguration(const utils::Str &configPath);
void validateLayout(Layout &layout);

utils::Vec<GateProperties> parseGates(const ql::quantum_program *program);

utils::Int calculateAmountOfBits(const utils::Vec<GateProperties> &gates, const utils::Vec<utils::Int> GateProperties::* operandType);

utils::Int calculateAmountOfGateOperands(const GateProperties &gate);
utils::Vec<GateOperand> getGateOperands(const GateProperties &gate);
utils::Pair<GateOperand, GateOperand> calculateEdgeOperands(const utils::Vec<GateOperand> &operands, utils::Int amountOfQubits);

void fixMeasurementOperands(utils::Vec<GateProperties> &gates);
utils::Bool isMeasurement(const GateProperties &gate);

Dimensions calculateTextDimensions(const utils::Str &text, utils::Int fontHeight);

void printGates(const utils::Vec<GateProperties> &gates);

} // namespace ql

#endif //WITH_VISUALIZER