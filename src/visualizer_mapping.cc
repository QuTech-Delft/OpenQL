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
    Vec<GateProperties> gates = parseGates(program);

    // Parse the topology if it exists in the platform configuration file.
    Topology topology;
    const Bool parsedTopology = layout.getUseTopology() ? parseTopology(program->platform.topology, topology) : false;
    if (parsedTopology) {
        QL_DOUT("Succesfully parsed topology.");
        QL_DOUT("xSize: " << topology.xSize);
        QL_DOUT("ySize: " << topology.ySize);
        QL_DOUT("qubits:");
        for (UInt qubitIndex = 0; qubitIndex < topology.vertices.size(); qubitIndex++) {
            QL_DOUT("\tid: " << qubitIndex << " position: [" << topology.vertices[qubitIndex].x << ", " << topology.vertices[qubitIndex].y << "]");
        }
        QL_DOUT("edges:");
        for (const Edge edge : topology.edges) {
            QL_DOUT("\tsrc: " << edge.src << ", dst: " << edge.dst);
        }
    } else {
        QL_WOUT("Could not parse qubit topology. Falling back on basic visualization.");
    }

    // QL_IOUT("Gate input to mapping graph visualizer:");
    // printGates(gates);
    // printGatesShort(gates);

    // Get visualized circuit with extra wide cycles from visualizer_circuit.cc.
    const Int amountOfQubits = calculateAmountOfBits(gates, &GateProperties::operands);
    const Int amountOfColumns = parsedTopology ? topology.xSize : ceil(sqrt(amountOfQubits));
    const Int amountOfRows = parsedTopology ? topology.ySize : ceil(sqrt(amountOfQubits));
    const Int qubitDiameter = layout.getQubitRadius() * 2;
    const Int columnWidth = qubitDiameter;
    const Int rowHeight = qubitDiameter + (layout.getShowRealIndices() ? layout.getFontHeightReal() + layout.getRealIndexSpacing() * 2 : 0);

    // Calculate the virtual qubits mapping for each cycle.
    const Int cycleDuration = utoi(program->platform.cycle_time);
    Int amountOfCycles = calculateAmountOfCycles(gates, cycleDuration);
    if (amountOfCycles <= 0) {
        QL_FATAL("Circuit contains no cycles! Cannot visualize mapping graph.");
    }
    // Visualize the circuit sequentially if one or more gates were not scheduled yet.
    if (amountOfCycles == MAX_CYCLE) {
        // Add a sequential cycle to each gate.
        amountOfCycles = 0;
        for (GateProperties &gate : gates) {
            gate.cycle = amountOfCycles;
            amountOfCycles += gate.duration / cycleDuration;
        }
    }

    // This vector stores the qubit mapping per cycle.
    Vec<Vec<Int>> virtualQubits(amountOfCycles);

    // This vector stores whether the mapping has changed for each cycle compared to the previous cycle.
    Vec<Bool> mappingChangedPerCycle(amountOfCycles, false);

    // Compute the mappings for each cycle.
    computeMappingPerCycle(layout, virtualQubits, mappingChangedPerCycle, gates, amountOfCycles, amountOfQubits);
    
    // Compute the minimum cycle widths for each cycle.
    Vec<Int> minCycleWidths(amountOfCycles, 0);
    for (Int cycleIndex = 0; cycleIndex < amountOfCycles; cycleIndex++) {
        if (mappingChangedPerCycle[cycleIndex]) {
            minCycleWidths[cycleIndex] = amountOfColumns * columnWidth + (amountOfColumns + 1) * layout.getQubitSpacing();
        }
    }

    // Load the fill colors for virtual qubits.
    //TODO: implement better way of generating distinct colors for any given amount of qubits.
    Vec<Color> virtualColors(amountOfQubits);
    const Int division = 255 / amountOfQubits;
    for (Int qubitIndex = 0; qubitIndex < amountOfQubits; qubitIndex++) {
        const Int currentColor = division * qubitIndex;
        const Int R = qubitIndex % 3 != 0 ? currentColor : 0;
        const Int G = qubitIndex % 3 != 1 ? currentColor : 0;
        const Int B = qubitIndex % 3 != 2 ? currentColor : 0;
        const Color virtualColor = {{ (Byte)R, (Byte)G, (Byte)B }};
        virtualColors[qubitIndex] = virtualColor;
    }

    // Generate the image.
    const Int extendedImageHeight = amountOfRows * rowHeight + (amountOfRows + 1) * layout.getQubitSpacing() + layout.getBorderSize();
    ImageOutput imageOutput = generateImage(program, configuration, minCycleWidths, extendedImageHeight);

    // Fill in cycle spaces beneath the circuit with the mapping graph.
    const Int yStart = imageOutput.structure.getImageHeight() - extendedImageHeight;
    //const Int yEnd = imageOutput.structure.getImageHeight();

    // Draw the mapping for each cycle.
    for (Int cycleIndex = 0; cycleIndex < amountOfCycles; cycleIndex++) {
        // Skip cycles for which the mapping has not changed.
        if (!mappingChangedPerCycle[cycleIndex]) {
            continue;
        }

        const Position4 position = imageOutput.structure.getCellPosition(cycleIndex, 0, QUANTUM);
        const Int xStart = position.x0;
        //const Int xEnd = position.x1;

        // Calculate the qubit positions.
        Vec<Position2> qubitPositions(amountOfQubits, { 0, 0 });
        for (Int qubitIndex = 0; qubitIndex < amountOfQubits; qubitIndex++) {
            const Int column = parsedTopology ? topology.vertices[qubitIndex].x : qubitIndex % amountOfColumns;
            const Int row = parsedTopology ? topology.ySize - 1 - topology.vertices[qubitIndex].y : qubitIndex / amountOfRows; // flip y-axis for topology
            const Int centerX = xStart + column * columnWidth + (column + 1) * layout.getQubitSpacing() + layout.getQubitRadius();
            const Int centerY = yStart + row * rowHeight + (row + 1) * layout.getQubitSpacing() + layout.getQubitRadius();
            qubitPositions[qubitIndex].x = centerX;
            qubitPositions[qubitIndex].y = centerY;
        }

        // Draw the edges.
        for (const Edge edge : topology.edges) {
            // Ignore qubits that are not present in the circuit.
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

void computeMappingPerCycle(const MappingGraphLayout layout,
                            Vec<Vec<Int>> &virtualQubits,
                            Vec<Bool> &mappingChangedPerCycle,
                            const Vec<GateProperties> &gates,
                            const Int amountOfCycles,
                            const Int amountOfQubits) {
    // Initialize the first cycle with a virtual index = real index mapping.
    for (Int qubitIndex = 0; qubitIndex < amountOfQubits; qubitIndex++) {
        const Int virtualIndex = layout.getInitDefaultVirtuals() ? qubitIndex : -1;
        virtualQubits[0].push_back(virtualIndex);
        mappingChangedPerCycle[0] = true; // the mapping has changed for the first cycle always!
    }

    struct SwapOperands {
        Int r0;
        Int r1;
        Int v0;
        Int v1;
    };

    // Find the swaps.
    Vec<SwapOperands> swaps(amountOfCycles, {-1, -1, -1, -1});
    Int startSearchFromCycle = 0;
    for (const GateProperties &gate : gates) {
        // Search the remaining cycles of the circuit for a swap.
        if (gate.cycle >= startSearchFromCycle) {
            // If a swap is found, search the rest of the circuit for more parts of the swap.
            if (gate.swap_params.part_of_swap) {
                const Int r0 = gate.swap_params.r0;
                const Int r1 = gate.swap_params.r1;
                const Int v0 = gate.swap_params.v0;
                const Int v1 = gate.swap_params.v1;
                Int nextPartOfSwapCycle = gate.cycle;
                // Search for the cycle of the next part of the swap.
                for (const GateProperties &gateInSearch : gates) {
                    if (gateInSearch.swap_params.part_of_swap) {
                        const Int nextV0 = gateInSearch.swap_params.v0;
                        const Int nextV1 = gateInSearch.swap_params.v1;
                        if (gateInSearch.cycle > nextPartOfSwapCycle) {
                            if (v0 == nextV0 && v1 == nextV1) {
                                nextPartOfSwapCycle = gateInSearch.cycle;
                            }
                        }
                    }
                }

                // We now have a swap from gate.cycle to nextPartOfSwapCycle. Add it to the swap vector.
                const SwapOperands swapOperands = {r0, r1, v0, v1};
                swaps[nextPartOfSwapCycle] = swapOperands;
                mappingChangedPerCycle[nextPartOfSwapCycle] = true;

                // Continue the search from the next cycle.
                startSearchFromCycle = nextPartOfSwapCycle + 1;
            }
        }
    }

    // Fill the cycles with qubit mappings.
    for (Int cycleIndex = 1; cycleIndex < amountOfCycles; cycleIndex++) {
        // Copy virtual qubit operand from the previous cycle of the same qubit.
        for (Int qubitIndex = 0; qubitIndex < amountOfQubits; qubitIndex++) {
            virtualQubits[cycleIndex].push_back(virtualQubits[cycleIndex - 1][qubitIndex]);
        }
        for (const GateProperties &gate : gates) {
            if (gate.cycle == cycleIndex) {
                for (UInt qubitIndex = 0; qubitIndex < gate.operands.size(); qubitIndex++) {
                    const Int mappingIndex = gate.operands[qubitIndex];
                    const Int mappingIndexFromPreviousCycle = virtualQubits[cycleIndex - 1][mappingIndex];
                    if (mappingIndexFromPreviousCycle == -1) {
                        virtualQubits[cycleIndex][mappingIndex] = mappingIndex;
                        mappingChangedPerCycle[cycleIndex] = true;
                    }
                }
            }
        }
        // Check if there was a swap in this cycle.
        const Int v0 = swaps[cycleIndex].v0;
        const Int v1 = swaps[cycleIndex].v1;
        const Bool swapInCycle = v0 != -1 && v1 != -1;
        if (swapInCycle) {
            const Int r0 = swaps[cycleIndex].r0;
            const Int r1 = swaps[cycleIndex].r1;
            virtualQubits[cycleIndex][r0] = v0;
            virtualQubits[cycleIndex][r1] = v1;
        }
    }
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