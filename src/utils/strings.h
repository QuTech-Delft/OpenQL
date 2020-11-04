#pragma once

#include <string>
#include <sstream>

// TODO: remove
#include <vector>

namespace ql {
namespace utils {

// Anonymous namespace to make the SFINAE and overload resolution of the
// debug_str_internal() function "private". Use debug_str().
namespace {

template <typename T, typename = decltype(std::declval<std::ostream&>() << std::declval<T>())>
std::string debug_str_internal(T val) {
    std::ostringstream ss{};
    ss << val;
    return ss.str();
}

template <typename T, typename... X>
std::string debug_str_internal(T val, X... vals) {
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
std::string debug_str(T val) {
    return debug_str_internal(val);
}

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

template <typename T>
std::string to_string(T arg) {
    std::stringstream ss;
    ss << arg;
    return ss.str ();
}

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

bool string_has(const std::string &str, const std::string &token);

} // namespace utils
} // namespace ql
