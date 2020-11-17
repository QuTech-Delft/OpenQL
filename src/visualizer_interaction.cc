/**
 * @file   visualizer_interaction.cc
 * @date   11/2020
 * @author Tim van der Meer
 * @brief  definition of the visualizer qubit interaction graph
 */

#ifdef WITH_VISUALIZER

#include "visualizer.h"
#include "visualizer_types.h"
#include "visualizer_common.h"
#include "visualizer_interaction.h"
#include "CImg.h"
#include "json.h"

#include <math.h>

namespace ql {

void visualizeInteractionGraph(const ql::quantum_program* program, const VisualizerConfigurationPaths configurationPaths) {
    IOUT("Visualizing qubit interaction graph...");

    // Get the gate list from the program.
    DOUT("Getting gate list...");
    std::vector<GateProperties> gates = parseGates(program);
    if (gates.size() == 0) {
        FATAL("Quantum program contains no gates!");
    }

    InteractionGraphLayout layout = parseInteractionGraphLayout(configurationPaths.config);

    const int amountOfQubits = calculateAmountOfBits(gates, &GateProperties::operands);
    // Prepare the interaction list per qubit.
    const std::vector<Qubit> qubits = findQubitInteractions(gates, amountOfQubits);
    // printInteractionList(qubits);

    if (qubits.size() > 1) {

        // Calculate the interaction circle properties.
        const double thetaSpacing = 2 * M_PI / amountOfQubits;
        const int interactionCircleRadius = std::max(layout.getMinInteractionCircleRadius(),
            (int) (layout.getInteractionCircleRadiusModifier() * calculateQubitCircleRadius(layout.getQubitRadius(), thetaSpacing)));
        const Position2 center{layout.getBorderWidth() + interactionCircleRadius, layout.getBorderWidth() + interactionCircleRadius};

        // Calculate the qubit coordinates on the interaction circle.
        std::vector<std::pair<Qubit, Position2>> qubitPositions;
        for (const Qubit &qubit : qubits) {
            const double theta = qubit.qubitIndex * thetaSpacing;
            const Position2 position = calculateQubitPosition(interactionCircleRadius, theta, center);
            qubitPositions.push_back( {qubit, position} );
        }

        // Initialize the image.
        DOUT("Initializing image...");
        const int numberOfChannels = 3;
        const int imageWidth = 2 * (layout.getBorderWidth() + interactionCircleRadius);
        const int imageHeight = 2 * (layout.getBorderWidth() + interactionCircleRadius);
        cimg_library::CImg<unsigned char> image(imageWidth, imageHeight, 1, numberOfChannels);
        image.fill(255);

        // Draw the edges between interacting qubits.
        //TODO: edges are drawn twice between qubits
        std::vector<std::pair<int, int>> drawnEdges;
        for (const std::pair<Qubit, Position2> &qubit : qubitPositions) {
            const Position2 qubitPosition = qubit.second;
            for (const InteractionsWithQubit &interactionsWithQubit : qubit.first.interactions) {
                if (edgeAlreadyDrawn(drawnEdges, qubit.first.qubitIndex, interactionsWithQubit.qubitIndex))
                    continue;
                
                drawnEdges.push_back( {qubit.first.qubitIndex,interactionsWithQubit.qubitIndex } );

                // Draw the edge.
                const double theta = interactionsWithQubit.qubitIndex * thetaSpacing;
                const Position2 interactionPosition = calculateQubitPosition(interactionCircleRadius, theta, center);
                image.draw_line(qubitPosition.x, qubitPosition.y, interactionPosition.x, interactionPosition.y, layout.getEdgeColor().data());

                // Draw the number of interactions.
                const std::string label = std::to_string(interactionsWithQubit.amountOfInteractions);
                const Dimensions labelDimensions = calculateTextDimensions(label, layout.getLabelFontHeight());
                const int hDiff = qubitPosition.x - interactionPosition.x;
                const int vDiff = qubitPosition.y - interactionPosition.y;
                int x = 0;
                int y = 0;
                if (hDiff > 0) {
                    x = qubitPosition.x - layout.getQubitRadius() - labelDimensions.width;
                    if (vDiff > 0) {
                        y = qubitPosition.y - layout.getQubitRadius() - labelDimensions.height;
                    } else {
                        y = qubitPosition.y + layout.getQubitRadius();
                    }
                } else {
                    x = qubitPosition.x + layout.getQubitRadius();
                    if (vDiff > 0) {
                        y = qubitPosition.y - layout.getQubitRadius() - labelDimensions.height;
                    } else {
                        y = qubitPosition.y + layout.getQubitRadius();
                    }
                }
                image.draw_text(x, y, label.c_str(), layout.getLabelColor().data(), 0, 1, layout.getLabelFontHeight());
            }
        }
        // Draw the qubits.
        for (const std::pair<Qubit, Position2> &qubit : qubitPositions) {
            // Draw the circle outline.
            image.draw_circle(qubit.second.x, qubit.second.y, layout.getQubitRadius(), layout.getCircleFillColor().data());
            image.draw_circle(qubit.second.x, qubit.second.y, layout.getQubitRadius(), layout.getCircleOutlineColor().data(), 1, 0xFFFFFFFF);
            // Draw the qubit label.
            const std::string label = std::to_string(qubit.first.qubitIndex);
            const Dimensions labelDimensions = calculateTextDimensions(label, layout.getLabelFontHeight());
            const int xGap = (2 * layout.getQubitRadius() - labelDimensions.width) / 2;
            const int yGap = (2 * layout.getQubitRadius() - labelDimensions.height) / 2;
            image.draw_text(qubit.second.x - layout.getQubitRadius() + xGap, qubit.second.y - layout.getQubitRadius() + yGap, label.c_str(), layout.getLabelColor().data(), 0, 1, layout.getLabelFontHeight());
        }

        // Save the image if enabled.
        if (layout.saveImage) {
            image.save("qubit_interaction_graph.bmp");
        }

        // Display the image.
        DOUT("Displaying image...");
        image.display("Qubit Interaction Graph");
    } else if (qubits.size() == 1) {
        // Draw the single qubit in the middle of the circle.
        //TODO
    } else {
        FATAL("Quantum program contains no qubits. Unable to visualize qubit interaction graph!");
    }
}

InteractionGraphLayout parseInteractionGraphLayout(const std::string &configPath) {
    DOUT("Parsing visualizer configuration file for interaction graph visualization...");

    json fullConfig;
    try {
        fullConfig = load_json(configPath);
    } catch (json::exception &e) {
        FATAL("Failed to load the visualization config file: \n\t" << std::string(e.what()));
    }

    json config;
    if (fullConfig.count("interactionGraph") == 1) {
        config = fullConfig["interactionGraph"];
    } else {
        WOUT("Could not find interaction graph configuration in visualizer configuration file. Is it named correctly?");
    }

    // Fill the layout object with the values from the config file. Any missing values will assume the default values hardcoded in the layout object.
    InteractionGraphLayout layout;

    // Check if the image should be saved to disk.
    if (fullConfig.count("saveImage") == 1) {
        layout.saveImage = fullConfig["saveImage"];
    }

    // Load the parameters.
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

double calculateQubitCircleRadius(const int qubitRadius, const double theta) {
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
    const double r = qubitRadius;
    const double alpha = M_PI - M_PI / 2.0 - theta / 2.0;
    const double h = r * tan(alpha);
    const double R = sqrt(h * h + r * r);

    return R;
}

Position2 calculateQubitPosition(const int radius, const double theta, const Position2 center) {
    const long x = (long) (radius * cos(theta) + center.x);
    const long y = (long) (radius * sin(theta) + center.y);

    return {x, y};
}

std::vector<Qubit> findQubitInteractions(const std::vector<GateProperties> gates, const int amountOfQubits) {
    // Initialize the qubit vector.
    std::vector<Qubit> qubits(amountOfQubits);
    for (int qubitIndex = 0; qubitIndex < amountOfQubits; qubitIndex++) {
        qubits[qubitIndex] = {qubitIndex, {}};
    }

    for (const GateProperties &gate : gates) {
        const std::vector<GateOperand> operands = getGateOperands(gate);
        if (operands.size() > 1) {
            // Find the qubits the current gate interacts with.
            std::vector<int> qubitIndices;
            for (const GateOperand &operand : operands) {
                if (operand.bitType == QUANTUM) {
                    qubitIndices.push_back(operand.index);
                }
            }

            // Add the interaction indices to the qubits.
            for (int i = 0; i < qubitIndices.size(); i++) {
                for (int j = 0; j < qubitIndices.size(); j++) {
                    // Do not add an interaction between a qubit and itself.
                    if (i != j) {
                        std::vector<InteractionsWithQubit> &interactions = qubits[qubitIndices[i]].interactions;
                        // Find the existing interaction count with the current qubit if it exists.
                        bool found = false;
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

bool edgeAlreadyDrawn(const std::vector<std::pair<int, int>> drawnEdges, const int first, const int second) {
    // Check if the edge already exists.
    for (const std::pair<int, int> &drawnEdge : drawnEdges) {
        if ((drawnEdge.first == first && drawnEdge.second == second) || (drawnEdge.first == second && drawnEdge.second == first)) {
            return true;
        }
    }

    return false;
}

void printInteractionList(const std::vector<Qubit> qubits) {
    // Print the qubit interaction list.
    for (const Qubit &qubit : qubits) {
        IOUT("qubit " << qubit.qubitIndex << " interacts with:");
        for (const InteractionsWithQubit &interactionsWithQubit : qubit.interactions) {
            IOUT("\tqubit " << interactionsWithQubit.qubitIndex << ": " << interactionsWithQubit.amountOfInteractions << " times");
        }
    }
}

} // namespace ql

#endif //WITH_VISUALIZER