#pragma once

#include <sstream>

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

} // namespace utils
} // namespace ql
