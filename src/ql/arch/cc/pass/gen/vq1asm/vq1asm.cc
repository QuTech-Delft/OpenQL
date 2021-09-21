/** \file
 * Defines the QuTech Central Controller Q1 processor assembly generator pass.
 */

#include "ql/arch/cc/pass/gen/vq1asm/vq1asm.h"

#include "ql/pmgr/pass_types/base.h"
#include "detail/backend.h"

namespace ql {
namespace arch {
namespace cc {
namespace pass {
namespace gen {
namespace vq1asm {

/**
 * Dumps docs for the code generator.
 */
void GenerateVQ1AsmPass::dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    utils::dump_str(os, line_prefix, R"(
    Assembly code generator for the Q1 processor in the QuTech Central
    Controller, version )" CC_BACKEND_VERSION_STRING R"(

    This pass actually generates three files:
     - `<prefix>.vq1asm`: the assembly code output file;
     - `<prefix>.map`: the instrument configuration file; and
     - `<prefix>.vcd`: a VCD (value change dump) file for viewing the waveforms
       that the program outputs.

    The pass is compile-time configured with the following options:
     - `OPT_CC_SCHEDULE_RC` = )"           + utils::to_string(OPT_CC_SCHEDULE_RC)           + R"(
     - `OPT_SUPPORT_STATIC_CODEWORDS` = )" + utils::to_string(OPT_SUPPORT_STATIC_CODEWORDS) + R"(
     - `OPT_STATIC_CODEWORDS_ARRAYS` = )"  + utils::to_string(OPT_STATIC_CODEWORDS_ARRAYS)  + R"(
     - `OPT_VECTOR_MODE` = )"              + utils::to_string(OPT_VECTOR_MODE)              + R"(
     - `OPT_FEEDBACK` = )"                 + utils::to_string(OPT_FEEDBACK)                 + R"(
     - `OPT_PRAGMA` = )"                   + utils::to_string(OPT_PRAGMA)                   + R"(
    )");
}

/**
 * Returns a user-friendly type name for this pass.
 */
utils::Str GenerateVQ1AsmPass::get_friendly_type() const {
    return "Central Controller code generator";
}

/**
 * Constructs a code generator.
 */
GenerateVQ1AsmPass::GenerateVQ1AsmPass(
    const utils::Ptr<const pmgr::Factory> &pass_factory,
    const utils::Str &instance_name,
    const utils::Str &type_name
) : pmgr::pass_types::Transformation(pass_factory, instance_name, type_name) {

    options.add_str(
        "map_input_file",
        "Specifies the input map filename."
    );

    options.add_bool(
        "verbose",
        "Selects whether verbose comments should be added to the generated "
        ".vq1asm file.",
        true
    );

    options.add_bool(
        "run_once",
        "When set, the emitted .vq1asm program runs once instead of repeating "
        "indefinitely."
    );

}

/**
 * Runs the code generator.
 */
utils::Int GenerateVQ1AsmPass::run(
    const ir::Ref &ir,
    const pmgr::pass_types::Context &context
) const {

#if 0   // FIXME
    // Make sure that the incoming code is scheduled, as expected.
    for (const auto &kernel : program->kernels) {
        if (!kernel->cycles_valid) {
            throw utils::Exception(
                "The code going into the CC backend must be scheduled, but isn't!"
            );
        }
    }
#endif

    // Parse the options.
    auto parsed_options = utils::Ptr<detail::Options>::make();
    parsed_options->output_prefix = context.output_prefix;
    parsed_options->map_input_file = options["map_input_file"].as_str();
    parsed_options->run_once = options["run_once"].as_bool();
    parsed_options->verbose = options["verbose"].as_bool();

    // Run the backend.
    QL_DOUT("Running Central Controller backend ... ");
    detail::Backend(ir, parsed_options.as_const());

    return 0;
}

} // namespace vq1asm
} // namespace gen
} // namespace pass
} // namespace cc
} // namespace arch
} // namespace ql
