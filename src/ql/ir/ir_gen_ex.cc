#include "ql/ir/ir.gen.h"

namespace ql {
namespace ir {

bool operator==(const Object &lhs, const PhysicalObject &rhs) {
    return (lhs.name == rhs.name &&
        lhs.data_type == rhs.data_type &&
        lhs.shape == rhs.shape);
}

} // namespace ir
} // namespace ql
