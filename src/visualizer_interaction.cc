/** \file
 * Definition of the visualizer qubit interaction graph.
 */

#ifdef WITH_VISUALIZER

#include "visualizer.h"
#include "visualizer_types.h"
#include "visualizer_common.h"
#include "visualizer_interaction.h"
#include "visualizer_cimg.h"
#include "utils/json.h"
#include "utils/num.h"
#include "utils/vec.h"
#include "utils/pair.h"

#include <fstream>

namespace ql {

using namespace utils;

void visualizeInteractionGraph(const quantum_program* program, const VisualizerConfiguration &configuration) {
    QL_IOUT("Visualizing qubit interaction graph...");

    // Get the gate list from the program.
    QL_DOUT("Getting gate list...");
    const Vec<GateProperties> gates = parseGates(program);
    if (gates.size() == 0) {
        QL_FATAL("Quantum program contains no gates!");
    }

    InteractionGraphLayout layout = parseInteractionGraphLayout(configuration.visualizerConfigPath);

    const Int amountOfQubits = calculateAmountOfBits(gates, &GateProperties::operands);
    // Prepare the interaction list per qubit.
    const Vec<Qubit> qubits = findQubitInteractions(gates, amountOfQubits);

    // Generate the DOT file if enabled.
    if (layout.isDotFileOutputEnabled()) {
        generateAndSaveDOTFile(qubits);
    }

    if (qubits.size() > 1) {

        // Calculate the interaction circle properties.
        const Real thetaSpacing = 2 * M_PI / amountOfQubits;
        const Int interactionCircleRadius = max(layout.getMinInteractionCircleRadius(),
            (Int) (layout.getInteractionCircleRadiusModifier() * calculateQubitCircleRadius(layout.getQubitRadius(), thetaSpacing)));
        const Position2 center{layout.getBorderWidth() + interactionCircleRadius, layout.getBorderWidth() + interactionCircleRadius};

        // Calculate the qubit coordinates on the interaction circle.
        Vec<Pair<Qubit, Position2>> qubitPositions;
        for (const Qubit &qubit : qubits) {
            const Real theta = qubit.qubitIndex * thetaSpacing;
            const Position2 position = calculatePositionOnCircle(interactionCircleRadius, theta, center);
            qubitPositions.push_back( {qubit, position} );
        }

        // Initialize the image.
        QL_DOUT("Initializing image...");
        const Int imageWidth = 2 * (layout.getBorderWidth() + interactionCircleRadius);
        const Int imageHeight = 2 * (layout.getBorderWidth() + interactionCircleRadius);
        Image image(imageWidth, imageHeight);
        image.fill(255);

        // Draw the edges between interacting qubits.
        Vec<Pair<Int, Int>> drawnEdges;
        // std::vector<std::pair<int, int>> drawnEdges;
        for (const Pair<Qubit, Position2> &qubit : qubitPositions) {
            const Position2 qubitPosition = qubit.second;
            for (const InteractionsWithQubit &interactionsWithQubit : qubit.first.interactions) {
                if (isEdgeAlreadyDrawn(drawnEdges, qubit.first.qubitIndex, interactionsWithQubit.qubitIndex))
                    continue;
                
                drawnEdges.push_back( {qubit.first.qubitIndex, interactionsWithQubit.qubitIndex } );

                // Draw the edge.
                const Real theta = interactionsWithQubit.qubitIndex * thetaSpacing;
                const Position2 interactionPosition = calculatePositionOnCircle(interactionCircleRadius, theta, center);
                image.drawLine(qubitPosition.x, qubitPosition.y, interactionPosition.x, interactionPosition.y, layout.getEdgeColor());

                // Calculate label dimensions.
                const Str label = to_string(interactionsWithQubit.amountOfInteractions);
                const Dimensions labelDimensions = calculateTextDimensions(label, layout.getLabelFontHeight());
                const Int a = labelDimensions.width;
                const Int b = labelDimensions.height;
                const Int labelRadius = sqrt(a * a + b * b);

                // Calculate position of label.
                const Int deltaX = interactionPosition.x - qubitPosition.x;
                const Int deltaY = interactionPosition.y - qubitPosition.y;
                const Real angle = atan2(deltaY, deltaX);
                const Position2 labelPosition = calculatePositionOnCircle(layout.getQubitRadius() + labelRadius, angle, qubitPosition);

                // Draw the number of interactions.
                image.drawText(labelPosition.x, labelPosition.y, label, layout.getLabelFontHeight(), layout.getLabelColor());
            }
        }
        // Draw the qubits.
        for (const Pair<Qubit, Position2> &qubit : qubitPositions) {
            // Draw the circle outline.
            image.drawFilledCircle(qubit.second.x, qubit.second.y, layout.getQubitRadius(), layout.getCircleFillColor(), 1);
            image.drawOutlinedCircle(qubit.second.x, qubit.second.y, layout.getQubitRadius(), layout.getCircleOutlineColor(), 1, LinePattern::UNBROKEN);
            // Draw the qubit label.
            const Str label = to_string(qubit.first.qubitIndex);
            const Dimensions labelDimensions = calculateTextDimensions(label, layout.getLabelFontHeight());
            const Int xGap = (2 * layout.getQubitRadius() - labelDimensions.width) / 2;
            const Int yGap = (2 * layout.getQubitRadius() - labelDimensions.height) / 2;
            image.drawText(qubit.second.x - layout.getQubitRadius() + xGap, qubit.second.y - layout.getQubitRadius() + yGap, label, layout.getLabelFontHeight(), layout.getLabelColor());
        }

        // Save the image if enabled.
        if (layout.saveImage) {
            image.save("qubit_interaction_graph.bmp");
        }

        // Display the image.
        QL_DOUT("Displaying image...");
        image.display("Qubit Interaction Graph");
    } else if (qubits.size() == 1) {
        // Draw the single qubit in the middle of the circle.
        //TODO
    } else {
        QL_FATAL("Quantum program contains no qubits. Unable to visualize qubit interaction graph!");
    }
}

void generateAndSaveDOTFile(const Vec<Qubit> &qubits) {
    try
    {
        QL_IOUT("Generating DOT file for qubit interaction graph...");

        std::ofstream output(generateFilePath("qubit_interaction_graph", "dot"));
        output << "graph qubit_interaction_graph {\n";
        output << "    node [shape=circle];\n";

        Vec<Pair<Int, Int>> drawnEdges;
        // std::vector<std::pair<int, int>> drawnEdges;
        for (const Qubit &qubit : qubits) {
            for (const InteractionsWithQubit &target : qubit.interactions) {
                if (isEdgeAlreadyDrawn(drawnEdges, qubit.qubitIndex, target.qubitIndex))
                    continue;
                drawnEdges.push_back( {qubit.qubitIndex, target.qubitIndex } );

                output << "    " << qubit.qubitIndex << " -- " << target.qubitIndex << " [label=" << target.amountOfInteractions << "];\n";
            }
        }

        output << "}";
        output.close();

        QL_IOUT("DOT file saved!");
    }
    catch(const Exception& e)
    {
        QL_WOUT("Could not save DOT file for qubit interaction graph: " << e.what());
    }
}

InteractionGraphLayout parseInteractionGraphLayout(const Str &configPath) {
    QL_DOUT("Parsing visualizer configuration file for interaction graph visualization...");

    Json fullConfig;
    try {
        fullConfig = load_json(configPath);
    } catch (Json::exception &e) {
        QL_FATAL("Failed to load the visualization config file: \n\t" << std::string(e.what()));
    }

    Json config;
    if (fullConfig.count("interactionGraph") == 1) {
        config = fullConfig["interactionGraph"];
    } else {
        QL_WOUT("Could not find interaction graph configuration in visualizer configuration file. Is it named correctly?");
    }

    // Fill the layout object with the values from the config file. Any missing values will assume the default values hardcoded in the layout object.
    InteractionGraphLayout layout;

    // Check if the image should be saved to disk.
    if (fullConfig.count("saveImage") == 1) {
        layout.saveImage = fullConfig["saveImage"];
    }

    // Load the parameters.
    if (config.count("outputDotFile") == 1)     layout.enableDotFileOutput(config["outputDotFile"]);

    if (config.count("borderWidth") == 1)                       layout.setBorderWidth(config["borderWidth"]);
    if (config.count("minInteractionCircleRadius") == 1)        layout.setMinInteractionCircleRadius(config["minInteractionCircleRadius"]);
    if (config.count("interactionCircleRadiusModifier") == 1)   layout.setInteractionCircleRadiusModifier(config["interactionCircleRadiusModifier"]);
    if (config.count("qubitRadius") == 1)                       layout.setQubitRadius(config["qubitRadius"]);
    if (config.count("labelFontHeight") == 1)                   layout.setLabelFontHeight(config["labelFontHeight"]);
    
    if (config.count("circleOutlineColor") == 1)    layout.setCircleOutlineColor(config["circleOutlineColor"]);
    if (config.count("circleFillColor") == 1)       layout.setCircleFillColor(config["circleFillColor"]);
    if (config.count("labelColor") == 1)            layout.setLabelColor(config["labelColor"]);
    if (config.count("edgeColor") == 1)             layout.setEdgeColor(config["edgeColor"]);

    return layout;
}

Real calculateQubitCircleRadius(const Int qubitRadius, const Real theta) {
    // - Distance between the centers of two adjacent qubits should be at
    //   least 2 times the qubit radius.
    // - We know the angle (theta) of the iscosceles triangle formed between the
    //   center of the circumferent circle and the two centers of the adjacent
    //   qubit circles, and we know the length of the base of that triangle.
    // - The unknown we want to calculate is the length of the two equisized
    //   sides of the triangle.
    // - That length is the minimum required radius of the circumferent circle,
    //   such that the qubit circles do not overlap.

    // Here be trigonometry.
    const Real r = qubitRadius;
    const Real alpha = PI - PI / 2.0 - theta / 2.0;
    const Real h = r * tan(alpha);
    const Real R = sqrt(h * h + r * r);

    return R;
}

Position2 calculatePositionOnCircle(const Int radius, const Real theta, const Position2 &center) {
    const Int x = (Int) (radius * cos(theta) + center.x);
    const Int y = (Int) (radius * sin(theta) + center.y);

    return {x, y};
}

Vec<Qubit> findQubitInteractions(const Vec<GateProperties> &gates, const Int amountOfQubits) {
    // Initialize the qubit vector.
    Vec<Qubit> qubits(amountOfQubits);
    for (Int qubitIndex = 0; qubitIndex < amountOfQubits; qubitIndex++) {
        Qubit qubit;
        qubit.qubitIndex = qubitIndex;
        // qubits[qubitIndex] = {qubitIndex, {}};
        qubits[qubitIndex] = qubit;
    }

    for (const GateProperties &gate : gates) {
        const Vec<GateOperand> operands = getGateOperands(gate);
        if (operands.size() > 1) {
            // Find the qubits the current gate interacts with.
            Vec<Int> qubitIndices;
            for (const GateOperand &operand : operands) {
                if (operand.bitType == QUANTUM) {
                    qubitIndices.push_back(operand.index);
                }
            }

            // Add the interaction indices to the qubits.
            for (UInt i = 0; i < qubitIndices.size(); i++) {
                for (UInt j = 0; j < qubitIndices.size(); j++) {
                    // Do not add an interaction between a qubit and itself.
                    if (i != j) {
                        Vec<InteractionsWithQubit> &interactions = qubits[qubitIndices[i]].interactions;
                        // Find the existing interaction count with the current qubit if it exists.
                        Bool found = false;
                        for (InteractionsWithQubit &interactionsWithQubit : interactions) {
                            if (interactionsWithQubit.qubitIndex == qubitIndices[j]) {
                                found = true;
                                interactionsWithQubit.amountOfInteractions++;
                            }
                        }
                        // If the interaction count does not yet exist, add a new one.
                        if (!found) {
                            interactions.push_back( {qubitIndices[j], 1} );
                        }
                    }
                }
            }
        }
    }

    return qubits;
}

Bool isEdgeAlreadyDrawn(const Vec<Pair<Int, Int>> &drawnEdges, const Int first, const Int second) {
    // Check if the edge already exists.
    for (const Pair<Int, Int> &drawnEdge : drawnEdges) {
        if ((drawnEdge.first == first && drawnEdge.second == second) || (drawnEdge.first == second && drawnEdge.second == first)) {
            return true;
        }
    }

    return false;
}

// bool isEdgeAlreadyDrawn(const std::vector<std::pair<int, int>> &drawnEdges, const int first, const int second) {
//     // Check if the edge already exists.
//     for (const std::pair<int, int> &drawnEdge : drawnEdges) {
//         if ((drawnEdge.first == first && drawnEdge.second == second) || (drawnEdge.first == second && drawnEdge.second == first)) {
//             return true;
//         }
//     }

//     return false;
// }

void printInteractionList(const Vec<Qubit> &qubits) {
    // Print the qubit interaction list.
    for (const Qubit &qubit : qubits) {
        QL_IOUT("qubit " << qubit.qubitIndex << " interacts with:");
        for (const InteractionsWithQubit &interactionsWithQubit : qubit.interactions) {
            QL_IOUT("\tqubit " << interactionsWithQubit.qubitIndex << ": " << interactionsWithQubit.amountOfInteractions << " times");
        }
    }
}

} // namespace ql

#endif //WITH_VISUALIZER