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

#include <math.h>

namespace ql {

void visualizeInteractionGraph(const ql::quantum_program* program) {
    IOUT("Visualizing qubit interaction graph...");

    // Get the gate list from the program.
    DOUT("Getting gate list...");
    std::vector<GateProperties> gates = parseGates(program);
    if (gates.size() == 0) {
        FATAL("Quantum program contains no gates!");
    }

    //TODO: load these from a file
    const int borderWidth = 32;
    const int minInteractionCircleRadius = 100;
    const double interactionCircleRadiusModifier = 3.0;
    const int qubitRadius = 17;
    const int labelFontHeight = 13;
    const Color circleOutlineColor = black;
    const Color circleFillColor = white;
    const Color labelColor = black;
    const Color edgeColor = black;

    // const int amountOfQubits = calculateAmountOfBits(gates, &GateProperties::operands);
    int amountOfQubits = calculateAmountOfBits(gates, &GateProperties::operands);
    // Prepare the interaction list per qubit.
    // const std::vector<QubitInteractions> qubitInteractionList = findQubitInteractions(gates, amountOfQubits);
    std::vector<Qubit> qubits = findQubitInteractions(gates, amountOfQubits);
    printInteractionList(qubits);

    if (qubits.size() > 1) {

        // Calculate the interaction circle properties.
        const double thetaSpacing = 2 * M_PI / amountOfQubits;
        const int interactionCircleRadius = std::max(minInteractionCircleRadius,
            (int) (interactionCircleRadiusModifier * calculateQubitCircleRadius(qubitRadius, thetaSpacing)));
        const Position2 center{borderWidth + interactionCircleRadius, borderWidth + interactionCircleRadius};

        // Calculate the qubit coordinates on the interaction circle.
        std::vector<std::pair<Qubit, Position2>> qubitPositions;//(amountOfQubits);
        for (const Qubit &qubit : qubits) {
            const double theta = qubit.qubitIndex * thetaSpacing;
            const Position2 position = calculateQubitPosition(interactionCircleRadius, theta, center);
            qubitPositions.push_back( {qubit, position} );
            // qubitPositions[qubit.qubitIndex] = {qubit, position};
        }

        // Initialize the image.
        DOUT("Initializing image...");
        const int numberOfChannels = 3;
        const int imageWidth = 2 * (borderWidth + interactionCircleRadius);
        const int imageHeight = 2 * (borderWidth + interactionCircleRadius);
        cimg_library::CImg<unsigned char> image(imageWidth, imageHeight, 1, numberOfChannels);
        image.fill(255);

        // Draw the edges between interacting qubits.
        //TODO: edges are drawn twice between qubits
        for (const std::pair<Qubit, Position2> &qubit : qubitPositions) {
            const Position2 qubitPosition = qubit.second;
            for (const InteractionsWithQubit &interactionsWithQubit : qubit.first.interactions) {
                const double theta = interactionsWithQubit.qubitIndex * thetaSpacing;
                const Position2 interactionPosition = calculateQubitPosition(interactionCircleRadius, theta, center);
                const std::string label = std::to_string(qubit.first.qubitIndex);
                const Dimensions labelDimensions = calculateTextDimensions(label, labelFontHeight);
                image.draw_line(qubitPosition.x, qubitPosition.y, interactionPosition.x, interactionPosition.y, edgeColor.data());
            }
        }
        // Draw the qubits.
        for (const std::pair<Qubit, Position2> &qubit : qubitPositions) {
            // Draw the circle outline.
            image.draw_circle(qubit.second.x, qubit.second.y, qubitRadius, circleFillColor.data());
            image.draw_circle(qubit.second.x, qubit.second.y, qubitRadius, circleOutlineColor.data(), 1, 0xFFFFFFFF);
            // Draw the qubit label.
            const std::string label = std::to_string(qubit.first.qubitIndex);
            const Dimensions labelDimensions = calculateTextDimensions(label, labelFontHeight);
            const int xGap = (2 * qubitRadius - labelDimensions.width) / 2;
            const int yGap = (2 * qubitRadius - labelDimensions.height) / 2;
            image.draw_text(qubit.second.x - qubitRadius + xGap, qubit.second.y - qubitRadius + yGap, label.c_str(), labelColor.data(), 0, 1, labelFontHeight);
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

double calculateQubitCircleRadius(const int qubitRadius, const double theta) {
    // - Distance between the centers of two adjacent qubits should be at
    //   least 2 * qubit radius.
    // - We know the angle (theta) of the iscosceles triangle formed between the
    //   center of the circumferent circle and the two centers of the adjacent
    //   qubit circles, and we know the length of the base of that triangle.
    // - The unknown we want to calculate is the length of the two
    //   equisized sides of the triangle.
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