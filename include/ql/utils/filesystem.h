/** \file
 * Provides platform-agnostic filesystem utilities.
 */

#pragma once

#include <fstream>
#include "ql/utils/str.h"
#include "ql/utils/exception.h"

namespace ql {
namespace utils {

/**
 * Returns whether the given path exists and is a directory.
 */
bool is_dir(const Str &path);

/**
 * Returns whether the given path exists and is a regular file.
 */
bool is_file(const Str &path);

/**
 * Returns whether the given path exists.
 */
bool path_exists(const Str &path);

/**
 * If path looks like it's a relative path, make it relative to base instead.
 * If path looks like it's absolute, return it unchanged.
 */
Str path_relative_to(const Str &base, const Str &path);

/**
 * Returns the directory of the given path. On Linux and MacOS, this just maps
 * to dirname() from libgen.h. On Windows, the string is stripped from the last
 * backslash or slash onward, if any.
 */
Str dir_name(const Str &path);

/**
 * (Recursively) creates a new directory if it does not already exist. Throws
 * an Exception if creation of the directory fails.
 */
void make_dirs(const Str &path);

/**
 * Wrapper for std::ofstream that:
 *  - takes care of the insane error handling magic of C++ streams;
 *  - opens the file upon construction instead of afterwards with open();
 *  - tries to ensure that the directory of the to-be-written file exists before
 *    attempting to open the file.
 * Note that close() does not need to be called; if it isn't, the destructor
 * will do it. But this automatic closing may throw an exception; if this
 * happens while another exception is being handled, abort() will be called.
 */
class OutFile {
private:
    std::ofstream ofs;
    Str path;
public:
    explicit OutFile(const Str &path);
    void write(const Str &content);
    void close();
    void check();
    std::ofstream &unwrap();
    template <typename T>
    OutFile &operator<<(T &&rhs) {
        ofs << std::forward<T>(rhs);
        check();
        return *this;
    }
};

/**
 * Wrapper for std::ifstream that:
 *  - takes care of the insane error handling magic of C++ streams;
 *  - opens the file upon construction instead of afterwards with open().
 * Note that close() does not need to be called; if it isn't, the destructor
 * will do it. But this automatic closing may throw an exception; if this
 * happens while another exception is being handled, abort() will be called.
 */
class InFile {
private:
    std::ifstream ifs;
    Str path;
public:
    InFile(const Str &path);
    Str read();
    void close();
    void check();
    template <typename T>
    InFile &operator>>(T &&rhs) {
        ifs >> std::forward<T>(rhs);
        check();
        return *this;
    }
};

} // namespace utils
} // namespace ql
