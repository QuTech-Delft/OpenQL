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
class Json;

namespace json_schema {

class Base {
private:

    /**
     * Documentation for this possible value.
     */
    Str doc;

protected:

    /**
     * Returns an identification string for this possible value.
     */
    virtual Str identify() = 0;

public:

    /**
     * Validates the given JSON object against this data type.
     *
     * If the JSON data type matches but its value doesn't validate, a suitable
     * exception is thrown. If the JSON data type does not match, false is
     * returned. Otherwise, true is returned to indicate a match.
     */
    virtual Bool validate(const Json &data) = 0;


};

using Ref = Ptr<Base>;

class Options {
private:

    List<Ref> options;

};

class Array : public Base {
private:

    /**
     * The set of allowable array sizes.
     */
    IntValidator size_validator;

    /**
     * The set of allowable values for the array elements.
     */
    Options element_options;


}

class Object {
public:

    Object &&with(const Str &key, const List<Ref> &options, const Str &doc) &&;

};

} // namespace json_schema

class Json {
private:

    /**
     * Shared pointer to the root node, to prevent it from being deallocated.
     */
    Ptr<nlohmann::json> root;

    /**
     * Reference to the current JSON object. Empty when this is the root,
     * otherwise it consists of a period-terminated list of period-separated
     * elements, with object indices surrounded in "" and array indices
     * surrounded in [].
     */
    nlohmann::json &current;

    /**
     * The path leading up to the current JSON object.
     */
    Str path;

public:

    /**
     * Reads a JSON configuration file.
     */
    static Json from_file(const Str &file_name);

    /**
     * Parses JSON data from a string.
     */
    static Json from_string(const Str &data);

    /**
     * Wraps data from nlohmann_json.
     */
    static Json from_nlohmann(nlohmann::json &&json);

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



    /**
     * Returns true if the
     */
    Bool check_object(const List<Str> &keys);

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
