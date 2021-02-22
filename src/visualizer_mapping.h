/** \file
 * Declaration of the visualizer mapping graph.
 */

#pragma once

#ifdef WITH_VISUALIZER

#include "visualizer.h"
#include "visualizer_types.h"

namespace ql {

void visualizeMappingGraph(const quantum_program* program, const VisualizerConfiguration &configuration);

MappingGraphLayout parseMappingGraphLayout(const utils::Str &configPath);

} // namespace ql

#endif //WITH_VISUALIZER