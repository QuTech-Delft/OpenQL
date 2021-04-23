/** \file
 * Defines the circuit visualizer pass.
 */

#include "ql/pass/ana/visualize/circuit.h"

#include "detail/circuit.h"

namespace ql {
namespace pass {
namespace ana {
namespace visualize {
namespace circuit {

/**
 * Dumps docs for the circuit visualizer.
 */
void VisualizeCircuitPass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix,
#ifdef WITH_VISUALIZER
    R"(
    Visualizes the incoming quantum circuit alongside any configured waveform
    output. Note that the circuit is considered to be the concatenation of
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
utils::Str VisualizeCircuitPass::get_friendly_type() const {
    return "Circuit visualizer";
}

/**
 * Constructs a circuit visualizer pass.
 */
VisualizeCircuitPass::VisualizeCircuitPass(
    const utils::Ptr<const pmgr::Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : ProgramAnalysis(pass_factory, instance_name, type_name) {
    options.add_str(
        "config",
        "Path to the visualizer configuration file.",
        "visualizer_config.json"
    );
    options.add_str(
        "waveform_mapping",
        "Path to the visualizer waveform mapping file.",
        "waveform_mapping.json"
    );
    options.add_bool(
        "interactive",
        "When yes, the visualizer will open a window when the pass is run. "
        "When no, an image will be saved as <output_prefix>.bmp instead."
    );
}

/**
 * Runs the circuit visualizer.
 */
utils::Int VisualizeCircuitPass::run(
    const ir::ProgramRef &program,
    const pmgr::pass_types::Context &context
) const {
#ifdef WITH_VISUALIZER
    detail::visualizeCircuit(
        program, {
            "CIRCUIT",
            options["config"].as_str(),
            options["waveform_mapping"].as_str(),
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

} // namespace circuit
} // namespace visualize
} // namespace ana
} // namespace pass
} // namespace ql
