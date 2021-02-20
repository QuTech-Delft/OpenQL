/** \file
 * Declaration of the visualizer qubit interaction graph.
 */

#pragma once

#ifdef WITH_VISUALIZER

#include "visualizer.h"
#include "visualizer_types.h"
#include "utils/num.h"
#include "utils/str.h"
#include "utils/vec.h"
#include "utils/pair.h"

namespace ql {

struct InteractionsWithQubit {
    utils::Int qubitIndex;
    utils::Int amountOfInteractions;

    // InteractionsWithQubit() = delete;
};

struct Qubit {
    utils::Int qubitIndex = 0;
    utils::Vec<InteractionsWithQubit> interactions;
};

void visualizeInteractionGraph(const quantum_program* program, const VisualizerConfiguration &configuration);

void generateAndSaveDOTFile(const utils::Vec<Qubit> &qubits);

InteractionGraphLayout parseInteractionGraphLayout(const utils::Str &configPath);

utils::Real calculateQubitCircleRadius(const utils::Int qubitRadius, const utils::Real theta);
Position2 calculatePositionOnCircle(const utils::Int radius, utils::Real theta, const Position2 &center);
utils::Vec<Qubit> findQubitInteractions(const utils::Vec<GateProperties> &gates, const utils::Int amountOfQubits);

// bool isEdgeAlreadyDrawn(const std::vector<std::pair<int, int>> &drawnEdges, const int first, const int second);
utils::Bool isEdgeAlreadyDrawn(const utils::Vec<utils::Pair<utils::Int, utils::Int>> &drawnEdges, const utils::Int first, const utils::Int second);

void printInteractionList(const utils::Vec<Qubit> &qubits);

} // namespace ql

#endif //WITH_VISUALIZER