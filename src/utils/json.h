#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include "utils/logger.h"
#include "utils/exception.h"

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
namespace utils {

using json = nlohmann::json;

json load_json(const std::string &file_name);

// get json value with error notification
// based on: https://github.com/nlohmann/json/issues/932
template<class T>
T json_get(const json &j, const std::string &key, const std::string &nodePath = "") {
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

} // namespace utils
} // namespace ql
