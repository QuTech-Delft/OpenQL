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

// check existence of JSON key within node, see PR #194
#define JSON_EXISTS(node, key)  ((node).count(key) > 0)

#define JSON_FATAL(s) FATAL("Error in JSON definition: " << s)   // NB: FATAL prepends "Error : "

#define JSON_ASSERT(node, key, nodePath)    \
    do {                                    \
        if (!JSON_EXISTS(node, key)) {      \
            JSON_FATAL("key '" << key << "' not found on path '" << nodePath << "', actual node contents '" << node << "'"); \
        }                                   \
    } while (false)

namespace ql {

using json = nlohmann::json;

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

json load_json(const std::string &file_name);

// get json value with error notification
// based on: https://github.com/nlohmann/json/issues/932
template<class T>
T json_get(const json &j, std::string key, std::string nodePath = "") {
    // first check existence of key
    auto it = j.find(key);
    if (it == j.end()) {
        JSON_FATAL("Key '" << key
                      << "' not found on path '" << nodePath
                      << "', actual node contents '" << j << "'");
    }

    // then try to get key
    try {
        return it->get<T>();
    } catch (const std::exception &e) {
        JSON_FATAL("Could not get value of key '" << key
                                                  << "' on path '" << nodePath
                                                  << "', exception message '"
                                                  << e.what()
                                                  << "', actual node contents '" << j
                                                  << "'");
    }
}

} // namespace ql
