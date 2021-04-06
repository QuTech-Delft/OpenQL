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
    os << line_prefix << "TODO" << std::endl;
}

/**
 * Constructs a interaction graph visualizer pass.
 */
VisualizeInteractionPass::VisualizeInteractionPass(
    const utils::Ptr<const pmgr::PassFactory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : ProgramAnalysis(pass_factory, instance_name, type_name) {
    options.add_str(
        "config",
        "path to the visualizer configuration file",
        "visualizer_config.json"
    );
    options.add_str(
        "waveform_mapping",
        "path to the visualizer waveform mapping file",
        "waveform_mapping.json"
    );
}

/**
 * Runs the interaction graph visualizer.
 */
utils::Int VisualizeInteractionPass::run(
    const plat::PlatformRef &platform,
    const ir::ProgramRef &program,
    const utils::Str &full_name
) const {
#ifdef WITH_VISUALIZER
    detail::visualizeInteractionGraph(
        program, {
            "INTERACTION_GRAPH",
            options["config"].as_str(),
            options["waveform_mapping"].as_str()
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
