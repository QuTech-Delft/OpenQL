#include "ql/ir/ir_gen_ex.h"

// These set of functions are used to disambiguate in cases where the compiler cannot decide
// between the operator== implementation and the synthesized version with reverse parameter order

namespace ql {
namespace ir {

bool operator==(const utils::Link<Object> &lhs, const utils::Link<Object> &rhs) {
    return (lhs->name == rhs->name &&
        lhs->data_type == rhs->data_type &&
        lhs->shape == rhs->shape);
}

bool operator==(const utils::Link<Object> &lhs, const utils::Link<PhysicalObject> &rhs) {
    return (lhs->name == rhs->name &&
            lhs->data_type == rhs->data_type &&
            lhs->shape == rhs->shape);
}

bool operator==(const utils::tree::base::One<Statement> &lhs, const utils::tree::base::One<Statement> &rhs) {
    return lhs.get_ptr() == rhs.get_ptr();
}

bool operator==(const utils::tree::base::One<Statement> &lhs, const utils::tree::base::One<SentinelStatement> &rhs) {
    return lhs.get_ptr() == rhs.get_ptr();
}

} // namespace ir
} // namespace ql
