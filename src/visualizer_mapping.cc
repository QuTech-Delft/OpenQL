/** \file
 * Definition of the visualizer mapping graph.
 */

#ifdef WITH_VISUALIZER

#include "visualizer.h"
#include "visualizer_types.h"
#include "visualizer_common.h"
#include "visualizer_cimg.h"
#include "visualizer_circuit.h"
#include "visualizer_mapping.h"
#include "utils/json.h"
#include "utils/num.h"
#include "utils/vec.h"

namespace ql {

using namespace utils;

void visualizeMappingGraph(const quantum_program* program, const VisualizerConfiguration &configuration) {
    QL_IOUT("Visualizing mapping graph...");

    // Parse the layout and gate vector.
    const MappingGraphLayout layout = parseMappingGraphLayout(configuration.visualizerConfigPath);
    const Vec<GateProperties> gates = parseGates(program);

    // Get visualized circuit with extra wide cycles from visualizer_circuit.cc.
    const Int amountOfQubits = calculateAmountOfBits(gates, &GateProperties::operands);
    const Int amountOfLines = ceil(sqrt(amountOfQubits));
    const Int minCycleWidth = amountOfLines * layout.qubitRadius + (amountOfLines - 1) * layout.qubitSpacing;
    Image image = generateImage(program, configuration, minCycleWidth, minCycleWidth).image;

    // Fill in cycle spaces beneath the circuit with the mapping graph.


    // Display the filled in image.
    image.display("Mapping Graph");
}

MappingGraphLayout parseMappingGraphLayout(const Str &configPath) {
    QL_DOUT("Parsing visualizer configuration file for mapping graph visualization...");

    Json fullConfig;
    try {
        fullConfig = load_json(configPath);
    } catch (Json::exception &e) {
        QL_FATAL("Failed to load the visualization config file: \n\t" << std::string(e.what()));
    }

    Json config;
    if (fullConfig.count("mappingGraph") == 1) {
        config = fullConfig["mappingGraph"];
    } else {
        QL_WOUT("Could not find mapping graph configuration in visualizer configuration file. Is it named correctly?");
    }

    // Fill the layout object with the values from the config file. Any missing values will assume the default values hardcoded in the layout object.
    MappingGraphLayout layout;

    // Check if the image should be saved to disk.
    if (fullConfig.count("saveImage") == 1) {
        layout.saveImage = fullConfig["saveImage"];
    }

    // Load the parameters.
    if (config.count("qubitRadius") == 1)  layout.setQubitRadius(config["qubitRadius"]);
    if (config.count("qubitSpacing") == 1) layout.setQubitSpacing(config["qubitSpacing"]);

    return layout;
}

} // namespace ql

#endif //WITH_VISUALIZER