/**
 * @file   utils.h
 * @date   04/2017
 * @author Nader Khammassi
 *         Imran Ashraf
 * @brief  string utils (from qx)
 */

#pragma once

#include <limits>
#include <algorithm>
#include <iterator>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <utility>
#include <vector>

#include "utils/json.h"
#include "utils/exception.h"
#include "utils/opt.h"
#include "utils/strings.h"
#include "utils/logger.h"
#include "utils/filesystem.h"
#include "utils/misc.h"

namespace ql {
namespace utils {

template<typename T>
void print_vector(
    const std::vector<T> &v,
    const std::string &prefix="",
    const std::string &separator=" | "
) {
    std::cout << to_string(v, prefix, separator) << std::endl;
}

} // namespace utils
} // namespace ql
