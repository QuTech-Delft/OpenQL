/** \file
 * Provides platform-agnostic filesystem utilities.
 */

#pragma once

#include "utils/str.h"

namespace ql {
namespace utils {

bool is_dir(const Str &path);
bool is_file(const Str &path);
bool path_exists(const Str &path);
Str dir_name(const Str &path);
void make_dirs(const Str &path);
void write_file(const Str &path, const Str &content);

} // namespace utils
} // namespace ql
