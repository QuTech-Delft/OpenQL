/** \file
 * Defines the cQASM reader pass.
 */

#pragma once

#include "ql/utils/str.h"
#include "ql/utils/json.h"
#include "ql/utils/opt.h"
#include "ql/pmgr/pass_types/specializations.h"

namespace ql {
namespace pass {
namespace io {
namespace cqasm {
namespace read {

/**
 * cQASM reader pass.
 */
class ReadCQasmPass : public pmgr::pass_types::Transformation {
protected:

    /**
     * Dumps docs for the cQASM reader.
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
     * Constructs a cQASM reader.
     */
    ReadCQasmPass(
        const utils::Ptr<const pmgr::Factory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Runs the cQASM reader.
     */
    utils::Int run(
        const ir::Ref &ir,
        const pmgr::pass_types::Context &context
    ) const override;

};

/**
 * Shorthand for referring to the pass using namespace notation.
 */
using Pass = ReadCQasmPass;

} // namespace reader
} // namespace cqasm
} // namespace io
} // namespace pass
} // namespace ql
