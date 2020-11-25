/** \file
 * Provides platform-agnostic filesystem utilities.
 */

#include "utils/filesystem.h"

#include <iostream>
#include <fstream>
#include <cerrno>
#include <algorithm>

#ifdef _WIN32
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#define stat _stat
#define S_IFDIR _S_IFDIR
#define S_IFREG _S_IFREG
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>
#endif

namespace ql {
namespace utils {

/**
 * Returns whether the given path exists and is a directory.
 */
bool is_dir(const Str &path) {
    struct stat info{};
    if (stat(path.c_str(), &info) != 0) {
        return false;
    }
    return (info.st_mode & S_IFDIR) != 0;
}

/**
 * Returns whether the given path exists and is a regular file.
 */
bool is_file(const Str &path) {
    struct stat info{};
    if (stat(path.c_str(), &info) != 0) {
        return false;
    }
    return (info.st_mode & S_IFREG) != 0;
}

/**
 * Returns whether the given path exists.
 */
bool path_exists(const Str &path) {
    struct stat info{};
    return !stat(path.c_str(), &info);
}

/**
 * Returns the directory of the given path. On Linux and MacOS, this just maps
 * to dirname() from libgen.h. On Windows, the string is stripped from the last
 * backslash or slash onward, if any.
 */
Str dir_name(const Str &path) {
#ifdef _WIN32
    auto last_slash = path.find_last_of('/');
    if (last_slash == Str::npos) {
        last_slash = 0;
    }
    auto last_backslash = path.find_last_of('\\');
    if (last_backslash == Str::npos) {
        last_backslash = 0;
    }
    auto new_length = std::max(last_slash, last_backslash);
    if (!new_length) {
        return path;
    }
    return path.substr(0, new_length);
#else
    Str path_copy{path};
    return Str(dirname(&path_copy[0]));
#endif
}

/**
 * (Recursively) creates a new directory if it does not already exist. Throws
 * an Exception if creation of the directory fails.
 */
void make_dirs(const Str &path) {

    // If the given path is already an existing directory, we don't need to do
    // anything.
    if (is_dir(path)) {
        return;
    }

    // If the parent of this path doesn't exist yet, recursively try to make it
    // a directory first.
    auto parent = dir_name(path);
    if (parent != path && !path_exists(parent)) {
        make_dirs(parent);
    }

    // Try to make the given directory.
    if (mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
        throw Exception("failed to make directory \"" + path + "\"", true);
    }

}

/**
 * Tries to create a file (if it doesn't already exist) and opens it for
 * writing. If the directory that path is contained by does not exists, it is
 * first created.
 */
OutFile::OutFile(const Str &path) : ofs(), path(path) {

    // If the parent path does not exist yet, recursively try to create a
    // directory for it.
    auto parent = dir_name(path);
    if (parent != path && !path_exists(parent)) {
        make_dirs(parent);
    }

    // Open the file.
    ofs.open(path);
    check();

}

/**
 * Writes to the file.
 */
void OutFile::write(const Str &content) {
    ofs << content;
    check();
}

/**
 * Closes the file prior to destruction. This is not necessary for correct
 * filesystem behavior (the file is always closed on destruction), but allows
 * any exceptions from the close() syscall to be caught.
 */
void OutFile::close() {
    ofs.close();
    check();
}

/**
 * Throws an exception if badbit or failbit are set.
 */
void OutFile::check() {
    if (ofs.fail()) {
        throw Exception("failed to write file \"" + path + "\"", true);
    }
}

/**
 * Provides unchecked access to the underlying std::ofstream object.
 */
std::ofstream &OutFile::unwrap() {
    return ofs;
}

/**
 * Tries to open a file for reading.
 */
InFile::InFile(const Str &path) : ifs(), path(path) {
    ifs.open(path);
    check();
}

/**
 * Reads the entire (remainder of the) file to a string.
 */
Str InFile::read() {
    Str s{(std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>()};
    check();
    return s;
}

/**
 * Closes the file prior to destruction. This is not necessary for correct
 * filesystem behavior (the file is always closed on destruction), but allows
 * any exceptions from the close() syscall to be caught.
 */
void InFile::close() {
    ifs.close();
    check();
}

/**
 * Throws an exception if badbit or failbit are set.
 */
void InFile::check() {
    if (ifs.fail()) {
        throw Exception("failed to write file \"" + path + "\"", true);
    }
}

} // namespace utils
} // namespace ql
