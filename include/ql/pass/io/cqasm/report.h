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
 * Dumps cQASM code for the given program to the given output stream.
 * Optionally, the after_kernel callback function may be used to dump additional
 * information at the end of each kernel. The third argument specifies the
 * appropriate line prefix for correctly-indented comments.
 */
void dump(
    const ir::ProgramRef &program,
    std::ostream &os = std::cout,
    std::function<void(const ir::KernelRef&, std::ostream&, const utils::Str&)> after_kernel
        = [](const ir::KernelRef&, std::ostream&, const utils::Str&){}
);

/**
 * Specialization of dump() that includes statistics per kernel and program in
 * comments.
 */
void dump_with_statistics(
    const ir::ProgramRef &program,
    std::ostream &os = std::cout
);

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
