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

#include "json.h"
#include "exception.h"
#include "opt.h"

#define println(x) std::cout << "[OPENQL] "<< x << std::endl

#define EOUT(content) \
    do {                                                                                                    \
        if (::ql::utils::logger::LOG_LEVEL >= ::ql::utils::logger::log_level_t::LOG_ERROR) {                \
            ::std::cerr << "[OPENQL] " __FILE__ ":" << __LINE__ << " Error: " << content << ::std::endl;    \
        }                                                                                                   \
    } while (false)

#define WOUT(content) \
    do {                                                                                                    \
        if (::ql::utils::logger::LOG_LEVEL >= ::ql::utils::logger::log_level_t::LOG_WARNING) {              \
            ::std::cerr << "[OPENQL] " __FILE__ ":" << __LINE__ << " Warning: " << content << ::std::endl;  \
        }                                                                                                   \
    } while (false)

#define IOUT(content) \
    do {                                                                                                    \
        if (::ql::utils::logger::LOG_LEVEL >= ::ql::utils::logger::log_level_t::LOG_INFO) {                 \
            ::std::cout << "[OPENQL] " __FILE__ ":" << __LINE__ << " Info: "<< content << ::std::endl;      \
        }                                                                                                   \
    } while (false)

#define DOUT(content) \
    do {                                                                                                    \
        if (::ql::utils::logger::LOG_LEVEL >= ::ql::utils::logger::log_level_t::LOG_DEBUG) {                \
            ::std::cout << "[OPENQL] " __FILE__ ":" << __LINE__ << " " << content << ::std::endl;           \
        }                                                                                                   \
    } while (false)

#define COUT(content) \
    do {                                                                                                    \
        ::std::cout << "[OPENQL] " __FILE__ ":" << __LINE__ << " " << content << ::std::endl;               \
    } while (false)

// helper macro: stringstream to string
// based on https://stackoverflow.com/questions/21924156/how-to-initialize-a-stdstringstream
#define SS2S(values) std::string(static_cast<std::ostringstream&&>(std::ostringstream() << values).str())

#define FATAL(content) \
    do {                                                                                                    \
        ::std::ostringstream fatal_ss{};                                                                    \
        fatal_ss << content;                                                                                \
        ::std::string fatal_s = fatal_ss.str();                                                             \
        EOUT(fatal_s);                                                                                      \
        throw ql::exception("Error : " + fatal_s, false);                                                   \
    } while (false)

#define ASSERT(condition)                                                                                   \
    do {                                                                                                    \
        if (!(condition)) {                                                                                 \
            FATAL("assert " #condition " failed in file " __FILE__ " at line " << __LINE__);                \
        }                                                                                                   \
    } while (false)

// get the number of elements in an array
#define ELEM_CNT(x) (sizeof(x)/sizeof(x[0]))

// check existence of JSON key within node, see PR #194
#define JSON_EXISTS(node, key)  ((node).count(key) > 0)

#define JSON_ASSERT(node, key, nodePath)    \
    do {                                    \
        if (!JSON_EXISTS(node, key)) {      \
            FATAL("key '" << key << "' not found on path '" << nodePath << "', actual node contents '" << node << "'"); \
        }                                   \
    } while (false)


#ifdef _MSC_VER
#ifdef BUILDING_OPENQL
#define OPENQL_DECLSPEC __declspec(dllexport)
#else
#define OPENQL_DECLSPEC __declspec(dllimport)
#endif
#else
#define OPENQL_DECLSPEC
#endif

namespace ql {

using json = nlohmann::json;

const size_t MAX_CYCLE = std::numeric_limits<int>::max();

namespace utils {

namespace logger {

enum log_level_t {
    LOG_NOTHING,
    LOG_CRITICAL,
    LOG_ERROR,
    LOG_WARNING,
    LOG_INFO,
    LOG_DEBUG
};

OPENQL_DECLSPEC extern log_level_t LOG_LEVEL;

void set_log_level(const std::string &level);

} // namespace logger

void make_output_dir(const std::string &dir);
std::string to_lower(std::string str);
std::string replace_all(std::string str, const std::string &from, const std::string &to);

// from: https://stackoverflow.com/questions/5878775/how-to-find-and-replace-string
// NB: also see replace_all
template <typename T, typename U>
T &replace(T &str, const U &from, const U &to) {
    size_t pos;
    size_t offset = 0;
    const size_t increment = to.size();

    while ((pos = str.find(from, offset)) != T::npos)
    {
        str.replace(pos, from.size(), to);
        offset = pos + increment;
    }

    return str;
}

// from https://stackoverflow.com/questions/9146395/reset-c-int-array-to-zero-the-fastest-way
template<typename T, size_t SIZE>
inline void zero(T(&arr)[SIZE]){
    memset(arr, 0, SIZE*sizeof(T));
}

/**
 * write content to the file <file_name>
 */
void write_file(const std::string &file_name, const std::string&content);

template <typename T>
std::string to_string(T arg) {
    std::stringstream ss;
    ss << arg;
    return ss.str ();
}

template<class T>
std::string to_string(
    const std::vector<T> &v,
    const std::string &vector_prefix = "",
    const std::string &elem_sep = ", "
) {
    std::ostringstream ss;
    ss << vector_prefix << " [";
    size_t sz = v.size();
    if (sz > 0) {
        size_t i;
        for (i = 0; i < sz - 1; ++i) {
            ss << v[i] << elem_sep;
        }
        ss << v[i];
    }

    ss << "]";
    return ss.str();
}

template<typename T>
void print_vector(
    const std::vector<T> &v,
    const std::string &prefix="",
    const std::string &separator=" | "
) {
    std::cout << to_string(v, prefix, separator) << std::endl;
}

template <typename T>
int sign_of(T val) {
    return (T(0) < val) - (val < T(0));
}

bool string_has(const std::string &str, const std::string &token);

// Helper function to sort the vector of pairs.
// Pairs are sorted by first element of pairs and then by second element
bool sort_pair_helper(
    const std::pair<size_t,size_t> &a,
    const std::pair<size_t,size_t> &b
);

} // namespace utils

json load_json(const std::string &file_name);

// get json value with error notification
// based on: https://github.com/nlohmann/json/issues/932
template<class T>
T json_get(const json &j, std::string key, std::string nodePath = "") {
    // first check existence of key
    auto it = j.find(key);
    if (it == j.end()) {
        FATAL("Key '" << key
                      << "' not found on path '" << nodePath
                      << "', actual node contents '" << j << "'");
    }

    // then try to get key
    try {
        return it->get<T>();
    } catch (const std::exception &e) {
        FATAL("Could not get value of key '" << key
                                             << "' on path '" << nodePath
                                             << "', exception message '"
                                             << e.what()
                                             << "', actual node contents '" << j
                                             << "'");
    }
}

} // namespace ql
