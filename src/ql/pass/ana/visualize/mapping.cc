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
#ifdef WITH_VISUALIZER
    R"(
    Visualizes the incoming quantum circuit alongside the qubit grid for the
    target chip, showing the virtual-to-real qubit mapping as the circuit
    progresses. Note that the circuit is considered to be the concatenation of
    all the kernels in the program, regardless of any control-flow relationship.
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
    const ir::ProgramRef &program,
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
