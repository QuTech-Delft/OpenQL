/** \file
 * Provides utilities for working with strings that the STL fails to
 * satisfactorily provide.
 */

#include "ql/utils/str.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <regex>
#include "ql/utils/exception.h"
#include "ql/utils/list.h"

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

/**
 * Returns whether str matches the pattern specified by pattern. Pattern syntax
 * is the usual one with * and ?, where * represents zero or more characters,
 * and ? represents exactly one character.
 */
Bool pattern_match(const Str &pattern, const Str &str) {
    StrStrm ss;
    for (char c : pattern) {
        if (c == '*') {
            ss << ".*";
        } else if (c == '?') {
            ss << ".";
        } else if (isalnum(c)) {
            ss << c;
        } else {
            ss << "\\x" << std::setfill('0') << std::setw(2) << std::hex << (int)c;
        }
    }
    return std::regex_match(str, std::regex(ss.str()));
}

/**
 * Takes a raw string and replaces its line prefix accordingly. Any prefixed
 * spacing common to all non-empty lines is removed, as are any empty lines at
 * the start and end. The remaining lines are then prefixed with line_prefix and
 * terminated with a single newline before being written to os.
 */
void dump_str(std::ostream &os, const Str &line_prefix, const Str &raw) {

    // Split into lines.
    List<Str> lines;
    UInt prev = 0;
    while (true) {
        UInt next = raw.find('\n', prev);
        lines.push_back(raw.substr(prev, next - prev));
        if (next == Str::npos) break;
        prev = next + 1;
    }

    // Clear empty lines entirely and find how much common whitespace we have.
    Int common_whitespace = -1;
    for (auto &line : lines) {
        auto whitespace = line.find_first_not_of(' ');
        if (whitespace == Str::npos) {
            line.clear();
        } else if (common_whitespace == -1 || common_whitespace > (Int)whitespace) {
            common_whitespace = whitespace;
        }
    }
    if (common_whitespace < 0) {
        common_whitespace = 0;
    }

    // Remove empty lines from the front and back of the list.
    while (!lines.empty() && lines.front().empty()) lines.pop_front();
    while (!lines.empty() && lines.back().empty()) lines.pop_back();

    // Print to the output stream.
    for (const auto &line : lines) {
        os << line_prefix << (line.empty() ? "" : line.substr(common_whitespace)) << '\n';
    }
    os.flush();

}

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
void wrap_str(std::ostream &os, const Str &line_prefix, const Str &raw) {
    static const UInt NCOLS = 80;

    // Split into lines.
    UInt prev = 0;
    while (true) {
        UInt next = raw.find('\n', prev);
        auto line = raw.substr(prev, next - prev);

        // Wrap each line, using an indent width based on the spacing at the
        // start of the line.
        auto indent_width = line.find_first_not_of(" -");
        if (indent_width == Str::npos) indent_width = 0;
        auto indent = Str(indent_width, ' ');

        // Split the line up into fragments that fit in 100 columns (ignoring
        // indentation and line_prefix size!), and emit them to the output
        // stream.
        Bool first_fragment = true;
        UInt wrap_from = 0;
        while (wrap_from != Str::npos) {
            UInt wrap_to, wrap_next;
            if (wrap_from + NCOLS < line.size()) {
                wrap_to = line.rfind(' ', wrap_from + NCOLS + (first_fragment ? indent_width : 0));
                if (wrap_to == Str::npos) {
                    wrap_to = wrap_from + NCOLS;
                    wrap_next = wrap_to;
                } else {
                    wrap_next = wrap_to + 1;
                }
            } else {
                wrap_to = line.size();
                wrap_next = Str::npos;
            }
            auto fragment = line.substr(wrap_from, wrap_to - wrap_from);
            if (first_fragment) {
                os << line_prefix << fragment << '\n';
                first_fragment = false;
            } else {
                os << line_prefix << indent << fragment << '\n';
            }
            wrap_from = wrap_next;
        }

        // Advance to the next line.
        if (next == Str::npos) break;
        prev = next + 1;

    }

    os.flush();
}

/**
 * Returns whether str starts with front.
 */
Bool starts_with(const Str &str, const Str &front) {
    return str.rfind(front, 0) == 0;
}

/**
 * Returns whether str ends with end.
 */
Bool ends_with(const Str &str, const Str &end) {
    if (str.size() >= end.size()) {
        return (str.compare(str.size() - end.size(), end.size(), end) == 0);
    } else {
        return false;
    }
}

} // namespace utils
} // namespace ql
