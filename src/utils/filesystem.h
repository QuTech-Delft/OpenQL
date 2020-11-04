/** \file
 * Provides platform-agnostic filesystem utilities.
 */

#pragma once

#include <string>

namespace ql {
namespace utils {

bool is_dir(const std::string &path);
bool is_file(const std::string &path);
bool path_exists(const std::string &path);
std::string dir_name(const std::string &path);
void make_dirs(const std::string &path);
void write_file(const std::string &path, const std::string &content);

} // namespace utils
} // namespace ql
