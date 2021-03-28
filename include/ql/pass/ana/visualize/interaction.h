/** \file
 * Declaration of the visualizer qubit interaction graph.
 */

#pragma once

// FIXME JvS: WITH_VISUALIZER must never appear in a public header file
#ifdef WITH_VISUALIZER

#include "ql/utils/num.h"
#include "ql/utils/str.h"
#include "ql/utils/pair.h"
#include "ql/utils/vec.h"
#include "ql/pass/ana/visualize/visualize.h"
#include "ql/pass/ana/visualize/types.h"

namespace ql {
namespace pass {
namespace ana {
namespace visualize {

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

} // namespace visualize
} // namespace ana
} // namespace pass
} // namespace ql

#endif //WITH_VISUALIZER
