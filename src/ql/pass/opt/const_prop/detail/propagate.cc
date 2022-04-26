/** \file
 * Recursively perform constant propagation on an IR node
 */

#define DEBUG(x) QL_DOUT(x)

#include "propagate.h"
#include "platform_functions.h"

#include "ql/ir/describe.h"
#include "ql/ir/ops.h"

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
#define R_I(ir, x) make_int_lit(ir, x)   // NB: performs checking against IR types (thus disallowing integer overflow)
#define R_B(ir, x) make_bit_lit(ir, x)   // NB: performs checking against IR types

#define X2(name, ret_type, par0, par1, operation, func)                     \
static FncRet func ## _ ## par0 ## par1(const ir::Ref &ir, FncArgs &args) { \
    auto a = args[0]->par0->value;                                          \
    auto b = args[1]->par1->value;                                          \
    return ret_type(ir, operation);                                         \
}
#define X1(name, ret_type, par0, operation, func)                           \
static FncRet func ## _ ## par0 (const ir::Ref &ir, FncArgs &args) {        \
    auto a = args[0]->par0->value;                                          \
    return ret_type(ir, operation);                                         \
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
    explicit ConstantPropagator(const ir::Ref &ir) : ir(ir) {
        register_functions();
    };

private:  // RecursiveVisitor overrides

    /**
     * Fallback function.
     */
    void visit_node(ir::Node &node) override {
        // Note that in our case it is not an error to come here, since we don't intend to handle all
        // node types
        DEBUG("visiting node '"  << ir::describe(node) << "'");
    }

    /**
     * Visitor function for `SetInstruction` nodes.
     */

    void visit_set_instruction(ir::SetInstruction &set_instruction) override {
        handle_expression(set_instruction.rhs);
    }

    /**
     * Visitor function for `DynamicLoop` nodes.
     */

    void visit_dynamic_loop(ir::DynamicLoop &dynamic_loop) override {
        handle_expression(dynamic_loop.condition);

        // descend loop body
        dynamic_loop.body->visit(*this);
    }

    /**
     * Visitor function for `IfElseBranch` nodes.
     */
    void visit_if_else_branch(ir::IfElseBranch &if_else_branch) override {
        handle_expression(if_else_branch.condition);

        // descend branch body
        if_else_branch.body->visit(*this);
    }

    /**
     * Visitor function for `FunctionCall` nodes.
     */
    void visit_function_call(ir::FunctionCall &function_call) override {
        DEBUG("recursing into operands of function call '" << ir::describe(function_call) << "'");
        for (auto &operand : function_call.operands) {
            handle_expression(operand);
        }
    }

    /**
     * Visitor function for `FunctionCall` nodes.
     */
    void visit_expression(ir::Expression &expression) override {
        if(auto function_call = expression.as_function_call()) {
            DEBUG("recursing into function call '" << ir::describe(*function_call) << "'");
            visit_function_call(*function_call);   // descend
        }
    }

private:    // types

    // function pointer
    typedef FncRet (*tOpFunc)(const ir::Ref &ir, const FncArgs &args);

    // map function key (see register_functions()) to function pointer
    using FuncMap = std::map<utils::Str, tOpFunc>;

private:    // vars
    const ir::Ref &ir;
    FuncMap func_map;

private:    // functions

    /*
     * Handle an expression node, i.e. replace eligible functions calls with a literal expression.
     *
     * Note that we cannot directly use visit_expression(), because that has a 'ir::Expression &' as parameter.
     * It is therefore not possible to change a function_call into a (say) int_literal. Here the expression is
     * wrapped in a 'utils<One>', allowing polymorphic access. This does force us however to visit all relevant node
     * types that contain an expression (although we can skip e.g. custom_instruction.operands).
     */
    void handle_expression(utils::One<ir::Expression> &expression) {
        DEBUG("descending '" << ir::describe(expression));
        visit_expression(*expression);    // descend
        DEBUG("done descending '" << ir::describe(expression));

        if (auto function_call = expression->as_function_call()) {
            QL_IOUT("function call '" << ir::describe(*function_call) << "'");

            // generate key, consistent with register_functions()
            utils::Str key = function_call->function_type->name + "_";
            for (auto &operand : function_call->operands) {
                if (operand->as_int_literal()) {
                    key += "i";
                } else if (operand->as_bit_literal()) {
                    key += "b";
                } else {
                    DEBUG("not touching operand '" << ir::describe(operand) << "'");
                    return;     // we don't handle functions with other operand types, but leave them untouched
                }
            }

            // NB: we don't perform type promotions like libqasm's cQASM resolver, see FunctionTable::call

            // lookup key
            auto it = func_map.find(key);
            if (it == func_map.end()) {
                DEBUG("ignoring non-eligible function '" << key << "'");

            } else {
                // call the function
                FncRet ret = (*it->second)(ir, function_call->operands);

                // replace node
                QL_IOUT("replacing '" << ir::describe(*function_call) << "' by '" << ir::describe(*ret) << "'");
                expression = ret;
            }
        }
    }

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

};

/**
 * Recursively perform constant propagation on an IR node.
 */
void propagate(const ir::Ref &ir, ir::Node &node) {
    ConstantPropagator visitor(ir);

    node.visit(visitor);
}

} // namespace detail
} // namespace const_prop
} // namespace opt
} // namespace pass
} // namespace ql
