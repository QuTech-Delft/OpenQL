/** \file
 * Provides the base exception class for OpenQL.
 */

#pragma once

#include <stdexcept>
#include <list>
#include <string>
#include <memory>
#include <sstream>
#include <ostream>

namespace backward {
class StackTrace;
} // namespace backward

namespace ql {
namespace utils {

/**
 * Enumeration of exception types.
 */
enum class ExceptionType {

    /**
     * An internal compiler error, i.e. something that is always checked and
     * really shouldn't be going wrong.
     */
    ICE,

    /**
     * An assertion failure, i.e. something that should never be able to happen
     * as long as there is basic sanity.
     */
    ASSERT,

    /**
     * An container error, thrown for instance for out-of-range access or
     * dereferencing null. Should never happen.
     */
    CONTAINER,

    /**
     * An operating system error. Might be due to the user, or might be
     * something that should never happen but is outside of OpenQL's control.
     */
    SYSTEM,

    /**
     * An error caused by incorrect usage, for example because the user or a
     * pass is trying to make an instruction that the user didn't define in the
     * platform.
     */
    USER,

    /**
     * An unknown error. This class should not be used for new exceptions, and
     * should be phased out of existing code.
     */
    UNKNOWN

};

/**
 * String conversion for exception types.
 */
std::ostream &operator<<(std::ostream &os, ExceptionType etyp);

/**
 * Base exception class for internal OpenQL errors.
 */
class Exception : public std::runtime_error {
private:
    
    /**
     * Buffer for what() result.
     */
    mutable std::string buf;
    
    /**
     * List of messages. These are printed with ": " separator by what().
     */
    std::list<std::string> messages;

    /**
     * Stack trace information.
     */
    std::shared_ptr<backward::StackTrace> trace;

    /**
     * The type of exception. We use this instead of inheritance so we can
     * change it based on context.
     */
    ExceptionType type;

public:
    
    /**
     * Creates a new exception object with the given message. If system is set,
     * the standard library is queried for the string representation of the
     * current value of errno, and this is appended to the message. Finally, a
     * backtrace is performed and appended to the message after a newline.
     */
    explicit Exception(
        const std::string &msg,
        ExceptionType type = ExceptionType::UNKNOWN
    ) noexcept;

    /**
     * Adds context to a message. The exception can also be promoted from a user
     * error to an ICE by setting ice to true, for when a function that might
     * also be used directly by a user is actually used by the compiler in a way
     * that should not fail.
     */
    void add_context(
        const std::string &msg,
        bool ice = false
    );

    /**
     * Returns the complete exception message.
     */
    const char *what() const noexcept override;
    
};

/**
 * Shorthand for throwing an exception with the specified type and (stringstream
 * based) message.
 */
#define QL_THROW(type, msg)                                                                                 \
    do {                                                                                                    \
        ::std::stringstream _throw_ss;                                                                      \
        _throw_ss << msg;                                                                                   \
        throw ::ql::utils::Exception(_throw_ss.str(), ::ql::utils::ExceptionType::type);                    \
    } while (false)

/**
 * Shorthand for throwing an internal compiler error.
 */
#define QL_ICE(msg) QL_THROW(ICE, msg)

/**
 * Shorthand for throwing a container error.
 */
#define QL_CONTAINER_ERROR(msg) QL_THROW(CONTAINER, msg)

/**
 * Shorthand for throwing an operating system error. The OS error (derived from
 * errno) is added to the end implicitly.
 */
#define QL_SYSTEM_ERROR(msg) QL_THROW(SYSTEM, msg)

/**
 * Shorthand for throwing a user error.
 */
#define QL_USER_ERROR(msg) QL_THROW(USER, msg)

/**
 * Asserts that the given condition is true, throwing an assertion failure
 * exception if false.
 */
#define QL_ASSERT(condition)                                                                                \
    do {                                                                                                    \
        if (!(condition)) {                                                                                 \
            QL_THROW(ASSERT, "assert " #condition " failed in file " __FILE__ " at line " << __LINE__);     \
        }                                                                                                   \
    } while (false)

/**
 * Asserts that the given values are equal, throwing an assertion failure
 * exception if not.
 */
#define QL_ASSERT_EQ(a, b)                                                                                  \
    do {                                                                                                    \
        auto _a = (a);                                                                                      \
        auto _b = (b);                                                                                      \
        if (_a != _b) {                                                                                     \
            QL_THROW(ASSERT,                                                                                \
                "assert \"" << try_to_string(_a) << "\" (" #a ") == "                                       \
                "\"" << try_to_string(_b) << "\" (" #b ") "                                                 \
                "failed in file " __FILE__ " at line " << __LINE__                                          \
            );                                                                                              \
        }                                                                                                   \
    } while (false)

/**
 * Asserts that `code` throws an exception derived from std::exception, catching
 * it and throwing an assertion failure exception otherwise.
 */
#define QL_ASSERT_RAISES(code)                                                                              \
    do {                                                                                                    \
        auto ok = false;                                                                                    \
        try {                                                                                               \
            code;                                                                                           \
        } catch (const std::exception &e) {                                                                 \
            ok = true;                                                                                      \
        }                                                                                                   \
        if (!ok) {                                                                                          \
            QL_THROW(ASSERT, "no exception thrown in file " __FILE__ " at line " << __LINE__);              \
        }                                                                                                   \
    } while (false)


} // namespace utils
} // namespace ql
