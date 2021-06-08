/** \file
 * Defines the mapping graph visualizer pass.
 */

#include "ql/pass/ana/visualize/mapping.h"

#include "detail/mapping.h"

namespace ql {
namespace pass {
namespace ana {
namespace visualize {
namespace mapping {

/**
 * Dumps docs for the mapping graph visualizer.
 */
void VisualizeMappingPass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix,
#ifndef WITH_VISUALIZER
    R"(
    NOTE*: the visualizer was not compiled into the build of OpenQL that
    generated this documentation. If this was not intended, and OpenQL is
    running on Linux or Mac, the X11 library development headers might be
    missing and the visualizer has disabled itself.

    )"
#endif
    R"(
    The mapping graph tracks the journey of the virtual qubits through the real
    topology of the quantum hardware as the cycles of the quantum program are
    executed. The virtual qubits change location whenever a swap/move gate (or
    their decomposed parts) is finished executing. For convenience, the abstract
    circuit representation of the quantum program is shown above the qubit
    mappings for each cycle.

    The topology of the quantum hardware is taken from the topology section in
    the hardware configuration file, together with the edges between the qubits.
    If no coordinates and/or edges are defined for the qubits, the qubits will
    simply be spaced sequentially in a grid structure without edges being shown.

    * Configuration file structure *

      The visualizer is configured by way of the visualizer configuration file.
      Each attribute has a default setting, so many can be omitted if no change
      is wanted.

      The circuit visualizer supports the following top-level sections:

       - `"mappingGraph"`: contains options for the mapping graph.
       - `"saveImage"`: a boolean indicating whether the generated image should
         be saved to disk. When this is true, the file will be saved regardless
         of/in addition to the interactive window as controlled by the
         `interactive` option.
       - `"backgroundColor"`: the background color of the generated image.

      NOTE: a single visualizer configuration file may be used for all three
      visualization pass types. The configuration file format is designed to be
      cross-compatible.

      The `"mappingGraph"` section should have the following structure.

      ```javascript
      "mappingGraph": {
          // whether qubits should be filled with the corresponding logical
          // qubit index in the first cycle
          "initDefaultVirtuals": false,
          // give each distinct virtual qubit a color
          "showVirtualColors": true,
          // show the real qubit indices above the qubits
          "showRealIndices": true,
          // whether to use the topology from the hardware configuration file
          "useTopology": true,
          // parameters for controlling the layout
          "qubitRadius": 15,
          "qubitSpacing": 7,
          "fontHeightReal": 13,
          "fontHeightVirtual": 13,
          "textColorReal": [0, 0, 255],
          "textColorVirtual": [255, 0, 0],
          // the gap between the qubit and the real index
          "realIndexSpacing": 1,
          "qubitFillColor": [255, 255, 255],
          "qubitOutlineColor": [0, 0, 0]
      }
      ```
    )");
}

/**
 * Returns a user-friendly type name for this pass.
 */
utils::Str VisualizeMappingPass::get_friendly_type() const {
    return "Qubit mapping graph visualizer";
}

/**
 * Constructs a mapping graph visualizer pass.
 */
VisualizeMappingPass::VisualizeMappingPass(
    const utils::Ptr<const pmgr::Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : ProgramAnalysis(pass_factory, instance_name, type_name) {
    options.add_str(
        "config",
        "Path to the visualizer configuration file.",
        "visualizer_config.json"
    );
    options.add_bool(
        "interactive",
        "When yes, the visualizer will open a window when the pass is run. "
        "When no, an image will be saved as <output_prefix>.bmp instead."
    );
}

/**
 * Runs the mapping graph visualizer.
 */
utils::Int VisualizeMappingPass::run(
    const ir::compat::ProgramRef &program,
    const pmgr::pass_types::Context &context
) const {
#ifdef WITH_VISUALIZER
    detail::visualizeMappingGraph(
        program, {
            "MAPPING_GRAPH",
            options["config"].as_str(),
            "", // unused
            options["interactive"].as_bool(),
            context.output_prefix,
            context.full_pass_name
        }
    );
    return 0;
#else
    QL_EOUT(
        "The visualizer was disabled during compilation of OpenQL. If this was "
        "not intended, and OpenQL is running on Linux or Mac, the X11 library "
        "development headers might be missing and the visualizer has disabled "
        "itself."
    );
    return -1;
#endif
}

} // namespace mapping
} // namespace visualize
} // namespace ana
} // namespace pass
} // namespace ql
