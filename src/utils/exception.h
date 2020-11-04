/** \file
 * Provides the base exception class for OpenQL.
 */

#pragma once

#include <string>
#include <stdexcept>

namespace ql {
namespace utils {

/**
 * Base exception class for OpenQL.
 *
 * When constructed, this exception class generates a traceback and adds this
 * to the exception message. This is not exactly performant, so don't use this
 * class in places where catching the exception is part of normal program flow;
 * the idea is to provide context when an uncaught exception reaches user code.
 */
class Exception : public std::runtime_error {
public:
    explicit Exception(
        const std::string &msg,
        bool system = false
    ) noexcept;
};

} // namespace utils
} // namespace ql
