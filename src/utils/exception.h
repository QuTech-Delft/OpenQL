/** \file
 * Provides the base exception class for OpenQL.
 */

#pragma once

#include <stdexcept>
#include "utils/str.h"

namespace ql {
namespace utils {

/**
 * Base exception class for internal OpenQL errors.
 *
 * When constructed, this exception class generates a traceback and adds this
 * to the exception message. This is not exactly performant, so don't use this
 * class in places where catching the exception is part of normal program flow;
 * the idea is to provide context when an uncaught exception reaches user code.
 */
class Exception : public std::runtime_error {
public:
    explicit Exception(
        const Str &msg,
        bool system = false
    ) noexcept;
};

/**
 * Exception class for containers.
 */
class ContainerException : public Exception {
public:

    /**
     * Forward all constructors to the parent type.
     */
    template<typename... Args>
    explicit ContainerException(Args&&... args) : Exception(std::forward<Args>(args)...) {
    }

};

/**
 * Base exception class for errors indicating incorrect usage of OpenQL.
 *
 * These differ from Exception in that they never include a stack trace.
 */
class UserError : public std::runtime_error {
public:
    explicit UserError(
        const Str &msg,
        bool system = false
    ) noexcept;
};

} // namespace utils
} // namespace ql
