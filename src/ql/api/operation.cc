/** \file
 * API header for using classical operations.
 */

#include "ql/api/operation.h"

//============================================================================//
//                               W A R N I N G                                //
//----------------------------------------------------------------------------//
//         Docstrings in this file must manually be kept in sync with         //
//   python/openql.i! This should be automated at some point, but isn't yet.  //
//============================================================================//

namespace ql {
namespace api {

/**
 * Creates a classical binary operation between two classical registers. The
 * operation is specified as a string, of which the following are supported:
 *  - "+": addition.
 *  - "-": subtraction.
 *  - "&": bitwise AND.
 *  - "|": bitwise OR.
 *  - "^": bitwise XOR.
 *  - "==": equality.
 *  - "!=": inequality.
 *  - ">": greater-than.
 *  - ">=": greater-or-equal.
 *  - "<": less-than.
 *  - "<=": less-or-equal.
 */
Operation::Operation(const CReg &lop, const std::string &op, const CReg &rop) {
    operation.emplace(*(lop.creg), op, *(rop.creg));
}

/**
 * Creates a classical unary operation on a register. The operation is
 * specified as a string, of which currently only "~" (bitwise NOT) is
 * supported.
 */
Operation::Operation(const std::string &op, const CReg &rop) {
    operation.emplace(op, *(rop.creg));
}

/**
 * Creates a classical "operation" that just returns the value of the given
 * register.
 */
Operation::Operation(const CReg &lop) {
    operation.emplace(*(lop.creg));
}

/**
 * Creates a classical "operation" that just returns the given integer
 * value.
 */
Operation::Operation(int val) {
    operation.emplace(val);
}

} // namespace api
} // namespace ql
