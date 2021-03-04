/** \file
 * Definition of the visualizer mapping graph.
 */

#ifdef WITH_VISUALIZER

#include "visualizer.h"
#include "visualizer_types.h"
#include "visualizer_common.h"
#include "visualizer_cimg.h"
#include "visualizer_circuit.h"
#include "visualizer_mapping.h"
#include "utils/json.h"
#include "utils/num.h"
#include "utils/vec.h"

namespace ql {

using namespace utils;

void visualizeMappingGraph(const quantum_program* program, const VisualizerConfiguration &configuration) {
    QL_IOUT("Visualizing mapping graph...");

    // Parse the layout and gate vector.
    const MappingGraphLayout layout = parseMappingGraphLayout(configuration.visualizerConfigPath);
    const Vec<GateProperties> gates = parseGates(program);

    // Get visualized circuit with extra wide cycles from visualizer_circuit.cc.
    const Int amountOfQubits = calculateAmountOfBits(gates, &GateProperties::operands);
    const Int amountOfLines = ceil(sqrt(amountOfQubits));
    const Int qubitDiameter = layout.getQubitRadius() * 2;
    const Int minCycleWidth = amountOfLines * qubitDiameter + (amountOfLines + 1) * layout.getQubitSpacing();
    const Int extendedImageHeight = minCycleWidth + layout.getBorderSize();
    ImageOutput imageOutput = generateImage(program, configuration, minCycleWidth, extendedImageHeight);

    // Fill in cycle spaces beneath the circuit with the mapping graph.
    const Int yStart = imageOutput.structure.getImageHeight() - extendedImageHeight;
    const Int yEnd = imageOutput.structure.getImageHeight();

    // Calculate the virtual qubits mapping for each cycle.
    const Int amountOfCycles = imageOutput.circuitData.getAmountOfCycles();
    Vec<Vec<Int>> virtualQubits(amountOfCycles);
    if (amountOfCycles <= 0) {
        QL_FATAL("Circuit contains no cycles! Cannot visualize mapping graph.");
    }

    // printGates(gates);

    // Initialize the first cycle with a virtual index = real index mapping.
    for (Int qubitIndex = 0; qubitIndex < amountOfQubits; qubitIndex++) {
        virtualQubits[0].push_back(qubitIndex);
    }
    // Each other cycle either gets a new virtual operand from a gate in that cycle, or carries over the previous
    // cycle's virtual operand for that qubit.
    for (Int cycleIndex = 1; cycleIndex < amountOfCycles; cycleIndex++) {
        // Copy virtual qubit operand from the previous cycle of the same qubit.
        for (Int qubitIndex = 0; qubitIndex < amountOfQubits; qubitIndex++) {
            virtualQubits[cycleIndex].push_back(virtualQubits[cycleIndex - 1][qubitIndex]);
        }
        // Update the virtual qubit operands from the gates in this cycle.
        for (Int qubitIndex = 0; qubitIndex < amountOfQubits; qubitIndex++) {
            // Check for changes in virtual operand for this qubit in the gates for this cycle.
            for (const GateProperties &gate : gates) {
                // Check if the gate's cycle matches the current cycle.
                if (gate.cycle == cycleIndex) {
                    const Vec<Int> virtualOperands = gate.virtual_operands;
                    const Vec<Int> realOperands = gate.operands;
                    if (virtualOperands.size() != realOperands.size()) {
                        continue;
                        // QL_FATAL("Size of virtual operands vector does match size of real operands vector!");
                    }
                    // Swap the virtual and real qubits.
                    for (Int operandIndex = 0; operandIndex < realOperands.size(); operandIndex++) {
                        const Int virtualQubit = virtualOperands[operandIndex];
                        const Int realQubit = realOperands[operandIndex];
                        virtualQubits[cycleIndex][realQubit] = virtualQubit;
                        virtualQubits[cycleIndex][virtualQubit] = realQubit;
                    }
                }
            }
        }
    }

    // Vec<Vec<Int>> virtualQubits(imageOutput.circuitData.getAmountOfCycles());
    // for (Vec<Int> &qubitsInCycle : virtualQubits) {
    //     for (Int i = 0; i < amountOfQubits; i++) {
    //         qubitsInCycle.push_back(-1);
    //     }
    // }

    // for (const Vec<Int> &qubitsInCycle : virtualQubits) {
    //     for (Int qubitIndex = 0; qubitIndex < qubitsInCycle.size(); qubitIndex++) {
    //         QL_IOUT("\tqubit index: " << qubitIndex << " virtual qubit: " << qubitsInCycle);
    //     }
    // }

    // printGates(gates);

    // QL_IOUT("Storing virtual operands...");

    // for (const GateProperties &gate : gates) {
    //     const Int cycleIndex = gate.cycle;
    //     const Vec<Int> virtualOperands = gate.virtual_operands;
    //     const Vec<Int> operands = gate.operands;
    //     QL_IOUT("cycleIndex: " << cycleIndex);
    //     QL_IOUT("virtualOperands.size(): " << virtualOperands.size());
    //     QL_IOUT("operands.size(): " << operands.size());
    //     for (Int i = 0; i < operands.size(); i++) {
    //         QL_IOUT("\ti: " << i);
    //         QL_IOUT("\toperands[i]: " << operands[i]);
    //         QL_IOUT("\tvirtualOperands[i]: " << virtualOperands[i]);
    //         virtualQubits[cycleIndex][operands[i]] = virtualOperands[i];
    //     }
    // }

    // for (const Vec<Int> &qubitsInCycle : virtualQubits) {
    //     for (Int qubitIndex = 0; qubitIndex < qubitsInCycle.size(); qubitIndex++) {
    //         QL_IOUT("\tqubit index: " << qubitIndex << " virtual qubit: " << qubitsInCycle);
    //     }
    // }

    // Draw the mapping for each cycle.
    for (Int cycleIndex = 0; cycleIndex < imageOutput.circuitData.getAmountOfCycles(); cycleIndex++) {
        // Vec<Int> virtualQubits(amountOfQubits);
        // const Cycle cycle = imageOutput.circuitData.getCycle(cycleIndex);

        // for (const Vec<std::reference_wrapper<GateProperties>> partition : cycle.gates) {
        //     for (const GateProperties gate : partition) {
        //         // QL_IOUT(gate.virtual_operands.size());
        //         // for (const Int vop : gate.virtual_operands) {
        //         //     QL_IOUT(vop);
        //         // }
        //         Vec<Int> virtualOperands = gate.virtual_operands;
        //         Vec<Int> operands = gate.operands;
        //         for (Int i = 0; i < operands.size(); i++) {
        //             virtualQubits[operands[i]] = virtualOperands[i];
        //         }
        //     }
        // }

        // QL_IOUT("cycle: " << cycleIndex);
        // for (Int i = 0; i < virtualQubits.size(); i++) {
        //     QL_IOUT("\t\treal: " << i << " --> virtual: " << virtualQubits[i]);
        // }

        const Position4 position = imageOutput.structure.getCellPosition(cycleIndex, 0, QUANTUM);
        const Int xStart = position.x0;
        const Int xEnd = position.x1;

        // Draw each of the qubit mappings in this cycle.
        for (Int qubitIndex = 0; qubitIndex < amountOfQubits; qubitIndex++) {
            // Draw qubit circle.
            const Int column = qubitIndex % amountOfLines;
            const Int row = qubitIndex / amountOfLines;
            const Int centerX = xStart + column * qubitDiameter + (column + 1) * layout.getQubitSpacing() + layout.getQubitRadius();
            const Int centerY = yStart + row * qubitDiameter + (row + 1) * layout.getQubitSpacing() + layout.getQubitRadius();
            imageOutput.image.drawOutlinedCircle(centerX, centerY, layout.getQubitRadius(), black, 1.0f, LinePattern::UNBROKEN);

            // Draw virtual operand label on qubit.
            const Int virtualOperand = virtualQubits[cycleIndex][qubitIndex];
            const Str text = utils::to_string(virtualOperand);

            //TODO: load from config
            const Int fontHeight = 13;
            const Color textColor = black;

            const Dimensions dimensions = calculateTextDimensions(text, fontHeight);
            const Int textX = centerX - dimensions.width / 2;
            const Int textY = centerY - dimensions.height / 2;
            imageOutput.image.drawText(textX, textY, text, fontHeight, textColor);
        }
    }

    // Display the filled in image.
    imageOutput.image.display("Mapping Graph");
}

MappingGraphLayout parseMappingGraphLayout(const Str &configPath) {
    QL_DOUT("Parsing visualizer configuration file for mapping graph visualization...");

    Json fullConfig;
    try {
        fullConfig = load_json(configPath);
    } catch (Json::exception &e) {
        QL_FATAL("Failed to load the visualization config file: \n\t" << std::string(e.what()));
    }

    Json config;
    if (fullConfig.count("mappingGraph") == 1) {
        config = fullConfig["mappingGraph"];
    } else {
        QL_WOUT("Could not find mapping graph configuration in visualizer configuration file. Is it named correctly?");
    }

    Json gridConfig;
    if (fullConfig.count("circuit") == 1) {
        Json circuitConfig = fullConfig["circuit"];
        if (circuitConfig.count("grid") == 1) {
            gridConfig = circuitConfig["grid"];
        } else {
            QL_WOUT("Could not find grid configuration in visualizer configuration file. Is it named correctly?");
        }
    }

    // Fill the layout object with the values from the config file. Any missing values will assume the default values hardcoded in the layout object.
    MappingGraphLayout layout;

    // Check if the image should be saved to disk.
    if (fullConfig.count("saveImage") == 1) {
        layout.saveImage = fullConfig["saveImage"];
    }

    // Load the parameters.
    if (config.count("qubitRadius") == 1)    layout.setQubitRadius(config["qubitRadius"]);
    if (config.count("qubitSpacing") == 1)   layout.setQubitSpacing(config["qubitSpacing"]);
    if (gridConfig.count("borderSize") == 1) layout.setBorderSize(gridConfig["borderSize"]);

    return layout;
}

} // namespace ql

#endif //WITH_VISUALIZER