/** \file
 * Provides platform-agnostic filesystem utilities.
 */

#pragma once

#include <fstream>
#include "ql/utils/str.h"
#include "ql/utils/exception.h"
#include "ql/utils/compat.h"
#include "ql/utils/list.h"

namespace ql {
namespace utils {

namespace {

/**
 * Stack of working directories. Private; use push_working_directory(),
 * pop_working_directory(), and get_working_directory() to access.
 */
QL_GLOBAL extern List<Str> working_directory_stack;

} // anonymous namespace

/**
 * Sets OpenQL's working directory to the given directory. If the directory
 * looks like a relative path, it is appended to the previous working directory.
 * Otherwise it overrides the working directory.
 */
void push_working_directory(const Str &dir);

/**
 * Reverts the change made by the previous push_working_directory() call.
 */
void pop_working_directory();

/**
 * Context management class that pushes the given working directory on
 * construction and pops it on destruction.
 */
struct WithWorkingDirectory {
    WithWorkingDirectory(const Str &dir) { push_working_directory(dir); }
    ~WithWorkingDirectory() { pop_working_directory(); }
};

/**
 * Returns OpenQL's current working directory. If no working directory has been
 * set yet, `.` is returned, so the OS working directory is effectively used
 * instead.
 */
Str get_working_directory();

/**
 * Returns whether the given path exists and is a directory. If path looks like
 * a relative path, it is interpreted as relative to the current OpenQL working
 * directory.
 */
Bool is_dir(const Str &path);

/**
 * Returns whether the given path exists and is a regular file. If path looks
 * like a relative path, it is interpreted as relative to the current OpenQL
 * working directory.
 */
Bool is_file(const Str &path);

/**
 * Returns whether the given path exists. If path looks like a relative path, it
 * is interpreted as relative to the current OpenQL working directory.
 */
Bool path_exists(const Str &path);

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
 * an Exception if creation of the directory fails. If path looks like a
 * relative path, it is interpreted as relative to the current OpenQL working
 * directory.
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
 * Relative paths are treated as relative to the current OpenQL working
 * directory.
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
 * Relative paths are treated as relative to the current OpenQL working
 * directory.
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
