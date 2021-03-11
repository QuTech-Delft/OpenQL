/** \file
 * Declaration of the visualizer mapping graph.
 */

#pragma once

#ifdef WITH_VISUALIZER

#include "visualizer.h"
#include "visualizer_types.h"
#include "utils/vec.h"
#include "utils/json.h"
#include "utils/num.h"

namespace ql {

struct Edge {
    utils::Int src;
    utils::Int dst;

    Edge() = delete;
};

struct Topology {
    utils::Int xSize = 0;
    utils::Int ySize = 0;
    utils::Vec<Position2> vertices;
    utils::Vec<Edge> edges;
};

void visualizeMappingGraph(const quantum_program* program, const VisualizerConfiguration &configuration);

void computeMappingPerCycle(const MappingGraphLayout layout,
                            utils::Vec<utils::Vec<utils::Int>> &virtualQubits,
                            utils::Vec<utils::Bool> &mappingChangedPerCycle,
                            const utils::Vec<GateProperties> &gates,
                            const utils::Int amountOfCycles,
                            const utils::Int amountOfQubits);

utils::Bool parseTopology(const utils::Json hardware_settings, Topology &topology);
MappingGraphLayout parseMappingGraphLayout(const utils::Str &configPath);

} // namespace ql

#endif //WITH_VISUALIZER