/**
 * @file   visualizer_common.cc
 * @date   11/2020
 * @author Tim van der Meer
 * @brief  definition of the visualizer
 */
 
#include "visualizer.h"
#include "visualizer_types.h"
#include "visualizer_common.h"
#include "visualizer_circuit.h"
#include "visualizer_interaction.h"
#include "options.h"
#include "json.h"

#include <iostream>
#include <limits>

using json = nlohmann::json;

namespace ql {

// --- DONE ---
// [CIRCUIT] visualization of custom gates
// [CIRCUIT] option to enable or disable classical bit lines
// [CIRCUIT] different types of cycle/duration(ns) labels
// [CIRCUIT] gate duration outlines in gate color
// [CIRCUIT] measurement without explicitly specified classical operand assumes default classical operand (same number as qubit number)
// [CIRCUIT] read cycle duration from hardware config file, instead of having hardcoded value
// [CIRCUIT] handle case where user does not or incorrectly specifies visualization nodes for custom gate
// [CIRCUIT] allow the user to set the layout parameters from a configuration file
// [CIRCUIT] implement a generic grid structure object to contain the visual structure of the circuit, to ease positioning of components in all the drawing functions
// [CIRCUIT] visual_type attribute instead of full visual attribute in hw config file, links to seperate visualization config file where details of that visual type are detailed
// [CIRCUIT] 'cutting' circuits where nothing/not much is happening both in terms of idle cycles and idle qubits
// [CIRCUIT] add bit line zigzag indicating a cut cycle range
// [CIRCUIT] add cutEmptyCycles and emptyCycleThreshold to the documentation
// [CIRCUIT] make a copy of the gate vector, so any changes inside the visualizer to the program do not reflect back to any future compiler passes
// [CIRCUIT] add option to display cycle edges
// [CIRCUIT] add option to draw horizontal lines between qubits
// [CIRCUIT] representing the gates as waveforms
// [CIRCUIT] allow for floats in the waveform sample vector
// [CIRCUIT] re-organize the attributes in the config file
// [CIRCUIT] change char arrays to Color
// [CIRCUIT] check for negative/invalid values during layout validation
// [CIRCUIT] GateProperties validation on construction (test with visualizer pass called at different points (during different passes) during compilation)
// [GENERAL] update code style
// [GENERAL] split visualizer.cc into multiple files
// [GENERAL] split Layout into multiple, one for each visualization type
// [INTERACTION] add interaction graph layout object and load it from a file
// [INTERACTION] add number indicating amount of interactions for edges
// [CIRCUIT] visualize before scheduler has been ran, no duration should be shown, just circuit in user-defined order
// [GENERAL] add option to save the image and/or open the window
// [INTERACTION] output dot files for graphing software, default circle graph will also be shown
// [INTERACTION] calculate angle to target qubit and place amount of interactions label accordingly

// -- IN PROGRESS ---
// [MAPPING] add pseudogate containing virtual > real qubit mapping
// [GENERAL] update documentation

// --- FUTURE WORK ---
// [GENERAL] add generating random circuits for visualization testing
// [CIRCUIT] add background color option
// [CIRCUIT] add connection line thickness parameter
// [CIRCUIT] add bitline thickness parameter (maybe)
// [CIRCUIT] allow collapsing the three qubit lines into one with an option
// [CIRCUIT] implement cycle cutting for pulse visualization
// [CIRCUIT] what happens when a cycle range is cut, but one or more gates still running within that range finish earlier than the longest running gate 
//           comprising the entire range?
// [CIRCUIT] when measurement connections are not shown, allow overlap of measurement gates
// [CIRCUIT] when gate is skipped due to whatever reason, show a dummy gate outline with a question mark, indicating where the gate is (maybe)
// [CIRCUIT] display barriers (need barrier fix first)
// [CIRCUIT] add classical bit number to measurement connection when classical lines are grouped
// [CIRCUIT] implement measurement symbol (to replace the M on measurement gates)
// [CIRCUIT] generate default gate visuals from the hardware configuration file if none are supplied in the instruction section of the visualizer config file

#ifndef WITH_VISUALIZER

void visualize(const ql::quantum_program* program, const std::string &visualizationType, const VisualizerConfigurationPaths configurationPaths) {
    WOUT("The visualizer is disabled. If this was not intended, and OpenQL is running on Linux or Mac, the X11 library "
        << "might be missing and the visualizer has disabled itself.");
}

#else

void visualize(const ql::quantum_program* program, const std::string &visualizationType, const VisualizerConfigurationPaths configurationPaths) {
    IOUT("Starting visualization...");
    IOUT("Visualization type: " << visualizationType);

    printGates(parseGates(program));

    // // Choose the proper visualization based on the visualization type.
    // if (visualizationType == "CIRCUIT") {
    //     visualizeCircuit(program, configurationPaths);
    // } else if (visualizationType == "INTERACTION_GRAPH") {
    //     visualizeInteractionGraph(program, configurationPaths);
    // } else if (visualizationType == "MAPPING_GRAPH") {
    //     WOUT("Mapping graph visualization not yet implemented.");
    // } else {
    //     FATAL("Unknown visualization type: " << visualizationType << "!");
    // }

    IOUT("Visualization complete...");
}

std::vector<GateProperties> parseGates(const ql::quantum_program* program) {
    std::vector<GateProperties> gates;

    for (ql::quantum_kernel kernel : program->kernels) {
        for (ql::gate* const gate : kernel.get_circuit()) {
            std::vector<int> codewords;
            if (gate->type() == __custom_gate__) {
                for (const size_t codeword : dynamic_cast<ql::custom_gate*>(gate)->codewords) {
                    codewords.push_back(safe_int_cast(codeword));
                }
            }

            std::vector<int> operands;
            std::vector<int> creg_operands;
            for (const size_t operand : gate->operands) { operands.push_back(safe_int_cast(operand)); }
            for (const size_t operand : gate->creg_operands) { creg_operands.push_back(safe_int_cast(operand)); }
            GateProperties gateProperties {
                gate->name,
                operands,
                creg_operands,
                safe_int_cast(gate->duration),
                safe_int_cast(gate->cycle),
                gate->type(),
                codewords,
                gate->visual_type
            };
            gates.push_back(gateProperties);
        }
    }

    return gates;
}

int calculateAmountOfBits(const std::vector<GateProperties> gates, const std::vector<int> GateProperties::* operandType) {
    DOUT("Calculating amount of bits...");

    //TODO: handle circuits not starting at a c- or qbit with index 0
    int minAmount = std::numeric_limits<int>::max();
    int maxAmount = 0;

    // Find the minimum and maximum index of the operands.
    for (const GateProperties &gate : gates)
    {
        std::vector<int>::const_iterator begin = (gate.*operandType).begin();
        const std::vector<int>::const_iterator end = (gate.*operandType).end();

        for (; begin != end; ++begin) {
            const int number = *begin;
            if (number < minAmount) minAmount = number;
            if (number > maxAmount) maxAmount = number;
        }
    }

    // If both minAmount and maxAmount are at their original values, the list of 
    // operands for all the gates was empty.This means there are no operands of 
    // the given type for these gates and we return 0.
    if (minAmount == std::numeric_limits<int>::max() && maxAmount == 0) {
        return 0;
    } else {
        return 1 + maxAmount - minAmount; // +1 because: max - min = #qubits - 1
    }
}

int calculateAmountOfGateOperands(const GateProperties gate) {
    return safe_int_cast(gate.operands.size() + gate.creg_operands.size());
}

std::vector<GateOperand> getGateOperands(const GateProperties gate) {
    std::vector<GateOperand> operands;

    for (const int operand : gate.operands)      { operands.push_back({QUANTUM, operand});   }
    for (const int operand : gate.creg_operands) { operands.push_back({CLASSICAL, operand}); }

    return operands;
}

std::pair<GateOperand, GateOperand> calculateEdgeOperands(const std::vector<GateOperand> operands, const int amountOfQubits) {
    if (operands.size() < 2) {
        FATAL("Gate operands vector does not have multiple operands!");
    }

    GateOperand minOperand = operands[0];
    GateOperand maxOperand = operands[operands.size() - 1];
    for (const GateOperand &operand : operands) {
        const int row = (operand.bitType == QUANTUM) ? operand.index : operand.index + amountOfQubits;
        if (row < minOperand.index) minOperand = operand;
        if (row > maxOperand.index) maxOperand = operand;
    }

    return {minOperand, maxOperand};
}

void fixMeasurementOperands(std::vector<GateProperties> &gates) {
    DOUT("Fixing measurement gates with no classical operand...");

    for (GateProperties &gate : gates) {
        // Check for a measurement gate without explicitly specified classical
        // operand.
        if (isMeasurement(gate)) {
            if (calculateAmountOfGateOperands(gate) == 1) {
                // Set classical measurement operand to the bit corresponding to
                // the measurements qubit index.
                DOUT("Found measurement gate with no classical operand. Assuming default classical operand.");
                const int cbit = gate.operands[0];
                gate.creg_operands.push_back(cbit);
            }
        }
    }
}

bool isMeasurement(const GateProperties gate) {
    //TODO: this method of checking for measurements is not robust and relies
    //      entirely on the user naming their instructions in a certain way!
    return (gate.name.find("measure") != std::string::npos);
}

Dimensions calculateTextDimensions(const std::string &text, const int fontHeight) {
    const char* chars = text.c_str();
    cimg_library::CImg<unsigned char> imageTextDimensions;
    const char color = 1;
    imageTextDimensions.draw_text(0, 0, chars, &color, 0, 1, fontHeight);

    return Dimensions { imageTextDimensions.width(), imageTextDimensions.height() };
}

void printGates(const std::vector<GateProperties> gates) {
    for (const GateProperties &gate : gates) {
        IOUT(gate.name);

        std::string operands = "[";
        for (size_t i = 0; i < gate.operands.size(); i++) {
            operands += std::to_string(gate.operands[i]);
            if (i != gate.operands.size() - 1) operands += ", ";
        }
        IOUT("\toperands: " << operands << "]");

        std::string creg_operands = "[";
        for (size_t i = 0; i < gate.creg_operands.size(); i++) {
            creg_operands += std::to_string(gate.creg_operands[i]);
            if (i != gate.creg_operands.size() - 1) creg_operands += ", ";
        }
        IOUT("\tcreg_operands: " << creg_operands << "]");

        IOUT("\tduration: " << gate.duration);
        IOUT("\tcycle: " << gate.cycle);
        IOUT("\ttype: " << gate.type);

        std::string codewords = "[";
        for (size_t i = 0; i < gate.codewords.size(); i++) {
            codewords += std::to_string(gate.codewords[i]);
            if (i != gate.codewords.size() - 1) codewords += ", ";
        }
        IOUT("\tcodewords: " << codewords << "]");

        IOUT("\tvisual_type: " << gate.visual_type);
    }
}

std::string generateFilePath(const std::string &filename, const std::string &extension) {
    return ql::options::get("output_dir") + "/" + filename + "." + extension;
}

int safe_int_cast(const size_t argument) {
    if (argument > std::numeric_limits<int>::max()) FATAL("Failed cast to int: size_t argument is too large!");
    return static_cast<int>(argument);
}

void assertPositive(const int argument, const std::string &parameter) {
    if (argument < 0) FATAL(parameter << " is negative. Only positive values are allowed!");
}

void assertPositive(const double argument, const std::string &parameter) {
    if (argument < 0) FATAL(parameter << " is negative. Only positive values are allowed!");
}

#endif //WITH_VISUALIZER

} // namespace ql