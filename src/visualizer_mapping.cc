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

    // Parse the topology if it exists in the platform configuration file.
    Topology topology;
    const Bool parsedTopology = layout.getUseTopology() ? parseTopology(program->platform.topology, topology) : false;
    if (parsedTopology) {
        QL_DOUT("Succesfully parsed topology.");
        QL_DOUT("xSize: " << topology.xSize);
        QL_DOUT("ySize: " << topology.ySize);
        QL_DOUT("qubits:");
        for (Int qubitIndex = 0; qubitIndex < topology.vertices.size(); qubitIndex++) {
            QL_DOUT("\tid: " << qubitIndex << " position: [" << topology.vertices[qubitIndex].x << ", " << topology.vertices[qubitIndex].y << "]");
        }
        QL_DOUT("edges:");
        for (const Edge edge : topology.edges) {
            QL_DOUT("\tsrc: " << edge.src << ", dst: " << edge.dst);
        }
    } else {
        QL_IOUT("Topology not parsed. Falling back on basic visualization.");
    }

    // printGates(gates);

    // Get visualized circuit with extra wide cycles from visualizer_circuit.cc.
    const Int amountOfQubits = calculateAmountOfBits(gates, &GateProperties::operands);
    const Int amountOfColumns = parsedTopology ? topology.xSize : ceil(sqrt(amountOfQubits));
    const Int amountOfRows = parsedTopology ? topology.ySize : ceil(sqrt(amountOfQubits));
    const Int qubitDiameter = layout.getQubitRadius() * 2;
    const Int columnWidth = qubitDiameter;
    const Int rowHeight = qubitDiameter + (layout.getShowRealIndices() ? layout.getFontHeightReal() + layout.getRealIndexSpacing() * 2 : 0);
    const Int minCycleWidth = amountOfColumns * columnWidth + (amountOfColumns + 1) * layout.getQubitSpacing();
    const Int extendedImageHeight = amountOfRows * rowHeight + (amountOfRows + 1) * layout.getQubitSpacing() + layout.getBorderSize();

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

    // Initialize the first cycle with a virtual index = real index mapping.
    for (Int qubitIndex = 0; qubitIndex < amountOfQubits; qubitIndex++) {
        const Int virtualIndex = layout.getInitDefaultVirtuals() ? qubitIndex : -1;
        virtualQubits[0].push_back(virtualIndex);
    }
    // Each other cycle either gets a new virtual operand from a gate in that cycle, or carries over the previous
    // cycle's virtual operand for that qubit.
    for (Int cycleIndex = 1; cycleIndex < amountOfCycles; cycleIndex++) {
        // Copy virtual qubit operand from the previous cycle of the same qubit.
        for (Int qubitIndex = 0; qubitIndex < amountOfQubits; qubitIndex++) {
            virtualQubits[cycleIndex].push_back(virtualQubits[cycleIndex - 1][qubitIndex]);
        }
        // Update the virtual qubit operands from the gates in this cycle.
        for (const GateProperties &gate : gates) {
            // Check if the gate's cycle matches the current cycle.
            if (gate.cycle == cycleIndex) {
                const Vec<Int> virtualOperands = gate.virtual_operands;
                const Vec<Int> realOperands = gate.operands;
                if (virtualOperands.size() != realOperands.size()) {
                    QL_DOUT("Size of virtual operands vector does not match size of real operands vector, skipping gate.");
                    continue;
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

    // Load the fill colors for virtual qubits.
    Vec<Color> virtualColors(amountOfQubits);
    const Int division = 255 / amountOfQubits;
    for (Int qubitIndex = 0; qubitIndex < amountOfQubits; qubitIndex++) {
        const Int currentColor = division * qubitIndex;
        const Int R = qubitIndex % 3 != 0 ? currentColor : 0;
        const Int G = qubitIndex % 3 != 1 ? currentColor : 0;
        const Int B = qubitIndex % 3 != 2 ? currentColor : 0;
        const Color virtualColor = {{ R, G, B }};
        virtualColors[qubitIndex] = virtualColor;
    }

    // Draw the mapping for each cycle.
    for (Int cycleIndex = 0; cycleIndex < imageOutput.circuitData.getAmountOfCycles(); cycleIndex++) {
        const Position4 position = imageOutput.structure.getCellPosition(cycleIndex, 0, QUANTUM);
        const Int xStart = position.x0;
        const Int xEnd = position.x1;

        // Calculate the qubit positions.
        Vec<Position2> qubitPositions(amountOfQubits, { 0, 0 });
        for (Int qubitIndex = 0; qubitIndex < amountOfQubits; qubitIndex++) {
            const Int column = parsedTopology ? topology.vertices[qubitIndex].x : qubitIndex % amountOfColumns;
            const Int row = parsedTopology ? topology.vertices[qubitIndex].y : qubitIndex / amountOfRows;
            const Int centerX = xStart + column * columnWidth + (column + 1) * layout.getQubitSpacing() + layout.getQubitRadius();
            const Int centerY = yStart + row * rowHeight + (row + 1) * layout.getQubitSpacing() + layout.getQubitRadius();
            qubitPositions[qubitIndex].x = centerX;
            qubitPositions[qubitIndex].y = centerY;
        }

        // Draw the edges.
        for (const Edge edge : topology.edges) {
            if (edge.src >= amountOfQubits || edge.dst >= amountOfQubits) {
                continue;
            }
            const Position2 src = qubitPositions[edge.src];
            const Position2 dst = qubitPositions[edge.dst];
            imageOutput.image.drawLine(src.x, src.y, dst.x, dst.y, black, 1.0f, LinePattern::UNBROKEN);
        }

        // Draw each of the qubit mappings in this cycle.
        for (Int qubitIndex = 0; qubitIndex < amountOfQubits; qubitIndex++) {
            const Position2 position = qubitPositions[qubitIndex];
            const Int virtualOperand = virtualQubits[cycleIndex][qubitIndex];

            // Draw qubit circle.
            const Color virtualColor = virtualOperand != -1 ? virtualColors[virtualOperand] : layout.getQubitFillColor();
            const Color fillColor = layout.getShowVirtualColors() && virtualOperand != -1 ? virtualColor : layout.getQubitFillColor();
            imageOutput.image.drawFilledCircle(position.x, position.y, layout.getQubitRadius(), fillColor, 1.0f);
            imageOutput.image.drawOutlinedCircle(position.x, position.y, layout.getQubitRadius(), layout.getQubitOutlineColor(), 1.0f, LinePattern::UNBROKEN);

            // Draw real qubit index if enabled.
            if (layout.getShowRealIndices()) {
                const Str realIndexLabel = utils::to_string(qubitIndex);
                const Dimensions realIndexLabelDimensions = calculateTextDimensions(realIndexLabel, layout.getFontHeightReal());

                const Int realIndexLabelX = position.x - realIndexLabelDimensions.width / 2;
                const Int realIndexLabelY = position.y - layout.getQubitRadius() - realIndexLabelDimensions.height - layout.getRealIndexSpacing();
                imageOutput.image.drawText(realIndexLabelX, realIndexLabelY, realIndexLabel, layout.getFontHeightReal(), layout.getTextColorReal());
            }

            // Draw virtual operand label on qubit.
            if (virtualOperand != -1) {
                const Str text = utils::to_string(virtualOperand);

                const Dimensions dimensions = calculateTextDimensions(text, layout.getFontHeightVirtual());
                const Int textX = position.x - dimensions.width / 2;
                const Int textY = position.y - dimensions.height / 2;
                imageOutput.image.drawText(textX, textY, text, layout.getFontHeightVirtual(), layout.getTextColorVirtual());
            }
        }
    }

    // Save the image if enabled.
    if (imageOutput.circuitLayout.saveImage) {
        imageOutput.image.save(generateFilePath("circuit_visualization", "bmp"));
    }

    // Display the filled in image.
    imageOutput.image.display("Mapping Graph");
}

Bool parseTopology(Json topologyJson, Topology &topology) {
    const Str fallbackMessage = "Falling back on basic visualization. Missing attribute: ";
    if (topologyJson.count("x_size") == 1) { topology.xSize = topologyJson["x_size"]; } else { QL_IOUT(fallbackMessage << "x_size"); return false; }
    if (topologyJson.count("y_size") == 1) { topology.ySize = topologyJson["y_size"]; } else { QL_IOUT(fallbackMessage << "y_size"); return false; }

    if (topologyJson.count("qubits") == 1) {
        const Json qubits = topologyJson["qubits"];
        topology.vertices.resize(qubits.size(), { 0, 0 });
        for (const Json qubit : qubits) {
            if (qubit.count("id") == 1 && qubit.count("x") == 1 && qubit.count("y") == 1) {
                const Int id = qubit["id"];
                const Int x = qubit["x"];
                const Int y = qubit["y"];
                topology.vertices[id].x = x;
                topology.vertices[id].y = y;
            }
            else {
                QL_IOUT(fallbackMessage << "id, or x or y");
                return false;
            }
        }
    } else {
        QL_IOUT(fallbackMessage << " qubits");
        return false;
    }

    if (topologyJson.count("edges") == 1) {
        const Json edges = topologyJson["edges"];
        topology.edges.resize(edges.size(), { 0, 0 });
        for (const Json edge : edges) {
            if (edge.count("id") == 1 && edge.count("src") == 1 && edge.count("dst") == 1) {
                const Int id = edge["id"];
                topology.edges[id].src = edge["src"];
                topology.edges[id].dst = edge["dst"];
            } else {
                QL_IOUT(fallbackMessage << " id, or src or dst");
                return false;
            }
        }
    } else {
        QL_IOUT(fallbackMessage << " edges");
        return false;
    }

    return true;
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
    if (config.count("initDefaultVirtuals") == 1)   layout.setInitDefaultVirtuals(config["initDefaultVirtuals"]);
    if (config.count("showVirtualColors") == 1)   layout.setShowVirtualColors(config["showVirtualColors"]);
    if (config.count("showRealIndices") == 1)   layout.setShowRealIndices(config["showRealIndices"]);
    if (config.count("useTopology") == 1)       layout.setUseTopology(config["useTopology"]);

    if (config.count("qubitRadius") == 1)       layout.setQubitRadius(config["qubitRadius"]);
    if (config.count("qubitSpacing") == 1)      layout.setQubitSpacing(config["qubitSpacing"]);
    if (config.count("fontHeightReal") == 1)      layout.setFontHeightReal(config["fontHeightReal"]);
    if (config.count("fontHeightVirtual") == 1)      layout.setFontHeightVirtual(config["fontHeightVirtual"]);

    if (config.count("textColorReal") == 1)      layout.setTextColorReal(config["textColorReal"]);
    if (config.count("textColorVirtual") == 1)      layout.setTextColorVirtual(config["textColorVirtual"]);
    if (config.count("qubitFillColor") == 1)      layout.setQubitFillColor(config["qubitFillColor"]);
    if (config.count("qubitOutlineColor") == 1)      layout.setQubitOutlineColor(config["qubitOutlineColor"]);

    if (config.count("realIndexSpacing") == 1)      layout.setRealIndexSpacing(config["realIndexSpacing"]);

    if (gridConfig.count("borderSize") == 1)    layout.setBorderSize(gridConfig["borderSize"]);

    return layout;
}

} // namespace ql

#endif //WITH_VISUALIZER