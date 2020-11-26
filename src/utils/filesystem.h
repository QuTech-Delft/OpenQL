/** \file
 * Provides platform-agnostic filesystem utilities.
 */

#pragma once

#include <fstream>
#include "utils/str.h"
#include "utils/exception.h"

namespace ql {
namespace utils {

bool is_dir(const Str &path);
bool is_file(const Str &path);
bool path_exists(const Str &path);
Str dir_name(const Str &path);
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
    OutFile(const Str &path);
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
