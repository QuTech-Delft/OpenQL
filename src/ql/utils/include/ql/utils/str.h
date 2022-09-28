/** \file
 * Provides utilities for working with strings that the STL fails to
 * satisfactorily provide.
 */

#pragma once

#include <string>
#include <sstream>
#include <typeinfo>
#include "ql/utils/num.h"

namespace ql {
namespace utils {

/**
 * Shorthand for std::string.
 */
using Str = std::string;

/**
 * Shorthand for std::ostringstream.
 */
using StrStrm = std::ostringstream;

// Anonymous namespace to make the SFINAE and overload resolution of the
// debug_str_internal() function "private". Use debug_str().
namespace {

template <typename T, typename = decltype(std::declval<std::ostream&>() << std::declval<T>())>
Str try_to_string_internal(T val) {
    StrStrm ss{};
    ss << val;
    return ss.str();
}

template <typename T, typename... X>
Str try_to_string_internal(T val, X... vals) {
    std::ostringstream ss{};
    ss << "<unknown value of type " << typeid(T).name() << ">";
    return ss.str();
}

} // anonymous namespace

/**
 * If a << overload for streams exists for type T, return the result of said
 * overload as a string. If no overload exists, "<unknown value of type ...>"
 * is returned, where ... is derived using typeid::name().
 */
template <typename T>
Str try_to_string(T val) {
    return try_to_string_internal(val);
}

/**
 * Convert the given value to a string using its stream operator<< overload.
 */
template <typename T>
Str to_string(T arg) {
    StrStrm ss;
    ss << arg;
    return ss.str();
}

/**
 * Parses the given string as an unsigned integer. Throws an exception if the
 * conversion fails.
 */
UInt parse_uint(const Str &str);

/**
 * Parses the given string as a signed integer. Throws an exception if the
 * conversion fails.
 */
Int parse_int(const Str &str);

/**
 * Parses the given string as a real number. Throws an exception if the
 * conversion fails.
 */
Real parse_real(const Str &str);

/**
 * Parses the given string as an unsigned integer. Returns the given default
 * value if the conversion fails.
 */
UInt parse_uint(const Str &str, UInt dflt, bool *success=nullptr);

/**
 * Parses the given string as a signed integer. Returns the given default value
 * if the conversion fails.
 */
Int parse_int(const Str &str, Int dflt, bool *success=nullptr);

/**
 * Parses the given string as a real number. Returns the given default value if
 * the conversion fails.
 */
Real parse_real(const Str &str, Real dflt, bool *success=nullptr);

/**
 * Converts the given string to lowercase.
 */
Str to_lower(Str str);

/**
 * Converts the given string to uppercase.
 */
Str to_upper(Str str);

/**
 * Replaces all occurrences of from in str with to.
 */
Str replace_all(Str str, const Str &from, const Str &to);

/**
 * Returns whether str matches the pattern specified by pattern. Pattern syntax
 * is the usual one with * and ?, where * represents zero or more characters,
 * and ? represents exactly one character.
 */
Bool pattern_match(const Str &pattern, const Str &str);

/**
 * Takes a raw string and replaces its line prefix accordingly. Any prefixed
 * spacing common to all non-empty lines is removed, as are any empty lines at
 * the start and end. The remaining lines are then prefixed with line_prefix and
 * terminated with a single newline before being written to os.
 */
void dump_str(std::ostream &os, const Str &line_prefix, const Str &raw);

/**
 * Takes a (documentation) string, and:
 *  - wraps long lines at column 80;
 *  - prefixes all resulting lines with line_prefix; and
 *  - dumps the resulting lines to the given stream.
 *
 * Indentation tries to be smart about lists with - bullets, but other than that
 * it's pretty stupid. Newlines are not converted to spaces prior to wrapping,
 * so the incoming documentation should not be pre-wrapped.
 */
void wrap_str(std::ostream &os, const Str &line_prefix, const Str &raw);

/**
 * Returns whether str starts with front.
 */
Bool starts_with(const Str &str, const Str &front);

/**
 * Returns whether str ends with end.
 */
Bool ends_with(const Str &str, const Str &end);

} // namespace utils
} // namespace ql
