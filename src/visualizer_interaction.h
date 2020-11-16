/**
 * @file   visualizer_interaction.h
 * @date   11/2020
 * @author Tim van der Meer
 * @brief  declaration of the visualizer qubit interaction graph
 */

#pragma once

#ifdef WITH_VISUALIZER

#include "visualizer.h"
#include "visualizer_types.h"

namespace ql {

struct InteractionsWithQubit {
    int qubitIndex;
    int amountOfInteractions;
};

struct Qubit {
    int qubitIndex;
    std::vector<InteractionsWithQubit> interactions;
};

void visualizeInteractionGraph(const ql::quantum_program* program);

double calculateQubitCircleRadius(const int qubitRadius, const double theta);
Position2 calculateQubitPosition(const int radius, const double theta, const Position2 center);
std::vector<Qubit> findQubitInteractions(const std::vector<GateProperties> gates, const int amountOfQubits);

void printInteractionList(const std::vector<Qubit> qubits);

} // namespace ql

#endif //WITH_VISUALIZER