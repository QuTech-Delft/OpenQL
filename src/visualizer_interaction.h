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

namespace ql {

struct InteractionsWithQubit {
    utils::Int qubitIndex;
    utils::Int amountOfInteractions;
};

struct Qubit {
    utils::Int qubitIndex;
    utils::Vec<InteractionsWithQubit> interactions;
};

void visualizeInteractionGraph(const ql::quantum_program* program, const VisualizerConfiguration &configuration);

void generateAndSaveDOTFile(const utils::Vec<Qubit> &qubits);

InteractionGraphLayout parseInteractionGraphLayout(const Str &configPath);

utils::Real calculateQubitCircleRadius(utils::Int qubitRadius, utils::Real theta);
Position2 calculatePositionOnCircle(utils::Int radius, utils::Real theta, const Position2 &center);
utils::Vec<Qubit> findQubitInteractions(const utils::Vec<GateProperties> &gates, utils::Int amountOfQubits);

bool isEdgeAlreadyDrawn(const utils::Vec<std::pair<utils::Int, utils::Int>> &drawnEdges, utils::Int first, utils::Int second);

void printInteractionList(const utils::Vec<Qubit> &qubits);

} // namespace ql

#endif //WITH_VISUALIZER