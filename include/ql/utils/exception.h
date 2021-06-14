/** \file
 * Provides the base exception class for OpenQL.
 */

#pragma once

#include <stdexcept>
#include <list>

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
private:
    
    /**
     * Buffer for what() result.
     */
    mutable std::string buf;
    
public:
    
    /**
     * List of messages. These are printed with ": " separator by what().
     */
    std::list<std::string> messages;
    
    /**
     * Stack trace information.
     */
    std::string trace;
    
    /**
     * Constructs an exception.
     */
    explicit Exception(
        const std::string &msg,
        bool system = false
    ) noexcept;
    
    /**
     * Returns the complete exception message.
     */
    const char *what() const noexcept override;
    
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
    explicit UserError(const std::string &msg) noexcept;
};

} // namespace utils
} // namespace ql
