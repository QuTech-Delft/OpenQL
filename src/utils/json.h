/** \file
 * Provides utilities for handling JSON files, and wraps nlohmann::json in
 * OpenQL's code style.
 */

#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include "utils/logger.h"
#include "utils/exception.h"
#include "utils/str.h"

// check existence of JSON key within node, see PR #194
#define QL_JSON_EXISTS(node, key)  ((node).count(key) > 0)

#define QL_JSON_FATAL(s) QL_FATAL("Error in JSON definition: " << s)   // NB: FATAL prepends "Error : "

#define QL_JSON_ASSERT(node, key, nodePath)     \
    do {                                        \
        if (!QL_JSON_EXISTS(node, key)) {       \
            QL_JSON_FATAL("key '" << key << "' not found on path '" << nodePath << "', actual node contents '" << node << "'"); \
        }                                       \
    } while (false)

namespace ql {
namespace utils {

using Json = nlohmann::json;

Json load_json(const Str &file_name);

// get json value with error notification
// based on: https://github.com/nlohmann/json/issues/932
template<class T>
T json_get(const Json &j, const Str &key, const Str &nodePath = "") {
    // first check existence of key
    auto it = j.find(key);
    if (it == j.end()) {
        QL_JSON_FATAL("Key '" << key
                              << "' not found on path '" << nodePath
                              << "', actual node contents '" << j << "'");
    }

    // then try to get key
    try {
        return it->get<T>();
    } catch (const std::exception &e) {
        QL_JSON_FATAL("Could not get value of key '" << key
                                                     << "' on path '" << nodePath
                                                     << "', exception message '"
                                                     << e.what()
                                                     << "', actual node contents '" << j
                                                     << "'");
    }
}

} // namespace utils
} // namespace ql
