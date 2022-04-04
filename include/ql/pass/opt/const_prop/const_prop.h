/** \file
 * Constant propagation pass.
 */

#pragma once

//#include "ql/com/dec/rules.h"
#include "ql/pmgr/pass_types/specializations.h"

namespace ql {
namespace pass {
namespace opt {
namespace const_prop {

using FncArgs = const utils::Any<ir::Expression>;
using FncRet = utils::One<ir::Expression>;

/**
 * Constant propagation pass.
 */
class ConstantPropagationPass : public pmgr::pass_types::Transformation {
public:

    /**
     * Constructs an constant propagator.
     */
    ConstantPropagationPass(
        const utils::Ptr<const pmgr::Factory> &pass_factory,
        const utils::Str &instance_name,
        const utils::Str &type_name
    );

    /**
     * Returns a user-friendly type name for this pass.
     */
    utils::Str get_friendly_type() const override;

    /**
     * Runs the constant propagator.
     */
    utils::Int run(
        const ir::Ref &ir,
        const pmgr::pass_types::Context &context
    ) const override;

protected:

    /**
     * Dumps docs for constant propagator.
     */
    void dump_docs(
        std::ostream &os,
        const utils::Str &line_prefix
    ) const override;

private:

    /**
     * Runs the constant propagator on the given block.
     */
    static void run_on_block(
        const ir::Ref &ir,
        const ir::BlockBaseRef &block
    );

    /**
     * Register the functions we handle.
     */
    void register_functions();

//    void dispatch(const ir::ExpressionRef &lhs, const ir::FunctionCall *fn, const Str &describe);

private:    // types

    // function pointer for dispatch()
    typedef FncRet (*tOpFunc)(const FncArgs &a);

    using FuncMap = std::map<utils::Str, tOpFunc>;


private:    // vars
    FuncMap func_map;                                       // map name to function info, see register_functions()

};

/**
 * Shorthand for referring to the pass using namespace notation.
 */
using Pass = ConstantPropagationPass;

} // namespace const_prop
} // namespace opt
} // namespace pass
} // namespace ql
