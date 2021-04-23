/** \file
 * Main header for the external API to OpenQL.
 */

#pragma once

//============================================================================//
//                               W A R N I N G                                //
//----------------------------------------------------------------------------//
//  Additions to/removals from the API (classes & global functions) must be   //
//           manually kept in sync with the __all__ declaration in            //
//                         python/openql/__init__.py                          //
//----------------------------------------------------------------------------//
//  Additions to/removals from the API fileset must manually be kept in sync  //
// with the python/openql.i (don't forget to add a .i subfile in src as well) //
//----------------------------------------------------------------------------//
//    Additions to/removals from the set of automatically-wrapped or SWIG     //
//  STL template expansion types that the API uses must be kept in sync with  //
//    the C++ to Python typemap in the docstring monkey-patching logic of     //
//                         python/openql/__init__.py                          //
//----------------------------------------------------------------------------//
//  After changing anything, make sure to check if documentation generation   //
//                     still works ("make html" in docs)                      //
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

