/** \file
 * API header for using classical integer registers.
 */

#pragma once

#include "ql/ir/compat/compat.h"
#include "ql/api/declarations.h"

//============================================================================//
//                               W A R N I N G                                //
//----------------------------------------------------------------------------//
//         Docstrings in this file must manually be kept in sync with         //
//       creg.i! This should be automated at some point, but isn't yet.       //
//============================================================================//

namespace ql {
namespace api {

/**
 * Represents a classical 32-bit integer register.
 */
class CReg {
private:
    friend class Operation;
    friend class Kernel;

    /**
     * The wrapped control register object.
     */
    ql::utils::Ptr<ql::ir::compat::ClassicalRegister> creg;

public:

    /**
     * Creates a register with the given index.
     */
    explicit CReg(size_t id);

};

} // namespace api
} // namespace ql
