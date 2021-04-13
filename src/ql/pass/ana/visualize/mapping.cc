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
    utils::dump_str(os, line_prefix, R"(
    TODO
    )");
}

/**
 * Constructs a mapping graph visualizer pass.
 */
VisualizeMappingPass::VisualizeMappingPass(
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
 * Runs the mapping graph visualizer.
 */
utils::Int VisualizeMappingPass::run(
    const ir::ProgramRef &program,
    const pmgr::pass_types::Context &context
) const {
#ifdef WITH_VISUALIZER
    detail::visualizeMappingGraph(
        program, {
            "MAPPING_GRAPH",
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

} // namespace mapping
} // namespace visualize
} // namespace ana
} // namespace pass
} // namespace ql
