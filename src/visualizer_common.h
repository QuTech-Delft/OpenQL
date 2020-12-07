/** \file
 * Declaration of the visualizer's shared functionalities.
 */

#pragma once

#ifdef WITH_VISUALIZER

#include "visualizer_types.h"
#include "program.h"
#include "utils/num.h"
#include "utils/str.h"
#include "utils/vec.h"
#include "utils/pair.h"

namespace ql {

utils::Vec<GateProperties> parseGates(const quantum_program* program);

utils::Int calculateAmountOfBits(const utils::Vec<GateProperties> &gates, const utils::Vec<utils::Int> GateProperties::* operandType);

utils::Int calculateAmountOfGateOperands(const GateProperties &gate);
utils::Vec<GateOperand> getGateOperands(const GateProperties &gate);
utils::Pair<GateOperand, GateOperand> calculateEdgeOperands(const utils::Vec<GateOperand> &operands, const utils::Int amountOfQubits);

void fixMeasurementOperands(utils::Vec<GateProperties> &gates);
utils::Bool isMeasurement(const GateProperties &gate);

utils::Str generateFilePath(const utils::Str &filename, const utils::Str &extension);

void printGates(const utils::Vec<GateProperties> &gates);

} // namespace ql

#endif //WITH_VISUALIZER