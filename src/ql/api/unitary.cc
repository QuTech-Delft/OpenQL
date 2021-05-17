/** \file
 * API header for defining unitary gates for the unitary decomposition logic.
 */

#include "ql/api/unitary.h"

//============================================================================//
//                               W A R N I N G                                //
//----------------------------------------------------------------------------//
//         Docstrings in this file must manually be kept in sync with         //
//      unitary.i! This should be automated at some point, but isn't yet.     //
//============================================================================//

namespace ql {
namespace api {

/**
 * Creates a unitary gate from the given row-major, square, unitary
 * matrix.
 */
Unitary::Unitary(
    const std::string &name,
    const std::vector<std::complex<double>> &matrix
) :
    name(name)
{
    unitary.emplace(name, ql::utils::Vec<ql::utils::Complex>(matrix.begin(), matrix.end()));
}

/**
 * Explicitly decomposes the gate. Does not need to be called; it will be
 * called automatically when the gate is added to the kernel.
 */
void Unitary::decompose() {
    unitary->decompose();
}

/**
 * Returns whether OpenQL was built with unitary decomposition support
 * enabled.
 */
bool Unitary::is_decompose_support_enabled() {
    return ql::com::Unitary::is_decompose_support_enabled();
}

} // namespace api
} // namespace ql
