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

    // Get visualized circuit with extra wide cycles from visualizer_circuit.cc.
    //TODO: give parameters that increase the size of the image so that there is space for the mapping graph
    const Int minCycleWidth = 0;
    const Int extendedImageHeight = 0;
    Image image = generateImage(program, configuration, minCycleWidth, extendedImageHeight).image;

    // Fill in cycle spaces beneath the circuit with the mapping graph.

    // Display the filled in image.
    image.display("Mapping Graph");
}

} // namespace ql

#endif //WITH_VISUALIZER