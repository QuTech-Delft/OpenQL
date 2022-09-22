/** \file
 * Declaration of the visualizer's shared functionalities.
 *
 * --- DONE ---
 * [CIRCUIT] visualization of custom gates
 * [CIRCUIT] option to enable or disable classical bit lines
 * [CIRCUIT] different types of cycle/duration(ns) labels
 * [CIRCUIT] gate duration outlines in gate color
 * [CIRCUIT] measurement without explicitly specified classical operand assumes default classical operand (same number as qubit number)
 * [CIRCUIT] read cycle duration from hardware config file, instead of having hardcoded value
 * [CIRCUIT] handle case where user does not or incorrectly specifies visualization nodes for custom gate
 * [CIRCUIT] allow the user to set the layout parameters from a configuration file
 * [CIRCUIT] implement a generic grid structure object to contain the visual structure of the circuit, to ease positioning of components in all the drawing functions
 * [CIRCUIT] visual_type attribute instead of full visual attribute in hw config file, links to seperate visualization config file where details of that visual type are detailed
 * [CIRCUIT] 'cutting' circuits where nothing/not much is happening both in terms of idle cycles and idle qubits
 * [CIRCUIT] add bit line zigzag indicating a cut cycle range
 * [CIRCUIT] add cutEmptyCycles and emptyCycleThreshold to the documentation
 * [CIRCUIT] make a copy of the gate vector, so any changes inside the visualizer to the program do not reflect back to any future compiler passes
 * [CIRCUIT] add option to display cycle edges
 * [CIRCUIT] add option to draw horizontal lines between qubits
 * [CIRCUIT] representing the gates as waveforms
 * [CIRCUIT] allow for floats in the waveform sample vector
 * [CIRCUIT] re-organize the attributes in the config file
 * [CIRCUIT] change char arrays to Color
 * [CIRCUIT] check for negative/invalid values during layout validation
 * [CIRCUIT] GateProperties validation on construction (test with visualizer pass called at different points (during different passes) during compilation)
 * [GENERAL] update code style
 * [GENERAL] split visualizer.cc into multiple files
 * [GENERAL] split Layout into multiple, one for each visualization type
 * [INTERACTION] add interaction graph layout object and load it from a file
 * [INTERACTION] add number indicating amount of interactions for edges
 * [CIRCUIT] visualize before scheduler has been ran, no duration should be shown, just circuit in user-defined order
 * [GENERAL] add option to save the image and/or open the window
 * [INTERACTION] output dot files for graphing software, default circle graph will also be shown
 * [INTERACTION] calculate angle to target qubit and place amount of interactions label accordingly
 * [MAPPING] add pseudogate containing virtual > real qubit mapping
 * [GENERAL] replace primitives and containers with the ones specified in utils (int, double, bool, vector, map, string, etc.)
 * [GENERAL] fix compilation error due to merge
 * [GENERAL] replace cimg calls with cimg wrapper calls
 * [GENERAL] delete default constructors in position types in visualizer_types.h
 * [GENERAL] fix isEdgeAlreadyDrawn() build error on CI
 * [MAPPING] turn pulse visualization and cycle cutting off for the mapping graph
 * [MAPPING] fix virtual qubit swapping
 *
 * -- IN PROGRESS ---
 * [GENERAL] update documentation
 *
 * --- FUTURE WORK ---
 * [GENERAL] fix size of image when image is too big for screen (not sure how with CImg)
 * [MAPPING] make cycle cutting work with the mapping graph
 * [MAPPING] make pulse visualization work with the mapping graph
 * [INTERACTION] apply background color option to interaction graph too
 * [GENERAL] add generating random circuits for visualization testing
 * [CIRCUIT] add connection line thickness parameter
 * [CIRCUIT] add bitline thickness parameter (maybe)
 * [CIRCUIT] allow collapsing the three qubit lines into one with an option for pulse visualization
 * [CIRCUIT] implement cycle cutting for pulse visualization
 * [CIRCUIT] what happens when a cycle range is cut, but one or more gates still running within that range finish earlier than the longest running gate
 *           comprising the entire range?
 * [CIRCUIT] when measurement connections are not shown, allow overlap of measurement gates
 * [CIRCUIT] when gate is skipped due to whatever reason, show a dummy gate outline with a question mark, indicating where the gate is (maybe)
 * [CIRCUIT] display barriers (need barrier fix first)
 * [CIRCUIT] add classical bit number to measurement connection when classical lines are grouped
 * [CIRCUIT] implement measurement symbol (to replace the M on measurement gates)
 * [CIRCUIT] generate default gate visuals from the hardware configuration file if none are supplied in the instruction section of the visualizer config file
 */

#pragma once

#ifdef WITH_VISUALIZER

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/pair.h"
#include "ql/utils/vec.h"
#include "ql/ir/compat/compat.h"
#include "types.h"

namespace ql {
namespace pass {
namespace ana {
namespace visualize {
namespace detail {

utils::Vec<GateProperties> parseGates(const ir::compat::ProgramRef &program);

utils::Int calculateAmountOfCycles(const utils::Vec<GateProperties> &gates, utils::Int cycleDuration);
utils::Int calculateAmountOfBits(const utils::Vec<GateProperties> &gates, const utils::Vec<utils::Int> GateProperties::* operandType);

utils::Int calculateAmountOfGateOperands(const GateProperties &gate);
utils::Vec<GateOperand> getGateOperands(const GateProperties &gate);
utils::Pair<GateOperand, GateOperand> calculateEdgeOperands(const utils::Vec<GateOperand> &operands, utils::Int amountOfQubits);

void fixMeasurementOperands(utils::Vec<GateProperties> &gates);
utils::Bool isMeasurement(const GateProperties &gate);

void printGates(const utils::Vec<GateProperties> &gates);
void printGatesShort(const utils::Vec<GateProperties> &gates);

} // namespace detail
} // namespace visualize
} // namespace ana
} // namespace pass
} // namespace ql

#endif //WITH_VISUALIZER
