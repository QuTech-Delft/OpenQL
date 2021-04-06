/** \file
 * Provides utilities for handling JSON files, and wraps nlohmann::json in
 * OpenQL's code style.
 */

#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include "ql/utils/logger.h"
#include "ql/utils/exception.h"
#include "ql/utils/str.h"
#include "ql/utils/list.h"
#include "ql/utils/set.h"
#include "ql/utils/ptr.h"

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

#if 0

using RawJson = nlohmann::json;

class JsonConfigurable {
protected:




};


class JsonObjectReader {

    /**
     * Registers an action for the given key, treating they key as required. The
     * action function will be called by run() exactly once, receiving a value
     * reader for the value.
     */
    JsonObjectReader &&require(
        const Str &key,
        const Str &doc,
        std::function<void(const JsonValueReader &reader)> action
    ) &&;

    /**
     * Registers an action for the given key, treating they key as required. The
     * action function will be called by run() exactly once, receiving a value
     * reader for the value.
     */
    JsonObjectReader &&optional(
        const Str &key,
        const Str &doc,
        std::function<void(const JsonValueReader *reader)> action
    ) &&;

    JsonObjectReader &&otherwise(std::function<void(const Str &key, const JsonValueReader &reader)> action) &&;

};

class JsonObj {
private:

    /**
     * Shared pointer to the root node, to prevent it from being deallocated.
     */
    Ptr<RawJson> root;

    /**
     * Reference to the current JSON object. Empty when this is the root,
     * otherwise it consists of a period-terminated list of period-separated
     * elements, with object indices surrounded in "" and array indices
     * surrounded in [].
     */
    RawJson &current;

    /**
     * The path leading up to the current JSON object.
     */
    Str path;

    /**
     * The set of keys that have been referenced thus far.
     */
    Set<Str> used_keys;

public:

    /**
     * Reads a JSON configuration file.
     */
    static JsonObj from_file(const Str &file_name);

    /**
     * Parses JSON data from a string.
     */
    static JsonObj from_string(const Str &data);

    /**
     * Wraps data from nlohmann_json.
     */
    static JsonObj from_nlohmann(nlohmann::json &&json);

    /**
     * Returns the path leading up to this object from the root. Empty when this
     * is the root, otherwise it consists of a period-terminated list of
     * period-separated elements, with object indices surrounded in "" and array
     * indices surrounded in [].
     */
    const Str &get_path() const;

    /**
     * Returns the raw nlohmann::json object that we're wrapping.
     */
    const RawJson &unwrap() const;

    /**
     * Returns the raw nlohmann::json object that we're wrapping.
     */
    RawJson &unwrap();

    bool is_unspecified

    /**
     * Returns true if the
     */
    void check_object(const List<Str> &keys);

};

#endif

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

template<>
const Json &json_get(const Json &j, const Str &key, const Str &nodePath);

} // namespace utils
} // namespace ql
