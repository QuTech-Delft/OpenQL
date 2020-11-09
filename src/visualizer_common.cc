/**
 * @file   visualizer_common.cc
 * @date   11/2020
 * @author Tim van der Meer
 * @brief  definition of the visualizer
 */
 
#include "visualizer.h"
#include "visualizer_common.h"
#include "visualizer_circuit.h"
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

// -- IN PROGRESS ---
// [GENERAL] update documentation
// [GENERAL] merge with develop

// --- FUTURE WORK ---
// [GENERAL] split visualizer.cc into multiple files, one for Structure, one for CircuitData and one for free code and new Visualizer class
// [GENERAL] add generating random circuits for visualization testing
// [CIRCUIT] allow collapsing the three qubit lines into one with an option
// [CIRCUIT] implement cycle cutting for pulse visualization
// [CIRCUIT] what happens when a cycle range is cut, but one or more gates still running within that range finish earlier than the longest running gate 
//       comprising the entire range?
// [CIRCUIT] when measurement connections are not shown, allow overlap of measurement gates
// [CIRCUIT] when gate is skipped due to whatever reason, maybe show a dummy gate outline indicating where the gate is?
// [CIRCUIT] display wait/barrier gate (need wait gate fix first)
// [CIRCUIT] add classical bit number to measurement connection when classical lines are grouped
// [CIRCUIT] implement measurement symbol (to replace the M on measurement gates)
// [CIRCUIT] generate default gate visuals from the configuration file
// [GENERAL] add option to save the image and/or open the window

#ifndef WITH_VISUALIZER

void visualize(const ql::quantum_program* program, const VisualizerConfigurationPaths configurationPaths) {
    WOUT("The visualizer is disabled. If this was not intended, the X11 library might be missing and the visualizer has disabled itself.");
}

#else

void visualize(const ql::quantum_program* program, const VisualizerConfigurationPaths configurationPaths) {
    IOUT("Starting visualization...");

    // Parse and validate the layout and instruction configuration file.
    Layout layout = parseConfiguration(configurationPaths.config);
    validateLayout(layout);

    // Get the gate list from the program.
    DOUT("Getting gate list...");
    std::vector<GateProperties> gates = parseGates(program);
    if (gates.size() == 0) {
        FATAL("Quantum program contains no gates!");
    }

    // Calculate circuit properties.
    DOUT("Calculating circuit properties...");
    const int cycleDuration = safe_int_cast(program->platform.cycle_time);
    DOUT("Cycle duration is: " + std::to_string(cycleDuration) + " ns.");
    // Fix measurement gates without classical operands.
    fixMeasurementOperands(gates);

    //TODO: allow user to choose visualization type (circuit, interaction graph, mapping graph)
    visualizeCircuit(gates, layout, cycleDuration, configurationPaths.waveformMapping);

    IOUT("Visualization complete...");
}

Layout parseConfiguration(const std::string &configPath) {
    DOUT("Parsing visualizer configuration file.");

    json config;
    try {
        config = load_json(configPath);
    } catch (json::exception &e) {
        FATAL("Failed to load the visualization config file: \n\t" << std::string(e.what()));
    }

    Layout layout;

    // Fill the layout object with the values from the config file. Any missing values will assume the default values hardcoded in the layout object.

    // -------------------------------------- //
    // -               CYCLES               - //
    // -------------------------------------- //
    if (config.count("cycles") == 1) {
        json cycles = config["cycles"];

        // LABELS
        if (cycles.count("labels") == 1) {
            json labels = cycles["labels"];

            if (labels.count("show") == 1)          layout.cycles.labels.setEnabled(labels["show"]);
            if (labels.count("inNanoSeconds") == 1) layout.cycles.labels.setInNanoSeconds(labels["inNanoSeconds"]);
            if (labels.count("rowHeight") == 1)     layout.cycles.labels.setRowHeight(labels["rowHeight"]);
            if (labels.count("fontHeight") == 1)    layout.cycles.labels.setFontHeight(labels["fontHeight"]);
            if (labels.count("fontColor") == 1)     layout.cycles.labels.setFontColor(labels["fontColor"]);
        }

        // EDGES
        if (cycles.count("edges") == 1) {
            json edges = cycles["edges"];

            if (edges.count("show") == 1)   layout.cycles.edges.setEnabled(edges["show"]);
            if (edges.count("color") == 1)  layout.cycles.edges.setColor(edges["color"]);
            if (edges.count("alpha") == 1)  layout.cycles.edges.setAlpha(edges["alpha"]);
        }

        // CUTTING
        if (cycles.count("cutting") == 1) {
            json cutting = cycles["cutting"];

            if (cutting.count("cut") == 1)                      layout.cycles.cutting.setEnabled(cutting["cut"]);
            if (cutting.count("emptyCycleThreshold") == 1)      layout.cycles.cutting.setEmptyCycleThreshold(cutting["emptyCycleThreshold"]);
            if (cutting.count("cutCycleWidth") == 1)            layout.cycles.cutting.setCutCycleWidth(cutting["cutCycleWidth"]);
            if (cutting.count("cutCycleWidthModifier") == 1)    layout.cycles.cutting.setCutCycleWidthModifier(cutting["cutCycleWidthModifier"]);
        }
        
        if (cycles.count("compress") == 1)                      layout.cycles.setCompressed(cycles["compress"]);
        if (cycles.count("partitionCyclesWithOverlap") == 1)    layout.cycles.setPartitioned(cycles["partitionCyclesWithOverlap"]);
    }

    // -------------------------------------- //
    // -              BIT LINES             - //
    // -------------------------------------- //
    if (config.count("bitLines") == 1)
    {
        json bitLines = config["bitLines"];

        // LABELS
        if (bitLines.count("labels") == 1) {
            json labels = bitLines["labels"];

            if (labels.count("show") == 1)          layout.bitLines.labels.setEnabled(labels["show"]);
            if (labels.count("columnWidth") == 1)   layout.bitLines.labels.setColumnWidth(labels["columnWidth"]);
            if (labels.count("fontHeight") == 1)    layout.bitLines.labels.setFontHeight(labels["fontHeight"]);
            if (labels.count("qbitColor") == 1)     layout.bitLines.labels.setQbitColor(labels["qbitColor"]);
            if (labels.count("cbitColor") == 1)     layout.bitLines.labels.setCbitColor(labels["cbitColor"]);
        }

        // QUANTUM
        if (bitLines.count("quantum") == 1) {
            json quantum = bitLines["quantum"];

            if (quantum.count("color") == 1) layout.bitLines.quantum.setColor(quantum["color"]);
        }

        // CLASSICAL
        if (bitLines.count("classical") == 1) {
            json classical = bitLines["classical"];

            if (classical.count("show") == 1)           layout.bitLines.classical.setEnabled(classical["show"]);
            if (classical.count("group") == 1)          layout.bitLines.classical.setGrouped(classical["group"]);
            if (classical.count("groupedLineGap") == 1) layout.bitLines.classical.setGroupedLineGap(classical["groupedLineGap"]);
            if (classical.count("color") == 1)          layout.bitLines.classical.setColor(classical["color"]);
        }

        // EDGES
        if (bitLines.count("edges") == 1) {
            json edges = bitLines["edges"];

            if (edges.count("show") == 1)       layout.bitLines.edges.setEnabled(edges["show"]);
            if (edges.count("thickness") == 1)  layout.bitLines.edges.setThickness(edges["thickness"]);
            if (edges.count("color") == 1)      layout.bitLines.edges.setColor(edges["color"]);
            if (edges.count("alpha") == 1)      layout.bitLines.edges.setAlpha(edges["alpha"]);
        }
    }

    // -------------------------------------- //
    // -                GRID                - //
    // -------------------------------------- //
    if (config.count("grid") == 1) {
        json grid = config["grid"];

        if (grid.count("cellSize") == 1)    layout.grid.setCellSize(grid["cellSize"]);
        if (grid.count("borderSize") == 1)  layout.grid.setBorderSize(grid["borderSize"]);
    }

    // -------------------------------------- //
    // -       GATE DURATION OUTLINES       - //
    // -------------------------------------- //
    if (config.count("gateDurationOutlines") == 1) {
        json gateDurationOutlines = config["gateDurationOutlines"];

        if (gateDurationOutlines.count("show") == 1)         layout.gateDurationOutlines.setEnabled(gateDurationOutlines["show"]);
        if (gateDurationOutlines.count("gap") == 1)          layout.gateDurationOutlines.setGap(gateDurationOutlines["gap"]);
        if (gateDurationOutlines.count("fillAlpha") == 1)    layout.gateDurationOutlines.setFillAlpha(gateDurationOutlines["fillAlpha"]);
        if (gateDurationOutlines.count("outlineAlpha") == 1) layout.gateDurationOutlines.setOutlineAlpha(gateDurationOutlines["outlineAlpha"]);
        if (gateDurationOutlines.count("outlineColor") == 1) layout.gateDurationOutlines.setOutlineColor(gateDurationOutlines["outlineColor"]);
    }

    // -------------------------------------- //
    // -            MEASUREMENTS            - //
    // -------------------------------------- //
    if (config.count("measurements") == 1) {
        json measurements = config["measurements"];

        if (measurements.count("drawConnection") == 1)  layout.measurements.enableDrawConnection(measurements["drawConnection"]);
        if (measurements.count("lineSpacing") == 1)     layout.measurements.setLineSpacing(measurements["lineSpacing"]);
        if (measurements.count("arrowSize") == 1)       layout.measurements.setArrowSize(measurements["arrowSize"]);
    }

    // -------------------------------------- //
    // -               PULSES               - //
    // -------------------------------------- //
    if (config.count("pulses") == 1) {
        json pulses = config["pulses"];

        if (pulses.count("displayGatesAsPulses") == 1)      layout.pulses.setEnabled(pulses["displayGatesAsPulses"]);
        if (pulses.count("pulseRowHeightMicrowave") == 1)   layout.pulses.setPulseRowHeightMicrowave(pulses["pulseRowHeightMicrowave"]);
        if (pulses.count("pulseRowHeightFlux") == 1)        layout.pulses.setPulseRowHeightFlux(pulses["pulseRowHeightFlux"]);
        if (pulses.count("pulseRowHeightReadout") == 1)     layout.pulses.setPulseRowHeightReadout(pulses["pulseRowHeightReadout"]);
        if (pulses.count("pulseColorMicrowave") == 1)       layout.pulses.setPulseColorMicrowave(pulses["pulseColorMicrowave"]);
        if (pulses.count("pulseColorFlux") == 1)            layout.pulses.setPulseColorFlux(pulses["pulseColorFlux"]);
        if (pulses.count("pulseColorReadout") == 1)         layout.pulses.setPulseColorReadout(pulses["pulseColorReadout"]);
    }

    // Load the custom instruction visualization parameters.
    if (config.count("instructions") == 1) {
        for (const auto &instruction : config["instructions"].items()) {
            try {
                GateVisual gateVisual;
                json content = instruction.value();

                // Load the connection color.
                json connectionColor = content["connectionColor"];
                gateVisual.connectionColor[0] = connectionColor[0];
                gateVisual.connectionColor[1] = connectionColor[1];
                gateVisual.connectionColor[2] = connectionColor[2];
                DOUT("Connection color: [" 
                    << (int)gateVisual.connectionColor[0] << ","
                    << (int)gateVisual.connectionColor[1] << ","
                    << (int)gateVisual.connectionColor[2] << "]");

                // Load the individual nodes.
                json nodes = content["nodes"];
                for (size_t i = 0; i < nodes.size(); i++) {
                    json node = nodes[i];
                    
                    Color fontColor = {node["fontColor"][0], node["fontColor"][1], node["fontColor"][2]};
                    Color backgroundColor = {node["backgroundColor"][0], node["backgroundColor"][1], node["backgroundColor"][2]};
                    Color outlineColor = {node["outlineColor"][0], node["outlineColor"][1], node["outlineColor"][2]};
                    
                    NodeType nodeType;
                    if (node["type"] == "NONE") {
                        nodeType = NONE;
                    } else if (node["type"] == "GATE") {
                        nodeType = GATE;
                    } else if (node["type"] == "CONTROL") {
                        nodeType = CONTROL;
                    } else if (node["type"] == "NOT") {
                        nodeType = NOT;
                    } else if (node["type"] == "CROSS") {
                        nodeType = CROSS;
                    } else {
                        WOUT("Unknown gate display node type! Defaulting to type NONE...");
                        nodeType = NONE;
                    }
                    
                    Node loadedNode = {
                        nodeType,
                        node["radius"],
                        node["displayName"],
                        node["fontHeight"],
                        fontColor,
                        backgroundColor,
                        outlineColor
                    };
                    
                    gateVisual.nodes.push_back(loadedNode);
                    
                    DOUT("[type: " << node["type"] << "] "
                        << "[radius: " << gateVisual.nodes.at(i).radius << "] "
                        << "[displayName: " << gateVisual.nodes.at(i).displayName << "] "
                        << "[fontHeight: " << gateVisual.nodes.at(i).fontHeight << "] "
                        << "[fontColor: "
                            << (int)gateVisual.nodes.at(i).fontColor[0] << ","
                            << (int)gateVisual.nodes.at(i).fontColor[1] << ","
                            << (int)gateVisual.nodes.at(i).fontColor[2] << "] "
                        << "[backgroundColor: "
                            << (int)gateVisual.nodes.at(i).backgroundColor[0] << ","
                            << (int)gateVisual.nodes.at(i).backgroundColor[1] << ","
                            << (int)gateVisual.nodes.at(i).backgroundColor[2] << "] "
                        << "[outlineColor: "
                            << (int)gateVisual.nodes.at(i).outlineColor[0] << ","
                            << (int)gateVisual.nodes.at(i).outlineColor[1] << ","
                            << (int)gateVisual.nodes.at(i).outlineColor[2] << "]");
                }

                layout.customGateVisuals.insert({instruction.key(), gateVisual});
            } catch (json::exception &e) {
                WOUT("Failed to load visualization parameters for instruction: '" << instruction.key()
                    << "' \n\t" << std::string(e.what()));
            }
        }
    } else {
        WOUT("Did not find 'instructions' attribute! The visualizer will try to fall back on default gate visualizations.");
    }

    return layout;
}

void validateLayout(Layout &layout) {
    DOUT("Validating layout...");

    //TODO: add more validation
    
    if (layout.cycles.cutting.getEmptyCycleThreshold() < 1) {
        WOUT("Adjusting 'emptyCycleThreshold' to minimum value of 1. Value in configuration file is set to "
            << layout.cycles.cutting.getEmptyCycleThreshold() << ".");
        layout.cycles.cutting.setEmptyCycleThreshold(1);
    }

    if (layout.pulses.areEnabled()) {
        if (layout.bitLines.classical.isEnabled()) {
            WOUT("Adjusting 'showClassicalLines' to false. Unable to show classical lines when 'displayGatesAsPulses' is true!");
            layout.bitLines.classical.setEnabled(false);
        }
        if (layout.cycles.arePartitioned()) {
            WOUT("Adjusting 'partitionCyclesWithOverlap' to false. It is unnecessary to partition cycles when 'displayGatesAsPulses' is true!");
            layout.cycles.setPartitioned(false);
        }
        if (layout.cycles.areCompressed()) {
            WOUT("Adjusting 'compressCycles' to false. Cannot compress cycles when 'displayGatesAsPulses' is true!");
            layout.cycles.setCompressed(false);
        }	
    }

    if (!layout.bitLines.labels.areEnabled())   layout.bitLines.labels.setColumnWidth(0);
    if (!layout.cycles.labels.areEnabled())     layout.cycles.labels.setRowHeight(0);
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

Dimensions calculateTextDimensions(const std::string &text, const int fontHeight, const Layout layout) {
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