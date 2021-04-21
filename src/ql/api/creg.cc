/** \file
 * API header for using classical integer registers.
 */

#include "ql/api/creg.h"

//============================================================================//
//                               W A R N I N G                                //
//----------------------------------------------------------------------------//
//         Docstrings in this file must manually be kept in sync with         //
//   python/openql.i! This should be automated at some point, but isn't yet.  //
//============================================================================//

namespace ql {
namespace api {

/**
 * Creates a register with the given index.
 */
CReg::CReg(size_t id) {
    creg.emplace(id);
}

} // namespace api
} // namespace ql
