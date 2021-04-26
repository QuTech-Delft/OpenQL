/** \file
 * Main OpenQL header.
 */

#pragma once

#include "ql/api/api.h"

namespace ql {

// The API is defined in its own folder in the source tree. The namespaces mimic
// that for consistency. However, making users use ql::api everywhere in user
// code would be a bit weird, so we pull the api namespace into the root ql
// namespace. The rest of OpenQL doesn't define anything in there directly, so
// this should be safe to do.
using namespace api;

} // namespace ql
