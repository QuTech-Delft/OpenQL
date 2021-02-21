/** \file
 * Declaration of the visualizer mapping graph.
 */

#pragma once

#ifdef WITH_VISUALIZER

#include "visualizer.h"

namespace ql {

void visualizeMappingGraph(const quantum_program* program, const VisualizerConfiguration &configuration);

} // namespace ql

#endif //WITH_VISUALIZER