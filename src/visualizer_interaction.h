/** \file
 * Declaration of the visualizer qubit interaction graph.
 */

#pragma once

#ifdef WITH_VISUALIZER

#include "utils/num.h"
#include "utils/str.h"
#include "utils/vec.h"
#include "visualizer.h"
#include "visualizer_common.h"

namespace ql {

struct InteractionsWithQubit {
    utils::Int qubitIndex;
    utils::Int amountOfInteractions;
};

struct Qubit {
    utils::Int qubitIndex;
    utils::Vec<InteractionsWithQubit> interactions;
};

void visualizeInteractionGraph(const utils::Vec<GateProperties> &gates);

utils::Real calculateQubitCircleRadius(utils::Int qubitRadius, utils::Real theta);
Position2 calculateQubitPosition(utils::Int radius, utils::Real theta, const Position2 &center);
utils::Vec<Qubit> findQubitInteractions(const utils::Vec<GateProperties> &gates, utils::Int amountOfQubits);

void printInteractionList(const utils::Vec<Qubit> &qubits);

} // namespace ql

#endif //WITH_VISUALIZER