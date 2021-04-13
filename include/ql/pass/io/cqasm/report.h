/** \file
 * Defines the cQASM writer pass.
 */

#pragma once

#include "ql/pmgr/pass_types.h"

namespace ql {
namespace pass {
namespace io {
namespace cqasm {
namespace report {

/**
 * cQASM writer pass.
 */
class ReportCQasmPass : public pmgr::pass_types::ProgramAnalysis {
protected:

    /**
     * Dumps docs for the cQASM writer.
     */
    void dump_docs(
        std::ostream &os,
        const utils::Str &line_prefix
    ) const override;

public:

    /**
     * Constructs a cQASM writer.
     */
    ReportCQasmPass(
        const utils::Ptr<const pmgr::PassFactory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Runs the cQASM writer.
     */
    utils::Int run(
        const ir::ProgramRef &program,
        const pmgr::pass_types::Context &context
    ) const override;

};

/**
 * Shorthand for referring to the pass using namespace notation.
 */
using Pass = ReportCQasmPass;

} // namespace report
} // namespace cqasm
} // namespace io
} // namespace pass
} // namespace ql
