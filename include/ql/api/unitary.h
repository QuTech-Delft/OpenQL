/** \file
 * API header for defining unitary gates for the unitary decomposition logic.
 */

#pragma once

#include "ql/com/unitary.h"
#include "ql/api/declarations.h"

//============================================================================//
//                               W A R N I N G                                //
//----------------------------------------------------------------------------//
//         Docstrings in this file must manually be kept in sync with         //
//      unitary.i! This should be automated at some point, but isn't yet.     //
//============================================================================//

namespace ql {
namespace api {

/**
 * Unitary matrix interface.
 */
class Unitary {
private:
    friend class Kernel;

    /**
     * The wrapped unitary gate.
     */
    ql::utils::Ptr<ql::com::Unitary> unitary;

public:

    /**
     * The name given to the unitary gate.
     */
    const std::string name;

    /**
     * Creates a unitary gate from the given row-major, square, unitary
     * matrix.
     */
    Unitary(const std::string &name, const std::vector<std::complex<double>> &matrix);

    /**
     * Explicitly decomposes the gate. Does not need to be called; it will be
     * called automatically when the gate is added to the kernel.
     */
    void decompose();

    /**
     * Returns whether OpenQL was built with unitary decomposition support
     * enabled.
     */
    static bool is_decompose_support_enabled();

};

} // namespace api
} // namespace ql
