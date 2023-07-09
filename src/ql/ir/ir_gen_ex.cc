#include "ql/ir/ir.gen.h"
#include "ql/utils/tree.h"

namespace ql {
namespace ir {

bool operator==(const utils::Link<Object> &lhs, const utils::Link<PhysicalObject> &rhs) {
    return (lhs->name == rhs->name &&
        lhs->data_type == rhs->data_type &&
        lhs->shape == rhs->shape);
}

} // namespace ir
} // namespace ql
