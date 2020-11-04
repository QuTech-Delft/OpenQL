#pragma once

#include <string>
#include <sstream>
#include <typeinfo>

// TODO: remove
#include <vector>

namespace ql {
namespace utils {

// Anonymous namespace to make the SFINAE and overload resolution of the
// debug_str_internal() function "private". Use debug_str().
namespace {

template <typename T, typename = decltype(std::declval<std::ostream&>() << std::declval<T>())>
std::string try_to_string_internal(T val) {
    std::ostringstream ss{};
    ss << val;
    return ss.str();
}

template <typename T, typename... X>
std::string try_to_string_internal(T val, X... vals) {
    std::ostringstream ss{};
    ss << "unknown value of type " << typeid(T).name() << ">";
    return ss.str();
}

} // anonymous namespace

/**
 * If a << overload for streams exists for type T, return the result of said
 * overload as a string. If no overload exists, "<unknown value of type ...>"
 * is returned, where ... is derived using typeid::name().
 */
template <typename T>
std::string try_to_string(T val) {
    return try_to_string_internal(val);
}

/**
 * Convert the given value to a string using its stream operator<< overload.
 */
template <typename T>
std::string to_string(T arg) {
    std::stringstream ss;
    ss << arg;
    return ss.str ();
}

std::string to_lower(std::string str);
std::string replace_all(std::string str, const std::string &from, const std::string &to);

// TODO: remove (should be method on Vec)
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

} // namespace utils
} // namespace ql
