/** \file
 * Defines the circuit visualization pass.
 */

#include "ql/pmgr/pass_types.h"

namespace ql {
namespace pass {
namespace ana {
namespace visualize {
namespace circuit {

/**
 * Circuit visualizer pass.
 */
class VisualizeCircuitPass : public pmgr::pass_types::ProgramAnalysis {
protected:

    /**
     * Dumps docs for the circuit visualizer.
     */
    void dump_docs(
        std::ostream &os,
        const utils::Str &line_prefix
    ) const override;

public:

    /**
     * Constructs a circuit visualizer pass.
     */
    VisualizeCircuitPass(
        const utils::Ptr<const pmgr::PassFactory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Runs the circuit visualizer.
     */
    utils::Int run(
        const plat::PlatformRef &platform,
        const ir::ProgramRef &program,
        const utils::Str &full_name
    ) const override;

};

/**
 * Shorthand for referring to the pass using namespace notation.
 */
using Pass = VisualizeCircuitPass;

} // namespace circuit
} // namespace visualize
} // namespace ana
} // namespace pass
} // namespace ql
