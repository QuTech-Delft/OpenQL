/** \file
 * Provides access to OpenQL's global options.
 */

#pragma once

#include "ql/utils/options.h"
#include "ql/utils/compat.h"

namespace ql {
namespace com {
namespace options {

/**
 * Makes a new options record for OpenQL.
 */
utils::Options make_ql_options();

/**
 * Global options object for all of OpenQL.
 */
QL_GLOBAL extern utils::Options global;

/**
 * Convenience function for getting an option value as a string from the global
 * options record.
 */
const utils::Str &get(const utils::Str &key);

/**
 * Convenience function for setting an option value for the global options
 * record.
 */
void set(const utils::Str &key, const utils::Str &value);

} // namespace options
} // namespace com
} // namespace ql
