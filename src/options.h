/**
 * @file   options.h
 * @date   05/2018
 * @author Imran Ashraf
 * @brief  Options class implementing parsing of supported options
 */

#pragma once

#include <string>

namespace ql {
namespace options {

void print();
void print_current_values();
void set(const std::string &opt_name, const std::string &opt_value);
std::string get(const std::string &opt_name);
void reset_options();

} // namespace options
} // namespace ql
