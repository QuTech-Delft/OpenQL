/** \file
 * Provides utilities for working with strings that the STL fails to
 * satisfactorily provide.
 */

#include "utils/str.h"

#include <algorithm>
#include <cctype>
#include "utils/exception.h"

namespace ql {
namespace utils {

/**
 * Parses the given string as an unsigned integer. Throws an exception if the
 * conversion fails.
 */
UInt parse_uint(const Str &str) {
    try {
        return std::stoull(str);
    } catch (std::invalid_argument &e) {
        (void)e;
        throw Exception("failed to parse \"" + str + "\" as an unsigned integer");
    } catch (std::range_error &e) {
        (void)e;
        throw Exception("failed to parse \"" + str + "\" as an unsigned integer (out of range)");
    }
}

/**
 * Parses the given string as a signed integer. Throws an exception if the
 * conversion fails.
 */
Int parse_int(const Str &str) {
    try {
        return std::stoll(str);
    } catch (std::invalid_argument &e) {
        (void)e;
        throw Exception("failed to parse \"" + str + "\" as a signed integer");
    } catch (std::range_error &e) {
        (void)e;
        throw Exception("failed to parse \"" + str + "\" as a signed integer (out of range)");
    }
}

/**
 * Parses the given string as a real number. Throws an exception if the
 * conversion fails.
 */
Real parse_real(const Str &str) {
    try {
        return std::stod(str);
    } catch (std::invalid_argument &e) {
        (void)e;
        throw Exception("failed to parse \"" + str + "\" as a real number");
    } catch (std::range_error &e) {
        (void)e;
        throw Exception("failed to parse \"" + str + "\" as a real number (out of range)");
    }
}

/**
 * Parses the given string as an unsigned integer. Returns the given default
 * value if the conversion fails.
 */
UInt parse_uint(const Str &str, UInt dflt, bool *success) {
    try {
        auto x = std::stoull(str);
        if (success) *success = true;
        return x;
    } catch (std::invalid_argument &e) {
        (void)e;
        if (success) *success = false;
        return dflt;
    } catch (std::range_error &e) {
        (void)e;
        if (success) *success = false;
        return dflt;
    }
}

/**
 * Parses the given string as a signed integer. Returns the given default value
 * if the conversion fails.
 */
Int parse_int(const Str &str, Int dflt, bool *success) {
    try {
        auto x = std::stoll(str);
        if (success) *success = true;
        return x;
    } catch (std::invalid_argument &e) {
        (void)e;
        if (success) *success = false;
        return dflt;
    } catch (std::range_error &e) {
        (void)e;
        if (success) *success = false;
        return dflt;
    }
}

/**
 * Parses the given string as a real number. Returns the given default value if
 * the conversion fails.
 */
Real parse_real(const Str &str, Real dflt, bool *success) {
    try {
        auto x = std::stod(str);
        if (success) *success = true;
        return x;
    } catch (std::invalid_argument &e) {
        (void)e;
        if (success) *success = false;
        return dflt;
    } catch (std::range_error &e) {
        (void)e;
        if (success) *success = false;
        return dflt;
    }
}

/**
 * Converts the given string to lowercase.
 */
std::string to_lower(std::string str) {
    std::transform(
        str.begin(), str.end(), str.begin(),
        [](unsigned char c){ return std::tolower(c); }
    );
    return str;
}

/**
 * Replaces all occurrences of from in str with to.
 */
std::string replace_all(std::string str, const std::string &from, const std::string &to) {
    // https://stackoverflow.com/questions/2896600/how-to-replace-all-occurrences-of-a-character-in-string
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

} // namespace utils
} // namespace ql
