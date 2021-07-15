/** \file
 * Provides platform-agnostic filesystem utilities.
 */

#include "ql/utils/filesystem.h"

#include <iostream>
#include <fstream>
#include <cerrno>
#include <algorithm>
#include <cctype>

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

namespace {

/**
 * Stack of working directories. Private; use push_working_directory(),
 * pop_working_directory(), and get_working_directory() to access.
 */
List<Str> working_directory_stack;

} // anonymous namespace

/**
 * Makes a path that looks like a relative path relative to the current OpenQL
 * working directory.
 */
static Str process_path(const Str &path) {
    return path_relative_to(get_working_directory(), path);
}

/**
 * Sets OpenQL's working directory to the given directory. If the directory
 * looks like a relative path, it is appended to the previous working directory.
 * Otherwise it overrides the working directory.
 */
void push_working_directory(const Str &dir) {
    working_directory_stack.push_back(process_path(dir));
}

/**
 * Reverts the change made by the previous push_working_directory() call.
 */
void pop_working_directory() {
    working_directory_stack.pop_back();
}

/**
 * Returns OpenQL's current working directory. If no working directory has been
 * set yet, `.` is returned, so the OS working directory is effectively used
 * instead.
 */
Str get_working_directory() {
    if (working_directory_stack.empty()) {
        return ".";
    } else {
        return working_directory_stack.back();
    }
}

/**
 * Returns whether the given path exists and is a directory. Does not apply
 * process_path() to the path first.
 */
static Bool is_dir_raw(const Str &path) {
    struct stat info{};
    if (stat(path.c_str(), &info) != 0) {
        return false;
    }
    return (info.st_mode & S_IFDIR) != 0;
}

/**
 * Returns whether the given path exists and is a directory. If path looks like
 * a relative path, it is interpreted as relative to the current OpenQL working
 * directory.
 */
Bool is_dir(const Str &path) {
    return is_dir_raw(process_path(path));
}

/**
 * Returns whether the given path exists and is a regular file. If path looks
 * like a relative path, it is interpreted as relative to the current OpenQL
 * working directory.
 */
Bool is_file(const Str &path) {
    auto processed_path = process_path(path);
    struct stat info{};
    if (stat(processed_path.c_str(), &info) != 0) {
        return false;
    }
    return (info.st_mode & S_IFREG) != 0;
}

/**
 * Returns whether the given path exists. Does not apply process_path() to the
 * path first.
 */
static Bool path_exists_raw(const Str &path) {
    struct stat info{};
    return !stat(path.c_str(), &info);
}

/**
 * Returns whether the given path exists. If path looks like a relative path, it
 * is interpreted as relative to the current OpenQL working directory.
 */
Bool path_exists(const Str &path) {
    return path_exists_raw(process_path(path));
}

/**
 * If path looks like it's a relative path, make it relative to base instead.
 * If path looks like it's absolute, return it unchanged.
 */
Str path_relative_to(const Str &base, const Str &path) {

    // Detect POSIX absolute paths.
    if (path.size() >= 1 && path.front() == '/') return path;

    // Detect Windows absolute paths.
    if (path.size() >= 2 && isalpha(path[0]) && path[1] == ':') return path;

    // Looks relative to me.
    return base + "/" + path;

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
 * an Exception if creation of the directory fails. Does not apply
 * process_path() to the path first.
 */
void make_dirs_raw(const Str &path) {

    // If the given path is already an existing directory, we don't need to do
    // anything.
    if (is_dir_raw(path)) {
        return;
    }

    // If the parent of this path doesn't exist yet, recursively try to make it
    // a directory first.
    auto parent = dir_name(path);
    if (parent != path && !path_exists(parent)) {
        make_dirs_raw(parent);
    }

    // Try to make the given directory.
    if (mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
        QL_SYSTEM_ERROR("failed to make directory \"" << path << "\"");
    }

}

/**
 * (Recursively) creates a new directory if it does not already exist. Throws
 * an Exception if creation of the directory fails. If path looks like a
 * relative path, it is interpreted as relative to the current OpenQL working
 * directory.
 */
void make_dirs(const Str &path) {
    make_dirs_raw(process_path(path));
}

/**
 * Tries to create a file (if it doesn't already exist) and opens it for
 * writing. If the directory that path is contained by does not exists, it is
 * first created.
 */
OutFile::OutFile(const Str &path) : ofs(), path(path) {
    auto processed_path = process_path(path);

    // If the parent path does not exist yet, recursively try to create a
    // directory for it.
    auto parent = dir_name(processed_path);
    if (parent != processed_path && !path_exists_raw(parent)) {
        make_dirs_raw(parent);
    }

    // Open the file.
    ofs.open(processed_path);
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
        QL_SYSTEM_ERROR("failed to write file \"" << path << "\"");
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
    ifs.open(process_path(path));
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
        QL_SYSTEM_ERROR("failed to read file \"" << path << "\"");
    }
}

} // namespace utils
} // namespace ql
