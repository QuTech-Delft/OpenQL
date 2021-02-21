/** \file
 * Definition of the visualizer mapping graph.
 */

#ifdef WITH_VISUALIZER

#include "visualizer.h"
#include "visualizer_cimg.h"
#include "visualizer_circuit.h"
#include "visualizer_mapping.h"

namespace ql {

using namespace utils;

void visualizeMappingGraph(const quantum_program* program, const VisualizerConfiguration &configuration) {
    QL_IOUT("Visualizing mapping graph...");

    // 1) get visualized circuit with extra wide cycles from visualizer_circuit.cc
    // 2) fill in cycle spaces beneath the circuit with the mapping graph

    Image image = generateImage(program, configuration).image;

    image.display("Mapping Graph");
}

} // namespace ql

#endif //WITH_VISUALIZER