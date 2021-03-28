/** \file
 * Declaration of the visualizer's shared functionalities.
 */

#pragma once

// FIXME JvS: WITH_VISUALIZER must never appear in a public header file
#ifdef WITH_VISUALIZER

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/pair.h"
#include "ql/utils/vec.h"
#include "program.h"
#include "ql/pass/ana/visualize/types.h"

namespace ql {
namespace pass {
namespace ana {
namespace visualize {

utils::Vec<GateProperties> parseGates(const quantum_program* program);

utils::Int calculateAmountOfCycles(const utils::Vec<GateProperties> &gates, const utils::Int cycleDuration);
utils::Int calculateAmountOfBits(const utils::Vec<GateProperties> &gates, const utils::Vec<utils::Int> GateProperties::* operandType);

utils::Int calculateAmountOfGateOperands(const GateProperties &gate);
utils::Vec<GateOperand> getGateOperands(const GateProperties &gate);
utils::Pair<GateOperand, GateOperand> calculateEdgeOperands(const utils::Vec<GateOperand> &operands, const utils::Int amountOfQubits);

void fixMeasurementOperands(utils::Vec<GateProperties> &gates);
utils::Bool isMeasurement(const GateProperties &gate);

utils::Str generateFilePath(const utils::Str &filename, const utils::Str &extension);

void printGates(const utils::Vec<GateProperties> &gates);
void printGatesShort(const utils::Vec<GateProperties> &gates);

} // namespace visualize
} // namespace ana
} // namespace pass
} // namespace ql

#endif //WITH_VISUALIZER
