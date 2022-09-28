/** \file
 * Defines the interaction graph visualizer pass.
 */

#include "ql/pass/ana/visualize/interaction.h"

#include "ql/pass/ana/visualize/detail/interaction.h"
#include "ql/pmgr/factory.h"

namespace ql {
namespace pass {
namespace ana {
namespace visualize {
namespace interaction {

bool VisualizeInteractionPass::is_pass_registered = pmgr::Factory::register_pass<VisualizeInteractionPass>("ana.visualize.Interaction");

/**
 * Dumps docs for the interaction graph visualizer.
 */
void VisualizeInteractionPass::dump_docs(
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
    The qubit interaction graph visualizes the interactions between each of the
    qubits in the circuit. If a gate acts on two or more qubits, those qubits
    interact with each other and an edge will be drawn in the graph, with a
    number indicating the amount of times those qubits have interacted with each
    other. Note that the visualization of this is very simple, and the DOT graph
    the visualizer can produce should be used with the user's favorite graphing
    software to create a better looking graph.

    * Configuration file structure *

      The visualizer is configured by way of the visualizer configuration file.
      Each attribute has a default setting, so many can be omitted if no change
      is wanted.

      The circuit visualizer supports the following top-level sections:

       - `"interactionGraph"`: contains options for the interaction graph.
       - `"saveImage"`: a boolean indicating whether the generated image should
         be saved to disk. When this is true, the file will be saved regardless
         of/in addition to the interactive window as controlled by the
         `interactive` option.
       - `"backgroundColor"`: the background color of the generated image.

      NOTE: a single visualizer configuration file may be used for all three
      visualization pass types. The configuration file format is designed to be
      cross-compatible.

      The `"interactionGraph"` section should have the following structure.

      ```javascript
      "interactionGraph": {
          // whether a DOT file should be generated for use with graphing
          // software
          "outputDotFile": true,
          "borderWidth": 32,
          // the minimum radius of the circle on which the qubits are placed
          "minInteractionCircleRadius": 100,
          "interactionCircleRadiusModifier": 3.0,
          "qubitRadius": 17,
          "labelFontHeight": 13,
          "circleOutlineColor": [0, 0, 0],
          "circleFillColor": [255, 255, 255],
          "labelColor": [0, 0, 0],
          "edgeColor": [0, 0, 0]
      }
      ```
    )");
}

/**
 * Returns a user-friendly type name for this pass.
 */
utils::Str VisualizeInteractionPass::get_friendly_type() const {
    return "Qubit interaction graph visualizer";
}

/**
 * Constructs a interaction graph visualizer pass.
 */
VisualizeInteractionPass::VisualizeInteractionPass(
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
 * Runs the interaction graph visualizer.
 */
utils::Int VisualizeInteractionPass::run(
    const ir::compat::ProgramRef &program,
    const pmgr::pass_types::Context &context
) const {
#ifdef WITH_VISUALIZER
    detail::visualizeInteractionGraph(
        program, {
            "INTERACTION_GRAPH",
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

} // namespace interaction
} // namespace visualize
} // namespace ana
} // namespace pass
} // namespace ql
