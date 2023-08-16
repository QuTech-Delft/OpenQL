/** \file
 * Declaration of the visualizer mapping graph.
 */

#pragma once

#ifdef WITH_VISUALIZER

#include "ql/utils/num.h"
#include "ql/utils/vec.h"
#include "ql/utils/json.h"
#include "ql/ir/ir.h"
#include "types.h"

namespace ql {
namespace pass {
namespace ana {
namespace visualize {
namespace detail {

struct Edge {
    utils::Int src;
    utils::Int dst;

    Edge() = delete;
    Edge(utils::Int src_, utils::Int dst_)
        : src{ src_ }
        , dst{ dst_ }
    {}
};

struct Topology {
    utils::Int xSize = 0;
    utils::Int ySize = 0;
    utils::Vec<Position2> vertices;
    utils::Vec<Edge> edges;
};

void visualizeMappingGraph(const ir::Ref &ir, const VisualizerConfiguration &configuration);

void computeMappingPerCycle(const MappingGraphLayout &layout,
                            utils::Vec<utils::Vec<utils::Int>> &virtualQubits,
                            utils::Vec<utils::Bool> &mappingChangedPerCycle,
                            const utils::Vec<GateProperties> &gates,
                            utils::Int amountOfCycles,
                            utils::Int amountOfQubits);

utils::Bool parseTopology(const ir::PlatformRef &platform, Topology &topology);
MappingGraphLayout parseMappingGraphLayout(const utils::Str &configPath);

} // namespace detail
} // namespace visualize
} // namespace ana
} // namespace pass
} // namespace ql

#endif //WITH_VISUALIZER
