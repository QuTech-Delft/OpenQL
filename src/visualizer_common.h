/** \file
 * Declaration of the visualizer's shared functionalities.
 */

#pragma once

#ifdef WITH_VISUALIZER

#include "visualizer.h"
#include "visualizer_types.h"
#include "utils/num.h"
#include "utils/str.h"
#include "utils/vec.h"
#include "utils/pair.h"

namespace ql {

std::vector<GateProperties> parseGates(const ql::quantum_program* program);

utils::Int calculateAmountOfBits(const utils::Vec<GateProperties> &gates, const utils::Vec<utils::Int> GateProperties::* operandType);

utils::Int calculateAmountOfGateOperands(const GateProperties &gate);
utils::Vec<GateOperand> getGateOperands(const GateProperties &gate);
utils::Pair<GateOperand, GateOperand> calculateEdgeOperands(const utils::Vec<GateOperand> &operands, utils::Int amountOfQubits);

void fixMeasurementOperands(utils::Vec<GateProperties> &gates);
utils::Bool isMeasurement(const GateProperties &gate);

Dimensions calculateTextDimensions(const utils::Str &text, utils::Int fontHeight);

std::string generateFilePath(const Str &filename, const Str &extension);

void printGates(const utils::Vec<GateProperties> &gates);

} // namespace ql

#endif //WITH_VISUALIZER