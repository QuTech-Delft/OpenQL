/**
 * @file   visualizer_common.h
 * @date   11/2020
 * @author Tim van der Meer
 * @brief  declaration of the visualizer's shared functionalities
 */

#pragma once

#ifdef WITH_VISUALIZER

#include "visualizer.h"
#include "visualizer_types.h"

// These undefs are necessary to avoid name collisions between CImg and Lemon.
#undef cimg_use_opencv
#undef True

#undef False
#undef IN
#undef OUT

namespace ql {

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