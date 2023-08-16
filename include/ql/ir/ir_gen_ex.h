#include "ql/ir/ir.gen.h"  // Object, PhysicalObject
#include "ql/utils/tree.h"  // Link

namespace ql {
namespace ir {

bool operator==(const utils::Link<Object> &lhs, const utils::Link<Object> &rhs);
bool operator==(const utils::Link<Object> &lhs, const utils::Link<PhysicalObject> &rhs);
bool operator==(const utils::tree::base::One<Statement> &lhs, const utils::tree::base::One<Statement> &rhs);
bool operator==(const utils::tree::base::One<Statement> &lhs, const utils::tree::base::One<SentinelStatement> &rhs);

} // namespace ir
} // namespace ql
