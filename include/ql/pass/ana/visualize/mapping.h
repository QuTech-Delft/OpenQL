/** \file
 * Declaration of the visualizer mapping graph.
 */

#pragma once

// FIXME JvS: WITH_VISUALIZER must never appear in a public header file
#ifdef WITH_VISUALIZER

#include "ql/utils/num.h"
#include "ql/utils/vec.h"
#include "ql/utils/json.h"
#include "ql/pass/ana/visualize/visualize.h"
#include "ql/pass/ana/visualize/types.h"

namespace ql {
namespace pass {
namespace ana {
namespace visualize {

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

void visualizeMappingGraph(const ir::ProgramRef &program, const VisualizerConfiguration &configuration);

void computeMappingPerCycle(const MappingGraphLayout &layout,
                            utils::Vec<utils::Vec<utils::Int>> &virtualQubits,
                            utils::Vec<utils::Bool> &mappingChangedPerCycle,
                            const utils::Vec<GateProperties> &gates,
                            utils::Int amountOfCycles,
                            utils::Int amountOfQubits);

utils::Bool parseTopology(const utils::Json &hardware_settings, Topology &topology);
MappingGraphLayout parseMappingGraphLayout(const utils::Str &configPath);

} // namespace visualize
} // namespace ana
} // namespace pass
} // namespace ql

#endif //WITH_VISUALIZER
