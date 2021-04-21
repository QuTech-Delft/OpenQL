/** \file
 * Defines the interaction graph visualizer pass.
 */

#include "ql/pass/ana/visualize/interaction.h"

#include "detail/interaction.h"

namespace ql {
namespace pass {
namespace ana {
namespace visualize {
namespace interaction {

/**
 * Dumps docs for the interaction graph visualizer.
 */
void VisualizeInteractionPass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix,
#ifdef WITH_VISUALIZER
    R"(
    Visualizes the qubit interaction graph for the entire program.
    )"
#else
    R"(
    The visualizer was not compiled into this build of OpenQL. If this was
    not intended, and OpenQL is running on Linux or Mac, the X11 library
    development headers might be missing and the visualizer has disabled itself.
    )"
#endif
    );
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
    const ir::ProgramRef &program,
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
