/** \file
 * Option-parsing and storage implementation.
 */

#pragma once

#include "utils/str.h"

namespace ql {
namespace options {

void print();
void print_current_values();
void set(const utils::Str &opt_name, const utils::Str &opt_value);
utils::Str get(const utils::Str &opt_name);
void reset_options();

} // namespace options
} // namespace ql
