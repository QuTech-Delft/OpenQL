/** \file
 * Defines the cQASM writer pass.
 */

#pragma once

#include "ql/pmgr/pass_types/specializations.h"

#include <functional>

namespace ql {
namespace pass {
namespace io {
namespace cqasm {
namespace report {

/**
 * cQASM writer pass.
 */
class ReportCQasmPass : public pmgr::pass_types::Analysis {
    static bool is_pass_registered;
    
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
     * Returns a user-friendly type name for this pass.
     */
    utils::Str get_friendly_type() const override;

    /**
     * Constructs a cQASM writer.
     */
    ReportCQasmPass(
        const utils::Ptr<const pmgr::Factory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Runs the cQASM writer.
     */
    utils::Int run(
        const ir::Ref &ir,
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
