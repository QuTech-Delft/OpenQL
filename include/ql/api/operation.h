/** \file
 * API header for using classical operations.
 */

#pragma once

#include "ql/ir/ir.h"
#include "ql/api/declarations.h"
#include "ql/api/creg.h"

//============================================================================//
//                               W A R N I N G                                //
//----------------------------------------------------------------------------//
//         Docstrings in this file must manually be kept in sync with         //
//     operation.i! This should be automated at some point, but isn't yet.    //
//============================================================================//

namespace ql {
namespace api {

/**
 * Represents a classical operation.
 */
class Operation {
private:
    friend class Kernel;
    friend class Program;

    /**
     * The wrapped classical operation object.
     */
    ql::utils::Ptr<ql::ir::ClassicalOperation> operation;

public:

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
    Operation(const CReg &lop, const std::string &op, const CReg &rop);

    /**
     * Creates a classical unary operation on a register. The operation is
     * specified as a string, of which currently only "~" (bitwise NOT) is
     * supported.
     */
    Operation(const std::string &op, const CReg &rop);

    /**
     * Creates a classical "operation" that just returns the value of the given
     * register.
     */
    explicit Operation(const CReg &lop);

    /**
     * Creates a classical "operation" that just returns the given integer
     * value.
     */
    explicit Operation(int val);

};

} // namespace api
} // namespace ql
