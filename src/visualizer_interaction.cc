/** \file
 * Definition of the visualizer qubit interaction graph.
 */

#ifdef WITH_VISUALIZER

#include <cmath>
#include "utils/pair.h"
#include "visualizer.h"
#include "visualizer_common.h"
#include "visualizer_interaction.h"
#include "CImg.h"
#undef Bool

namespace ql {

using namespace utils;

void visualizeInteractionGraph(const Vec<GateProperties> &gates) {
    QL_IOUT("Visualizing qubit interaction graph...");

    //TODO: load these from a file
    const Int borderWidth = 32;
    const Int minInteractionCircleRadius = 100;
    const Real interactionCircleRadiusModifier = 3.0;
    const Int qubitRadius = 17;
    const Int labelFontHeight = 13;
    const Color circleOutlineColor = black;
    const Color circleFillColor = white;
    const Color labelColor = black;
    const Color edgeColor = black;

    // const Int amountOfQubits = calculateAmountOfBits(gates, &GateProperties::operands);
    Int amountOfQubits = calculateAmountOfBits(gates, &GateProperties::operands);
    // Prepare the interaction list per qubit.
    // const Vec<QubitInteractions> qubitInteractionList = findQubitInteractions(gates, amountOfQubits);
    Vec<Qubit> qubits = findQubitInteractions(gates, amountOfQubits);
    printInteractionList(qubits);

    if (qubits.size() > 1) {
        // --------------------------- REMOVE WHEN DONE --------------------------- //
        // const Int amountOfTestQubits = 7;
        // for (Int i = 0; i < amountOfTestQubits; i++) {
        //     qubits.push_back({amountOfQubits + i, {}});
        // }
        // amountOfQubits += amountOfTestQubits;
        // --------------------------- REMOVE WHEN DONE --------------------------- //

        // Calculate the interaction circle properties.
        const Real thetaSpacing = 2 * M_PI / amountOfQubits;
        const Int interactionCircleRadius = max(minInteractionCircleRadius,
            (Int) (interactionCircleRadiusModifier * calculateQubitCircleRadius(qubitRadius, thetaSpacing)));
        const Position2 center{borderWidth + interactionCircleRadius, borderWidth + interactionCircleRadius};

        // Calculate the qubit coordinates on the interaction circle.
        Vec<Pair<Qubit, Position2>> qubitPositions;//(amountOfQubits);
        for (const Qubit &qubit : qubits) {
            const Real theta = qubit.qubitIndex * thetaSpacing;
            const Position2 position = calculateQubitPosition(interactionCircleRadius, theta, center);
            qubitPositions.push_back( {qubit, position} );
            // qubitPositions[qubit.qubitIndex] = {qubit, position};
        }

        // Initialize the image.
        QL_DOUT("Initializing image...");
        const Int numberOfChannels = 3;
        const Int imageWidth = 2 * (borderWidth + interactionCircleRadius);
        const Int imageHeight = 2 * (borderWidth + interactionCircleRadius);
        cimg_library::CImg<unsigned char> image(imageWidth, imageHeight, 1, numberOfChannels);
        image.fill(255);

        // Draw the edges between interacting qubits.
        //TODO: edges are drawn twice between qubits
        for (const Pair<Qubit, Position2> &qubit : qubitPositions) {
            const Position2 qubitPosition = qubit.second;
            for (const InteractionsWithQubit &interactionsWithQubit : qubit.first.interactions) {
                const Real theta = interactionsWithQubit.qubitIndex * thetaSpacing;
                const Position2 interactionPosition = calculateQubitPosition(interactionCircleRadius, theta, center);
                const Str label = to_string(qubit.first.qubitIndex);
                image.draw_line(qubitPosition.x, qubitPosition.y, interactionPosition.x, interactionPosition.y, edgeColor.data());
            }
        }
        // Draw the qubits.
        for (const Pair<Qubit, Position2> &qubit : qubitPositions) {
            // Draw the circle outline.
            image.draw_circle(qubit.second.x, qubit.second.y, qubitRadius, circleFillColor.data());
            image.draw_circle(qubit.second.x, qubit.second.y, qubitRadius, circleOutlineColor.data(), 1, 0xFFFFFFFF);
            // Draw the qubit label.
            const Str label = to_string(qubit.first.qubitIndex);
            const Dimensions labelDimensions = calculateTextDimensions(label, labelFontHeight);
            const Int xGap = (2 * qubitRadius - labelDimensions.width) / 2;
            const Int yGap = (2 * qubitRadius - labelDimensions.height) / 2;
            image.draw_text(qubit.second.x - qubitRadius + xGap, qubit.second.y - qubitRadius + yGap, label.c_str(), labelColor.data(), 0, 1, labelFontHeight);
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

Real calculateQubitCircleRadius(const Int qubitRadius, const Real theta) {
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
    const Real r = qubitRadius;
    const Real alpha = PI - PI / 2.0 - theta / 2.0;
    const Real h = r * std::tan(alpha);
    const Real R = sqrt(h * h + r * r);

    return R;
}

Position2 calculateQubitPosition(const Int radius, const Real theta, const Position2 &center) {
    const Int x = (Int) (radius * std::cos(theta) + center.x);
    const Int y = (Int) (radius * std::sin(theta) + center.y);

    return {x, y};
}

Vec<Qubit> findQubitInteractions(const Vec<GateProperties> &gates, const Int amountOfQubits) {
    // Initialize the qubit vector.
    Vec<Qubit> qubits(amountOfQubits);
    for (Int qubitIndex = 0; qubitIndex < amountOfQubits; qubitIndex++) {
        qubits[qubitIndex] = {qubitIndex, {}};
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