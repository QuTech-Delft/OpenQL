#pragma once

#include <string>

namespace ql {
namespace utils {

void make_output_dir(const std::string &dir);

/**
 * write content to the file <file_name>
 */
void write_file(const std::string &file_name, const std::string &content);

} // namespace utils
} // namespace ql
