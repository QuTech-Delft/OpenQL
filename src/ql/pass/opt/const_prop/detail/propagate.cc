/** \file
 * Recursively perform constant propagation on an IR node
 */

#include "propagate.h"
#include "platform_functions.h"

#include "ql/ir/describe.h"

namespace ql {
namespace pass {
namespace opt {
namespace const_prop {
namespace detail {

// type definitions for platform functions we implement here
using FncArgs = const utils::Any<ir::Expression>;
using FncRet = utils::One<ir::Expression>;


// create the functions from PLATFORM_FUNCTION_LIST
// the identifier consists of the concatenation of func, "_" and the (non macro expanded) parameters, e.g.
// "ap_add_P_IP_I". Doesn't look great, but you never see it anyway.
#define P_I as_int_literal()
#define P_B as_bit_literal()
#define R_I(x) utils::make<ir::IntLiteral>(x)   // FIXME: use make_int_lit ?
#define R_B(x) utils::make<ir::BitLiteral>(x)

#define X2(name, ret_type, par0, par1, operation, func)  \
static FncRet func ## _ ## par0 ## par1(FncArgs &v) {   \
    auto a = v[0]->par0->value;                         \
    auto b = v[1]->par1->value;                         \
    return ret_type(operation);                         \
}
#define X1(name, ret_type, par0, operation, func)       \
static FncRet func ## _ ## par0 (FncArgs &v) {          \
    auto a = v[0]->par0->value;                         \
    return ret_type(operation);                         \
}

PLATFORM_FUNCTION_LIST

#undef X1
#undef X2
#undef R_B
#undef R_I
#undef P_B
#undef P_I


class ConstantPropagator : public ir::RecursiveVisitor {
public:

    /**
     * Constructs a ConstantPropagator.
     */
    ConstantPropagator() = default;

private:  // Visitor overrides

    /**
     * Fallback function.
     */
    void visit_node(ir::Node &node) override {
        node.dump(std::cerr);
        QL_ICE("unexpected node type encountered in ConstantPropagator");
    }

    /**
     * Visitor function for `FunctionCall` nodes.
     */
    void visit_function_call(ir::FunctionCall &node) override {
        QL_IOUT("function call" << ir::describe(node));
    }

    /**
     * Visitor function for `IfElse` nodes.
     */
//    void visit_if_else(ir::IfElse &node) override {
//        QL_IOUT("if_else" << ir::describe(node));
//    }

private:    // types

    // function pointer for dispatch()
    typedef FncRet (*tOpFunc)(const FncArgs &a);

    using FuncMap = std::map<utils::Str, tOpFunc>;

private:    // vars
    FuncMap func_map;                                       // map name to function info, see register_functions()

private:    // functions

    /**
     * Register the functions we handle.
     */
    void register_functions() {
        // Initialize func_map with the functions from PLATFORM_FUNCTION_LIST
        // The key consists of the concatenation of name, "_" and the stringified profile, e.g "operator+_ii"

        #define str(x) #x
        #define xstr(x) str(x)
        #define P_I i
        #define P_B b
        #define X2(name, ret_type, par0, par1, operation, func) { name "_" xstr(par0) xstr(par1), &(func ## _ ## par0 ## par1) },
        #define X1(name, ret_type, par0,       operation, func) { name "_" xstr(par0)           , &(func ## _ ## par0        ) },
        func_map = {
            PLATFORM_FUNCTION_LIST
        };
        #undef X1
        #undef X2
        #undef P_B
        #undef P_I
        #undef xstr
        #undef str
    };

//    void dispatch(const ir::ExpressionRef &lhs, const ir::FunctionCall *fn, const Str &describe);

};

/**
 * Recursively perform constant propagation on an IR node.
 */
void propagate(ir::Node &node) {
    ConstantPropagator visitor;

    node.visit(visitor);
}

} // namespace detail
} // namespace const_prop
} // namespace opt
} // namespace pass
} // namespace ql
