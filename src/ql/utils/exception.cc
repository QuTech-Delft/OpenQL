/** \file
 * Provides the base exception class for OpenQL.
 */

#include "ql/utils/exception.h"

#include <cstring>
#include <cerrno>
#include <iostream>
#include "backward.hpp"
#include "ql/utils/logger.h"

namespace ql {
namespace utils {

/**
 * Builds the message for Exception.
 */
static std::shared_ptr<backward::StackTrace> stack_trace() noexcept {

    // Try to capture a stack to add that to the message.
    try {

        // Capture a stack trace.
        auto trace = std::make_shared<backward::StackTrace>();
        trace->load_here(32);

        // Try to remove the part of the stack trace due to this file and
        // backward itself. But it's not particularly important, so ignore
        // exceptions that might occur and move on.
        try {
            static size_t skip = 0;
            if (!skip) {
                backward::TraceResolver tr;
                tr.load_stacktrace(trace);
                for (size_t i = 0; i < trace->size(); i++) {
                    auto fn = tr.resolve((*trace)[i]);
                    if (
                        (fn.source.filename.find("exception.cc") == std::string::npos)
                        && (fn.source.filename.find("backward.hpp") == std::string::npos)
                    ) {
                        skip = i;
                        break;
                    }
                }
            }
            trace->skip_n_firsts(skip);
        } catch (std::exception &e) {
            (void)e;
        }

        return trace;
    } catch (std::exception &e) {
        (void)e;
    }

    return {};
}

/**
 * String conversion for exception types.
 */
std::ostream &operator<<(std::ostream &os, ExceptionType etyp) {
    switch (etyp) {
        case ExceptionType::ICE:        return os << "Internal compiler error";
        case ExceptionType::ASSERT:     return os << "Assertion failure";
        case ExceptionType::CONTAINER:  return os << "Container error";
        case ExceptionType::SYSTEM:     return os << "OS error";
        case ExceptionType::USER:       return os << "Usage error";
        default:                        return os << "Unknown error";
    }
}

/**
 * Creates a new exception object with the given message. If system is set,
 * the standard library is queried for the string representation of the current
 * value of errno, and this is appended to the message. Finally, a backtrace is
 * performed and appended to the message after a newline.
 */
Exception::Exception(
    const std::string &msg,
    ExceptionType type
) noexcept : std::runtime_error(msg), messages({msg}), trace(stack_trace()), type(type) {
    if (type == ExceptionType::SYSTEM) {
        messages.emplace_back(std::strerror(errno));
    }
}

/**
 * Adds context to a message. The exception can also be promoted from a user
 * error to an ICE by setting ice to true, for when a function that might
 * also be used directly by a user is actually used by the compiler in a way
 * that should not fail.
 */
void Exception::add_context(const std::string &msg, bool ice) {
    messages.push_front(msg);
    if (ice && type == ExceptionType::USER) {
        type = ExceptionType::ICE;
    }
}

/**
 * Returns the complete exception message.
 */
const char *Exception::what() const noexcept {

    // Start with the exception type.
    utils::StrStrm ss;
    ss << type;

    // Append messages.
    for (const auto &msg : messages) {
        ss << ": " << msg;
    }

    // Append the stack trace to the message only if debug.
    if (type != ExceptionType::USER || QL_IS_LOG_DEBUG) {
        ss << "\n";
        backward::Printer p{};
        p.trace_context_size = 0;
        p.print(*trace, ss);

    }

    buf = ss.str();
    return buf.c_str();
}

} // namespace utils
} // namespace ql
