#include "utils/strings.h"

#include <algorithm>

namespace ql {
namespace utils {

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

bool string_has(const std::string &str, const std::string &token) {
    return str.find(token) != std::string::npos;
}

} // namespace utils
} // namespace ql
