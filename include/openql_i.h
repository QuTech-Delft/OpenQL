/** \file
 * Compatibility header for C++ programs which used to #include "openql_i.h"
 */

#pragma once

#warning "Use of the openql_i.h header is deprecated. Please use #include <openql>!"

#include "openql.h"
#include "ql/api/api.h"

// openql_i.h used to declare everything in the root namespace, so we'll have to
// mimic that with an ugly using namespace call.
using namespace ql::api;
