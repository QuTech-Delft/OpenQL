#include "ql/ir/ir.gen.h"  // Object, PhysicalObject
#include "ql/utils/tree.h"  // Link

namespace ql {
namespace ir {

bool operator==(const utils::Link<Object> &lhs, const utils::Link<PhysicalObject> &rhs);

} // namespace ir
} // namespace ql
