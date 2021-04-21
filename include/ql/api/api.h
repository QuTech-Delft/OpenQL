/** \file
 * Main header for the external API to OpenQL.
 */

#pragma once

//============================================================================//
//                               W A R N I N G                                //
//----------------------------------------------------------------------------//
// Additions to/removals from the API (classes & global functions) must be //
// manually kept in sync with:
//  - the Sphinx documentation .rst files in docs/api;
//  - the __all__ declaration in python/openql/__init__.py;
//  -
//============================================================================//

#include "ql/api/misc.h"
#include "ql/api/declarations.h"
#include "ql/api/pass.h"
#include "ql/api/compiler.h"
#include "ql/api/platform.h"
#include "ql/api/creg.h"
#include "ql/api/operation.h"
#include "ql/api/unitary.h"
#include "ql/api/kernel.h"
#include "ql/api/program.h"
#include "ql/api/cqasm_reader.h"

