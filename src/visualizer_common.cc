/** \file
 * Declaration of the visualizer's shared functionalities.
 */
 
#include "visualizer.h"
#include "visualizer_types.h"
#include "visualizer_common.h"
#include "visualizer_circuit.h"
#include "visualizer_interaction.h"
#include "visualizer_cimg.h"
#include "options.h"
#include "utils/str.h"
#include "utils/json.h"
#include "utils/vec.h"
#include "utils/pair.h"

#include <iostream>

namespace ql {

using namespace utils;

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
// [MAPPING] add pseudogate containing virtual > real qubit mapping
// [GENERAL] replace primitives and containers with the ones specified in utils (int, double, bool, vector, map, string, etc.)
// [GENERAL] fix compilation error due to merge
// [GENERAL] replace cimg calls with cimg wrapper calls
// [GENERAL] delete default constructors in position types in visualizer_types.h

// -- IN PROGRESS ---
// [GENERAL] fix isEdgeAlreadyDrawn() build error on CI
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

void visualize(const quantum_program* program, const Str &visualizationType, const VisualizerConfiguration &configuration) {
    QL_WOUT("The visualizer is disabled. If this was not intended, and OpenQL is running on Linux or Mac, the X11 library "
         << "might be missing and the visualizer has disabled itself.");
}

#else

void visualize(const quantum_program* program, const Str &visualizationType, const VisualizerConfiguration &configuration) {
    QL_IOUT("Starting visualization...");
    QL_IOUT("Visualization type: " << visualizationType);

    // for (ql::quantum_kernel kernel : program->kernels) {
    //     for (ql::gate* const gate : kernel.get_circuit()) {
    //         if (gate->type() == __remap_gate__) {
    //             const ql::remap *remap_p = dynamic_cast<ql::remap*>(gate);
    //             IOUT("remap gate: [" << remap_p->operands[0] << ", " << remap_p->virtual_qubit_index << "]");
    //         }
    //     }
    // }

    // printGates(parseGates(program));

    // Choose the proper visualization based on the visualization type.
    if (visualizationType == "CIRCUIT") {
        visualizeCircuit(program, configuration);
    } else if (visualizationType == "INTERACTION_GRAPH") {
        visualizeInteractionGraph(program, configuration);
    } else if (visualizationType == "MAPPING_GRAPH") {
        QL_WOUT("Mapping graph visualization not yet implemented.");
    } else {
        QL_FATAL("Unknown visualization type: " << visualizationType << "!");
    }

    QL_IOUT("Visualization complete...");
}

Vec<GateProperties> parseGates(const quantum_program* program) {
    Vec<GateProperties> gates;

    for (quantum_kernel kernel : program->kernels) {
        for (gate* const gate : kernel.get_circuit()) {
            Vec<Int> operands;
            Vec<Int> creg_operands;
            for (const UInt operand : gate->operands) { operands.push_back(utoi(operand)); }
            for (const UInt operand : gate->creg_operands) { creg_operands.push_back(utoi(operand)); }
            GateProperties gateProperties {
                gate->name,
                operands,
                creg_operands,
                utoi(gate->duration),
                utoi(gate->cycle),
                gate->type(),
                {},
                "UNDEFINED",
                0
                // gate->type() == __remap_gate__ ? dynamic_cast<ql::remap*>(gate)->virtual_qubit_index : MAX
            };
            gates.push_back(gateProperties);
        }
    }

    return gates;
}

Int calculateAmountOfBits(const Vec<GateProperties> &gates, const Vec<Int> GateProperties::* operandType) {
    QL_DOUT("Calculating amount of bits...");

    //TODO: handle circuits not starting at a c- or qbit with index 0
    Int minAmount = MAX;
    Int maxAmount = 0;

    // Find the minimum and maximum index of the operands.
    for (const GateProperties &gate : gates)
    {
        Vec<Int>::const_iterator begin = (gate.*operandType).begin();
        const Vec<Int>::const_iterator end = (gate.*operandType).end();

        for (; begin != end; ++begin) {
            const Int number = *begin;
            if (number < minAmount) minAmount = number;
            if (number > maxAmount) maxAmount = number;
        }
    }

    // If both minAmount and maxAmount are at their original values, the list of 
    // operands for all the gates was empty.This means there are no operands of 
    // the given type for these gates and we return 0.
    if (minAmount == MAX && maxAmount == 0) {
        return 0;
    } else {
        return 1 + maxAmount - minAmount; // +1 because: max - min = #qubits - 1
    }
}

Int calculateAmountOfGateOperands(const GateProperties &gate) {
    return utoi(gate.operands.size() + gate.creg_operands.size());
}

Vec<GateOperand> getGateOperands(const GateProperties &gate) {
    Vec<GateOperand> operands;

    for (const Int operand : gate.operands)      { operands.push_back({QUANTUM, operand});   }
    for (const Int operand : gate.creg_operands) { operands.push_back({CLASSICAL, operand}); }

    return operands;
}

Pair<GateOperand, GateOperand> calculateEdgeOperands(const Vec<GateOperand> &operands, const Int amountOfQubits) {
    if (operands.size() < 2) {
        QL_FATAL("Gate operands vector does not have multiple operands!");
    }

    GateOperand minOperand = operands[0];
    GateOperand maxOperand = operands[operands.size() - 1];
    for (const GateOperand &operand : operands) {
        const Int row = (operand.bitType == QUANTUM) ? operand.index : operand.index + amountOfQubits;
        if (row < minOperand.index) minOperand = operand;
        if (row > maxOperand.index) maxOperand = operand;
    }

    return {minOperand, maxOperand};
}

void fixMeasurementOperands(Vec<GateProperties> &gates) {
    QL_DOUT("Fixing measurement gates with no classical operand...");

    for (GateProperties &gate : gates) {
        // Check for a measurement gate without explicitly specified classical
        // operand.
        if (isMeasurement(gate)) {
            if (calculateAmountOfGateOperands(gate) == 1) {
                // Set classical measurement operand to the bit corresponding to
                // the measurements qubit index.
                QL_DOUT("Found measurement gate with no classical operand. Assuming default classical operand.");
                const Int cbit = gate.operands[0];
                gate.creg_operands.push_back(cbit);
            }
        }
    }
}

Bool isMeasurement(const GateProperties &gate) {
    //TODO: this method of checking for measurements is not robust and relies
    //      entirely on the user naming their instructions in a certain way!
    return (gate.name.find("measure") != Str::npos);
}

void printGates(const Vec<GateProperties> &gates) {
    for (const GateProperties &gate : gates) {
        QL_IOUT(gate.name);

        Str operands = "[";
        for (UInt i = 0; i < gate.operands.size(); i++) {
            operands += to_string(gate.operands[i]);
            if (i != gate.operands.size() - 1) operands += ", ";
        }
        QL_IOUT("\toperands: " << operands << "]");

        Str creg_operands = "[";
        for (UInt i = 0; i < gate.creg_operands.size(); i++) {
            creg_operands += to_string(gate.creg_operands[i]);
            if (i != gate.creg_operands.size() - 1) creg_operands += ", ";
        }
        QL_IOUT("\tcreg_operands: " << creg_operands << "]");

        QL_IOUT("\tduration: " << gate.duration);
        QL_IOUT("\tcycle: " << gate.cycle);
        QL_IOUT("\ttype: " << gate.type);

        Str codewords = "[";
        for (UInt i = 0; i < gate.codewords.size(); i++) {
            codewords += to_string(gate.codewords[i]);
            if (i != gate.codewords.size() - 1) codewords += ", ";
        }
        QL_IOUT("\tcodewords: " << codewords << "]");

        QL_IOUT("\tvisual_type: " << gate.visual_type);

        // if (gate.type == __remap_gate__) {
        //     QL_IOUT("\tvirtual_qubit_index: " << gate.virtual_qubit_index);
        // }
    }
}

Str generateFilePath(const Str &filename, const Str &extension) {
    return options::get("output_dir") + "/" + filename + "." + extension;
}

void assertPositive(const Int parameterValue, const Str &parameterName) {
    if (parameterValue < 0) QL_FATAL(parameterName << " is negative. Only positive values are allowed!");
}

void assertPositive(const Real parameterValue, const Str &parameterName) {
    if (parameterValue < 0) QL_FATAL(parameterName << " is negative. Only positive values are allowed!");
}

#endif //WITH_VISUALIZER

} // namespace ql