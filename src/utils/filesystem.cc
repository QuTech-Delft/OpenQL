/** \file
 * Provides platform-agnostic filesystem utilities.
 */

#include "utils/filesystem.h"

#include <iostream>
#include <fstream>
#include <cerrno>
#include "utils/exception.h"

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
bool is_dir(const std::string &path) {
    struct stat info{};
    if (stat(path.c_str(), &info) != 0) {
        return false;
    }
    return (info.st_mode & S_IFDIR) != 0;
}

/**
 * Returns whether the given path exists and is a regular file.
 */
bool is_file(const std::string &path) {
    struct stat info{};
    if (stat(path.c_str(), &info) != 0) {
        return false;
    }
    return (info.st_mode & S_IFREG) != 0;
}

/**
 * Returns whether the given path exists.
 */
bool path_exists(const std::string &path) {
    struct stat info{};
    return !stat(path.c_str(), &info);
}

/**
 * Returns the directory of the given path. On Linux and MacOS, this just maps
 * to dirname() from libgen.h. On Windows, the string is stripped from the last
 * backslash or slash onward, if any.
 */
std::string dir_name(const std::string &path) {
#ifdef _WIN32
    auto last_slash = path.find_last_of('/');
    if (last_slash == std::string::npos) {
        last_slash = 0;
    }
    auto last_backslash = path.find_last_of('\\');
    if (last_backslash == std::string::npos) {
        last_backslash = 0;
    }
    auto new_length = std::max(last_slash, last_backslash);
    if (!new_length) {
        return path;
    }
    return path.substr(0, new_length);
#else
    std::string path_copy{path};
    return std::string(dirname(&path_copy[0]));
#endif
}

/**
 * (Recursively) creates a new directory if it does not already exist. Throws
 * an Exception if creation of the directory fails.
 */
void make_dirs(const std::string &path) {

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
 * Writes the given string to a file, creating a new file if it didn't already
 * exist, or otherwise overwriting the existing file. If the parent directory of
 * the specified path does not exist, it is first created. Throws an Exception
 * if writing the file fails.
 */
void write_file(const std::string &path, const std::string &content) {

    // If the parent path does not exist yet, recursively try to create a
    // directory for it.
    auto parent = dir_name(path);
    if (parent != path && !path_exists(parent)) {
        make_dirs(parent);
    }

    // Try to write the file.
    try {
        std::ofstream file;
        file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        file.open(path);
        file << content;
        file.close();
    } catch (std::ios_base::failure &e) {
        throw Exception("failed to write file \"" + path + "\": " + e.what(), true);
    }

}

} // namespace utils
} // namespace ql
