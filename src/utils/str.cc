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

Int parse_int(const Str &str) {
    try {
        return std::stoll(str);
    } catch (std::invalid_argument &e) {
        (void)e;
        throw Exception("failed to parse \"" + str + "\" as an unsigned integer");
    } catch (std::range_error &e) {
        (void)e;
        throw Exception("failed to parse \"" + str + "\" as an unsigned integer (out of range)");
    }
}

Real parse_double(const Str &str) {
    try {
        return std::stod(str);
    } catch (std::invalid_argument &e) {
        (void)e;
        throw Exception("failed to parse \"" + str + "\" as an unsigned integer");
    } catch (std::range_error &e) {
        (void)e;
        throw Exception("failed to parse \"" + str + "\" as an unsigned integer (out of range)");
    }
}

std::string to_lower(std::string str) {
    std::transform(
        str.begin(), str.end(), str.begin(),
        [](unsigned char c){ return std::tolower(c); }
    );
    return str;
}

/**
 * @param str
 *    string to be processed
 * @param from
 *    string to be replaced
 * @param to
 *    string used to replace from
 * @brief
 *    replace recursively from by to in str
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
