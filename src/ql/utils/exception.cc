/** \file
 * Provides the base exception class for OpenQL.
 */

#include "ql/utils/exception.h"

#include <cstring>
#include <cerrno>
#include "backward.hpp"

namespace ql {
namespace utils {

/**
 * Builds the message for Exception.
 */
static std::string stack_trace() noexcept {

    // Try to capture a stack to add that to the message.
    try {

        // Capture a stack trace.
        backward::StackTrace trace;
        trace.load_here(32);

        // Try to remove the part of the stack trace due to this file and
        // backward itself. But it's not particularly important, so ignore
        // exceptions that might occur and move on.
        try {
            backward::TraceResolver tr;
            tr.load_stacktrace(trace);
            for (size_t i = 0; i < trace.size(); i++) {
                auto fn = tr.resolve(trace[i]);
                if (
                    (fn.source.filename.find("exception.cc") == std::string::npos)
                    && (fn.source.filename.find("backward.hpp") == std::string::npos)
                ) {
                    trace.skip_n_firsts(i);
                    break;
                }
            }
        } catch (std::exception &e) {
            (void)e;
        }

        // Append the stack trace to the message.
        backward::Printer p{};
        p.trace_context_size = 0;
        std::ostringstream ss{};
        p.print(trace, ss);
        return ss.str();

    } catch (std::exception &e) {
        (void)e;
    }

    return "";
}

/**
 * Creates a new exception object with the given message. If system is set,
 * the standard library is queried for the string representation of the current
 * value of errno, and this is appended to the message. Finally, a backtrace is
 * performed and appended to the message after a newline.
 */
Exception::Exception(
    const std::string &msg,
    bool system
) noexcept : std::runtime_error(msg), messages({msg}), trace(stack_trace()) {
    if (system) {
        messages.push_back(std::strerror(errno));
    }
}

/**
 * Returns the complete exception message.
 */
const char *Exception::what() const noexcept {
    buf.clear();
    bool first = true;
    for (const auto &msg : messages) {
        if (first) {
            first = false;
        } else {
            buf += ": ";
        }
        buf += msg;
    }
    if (!trace.empty()) {
        buf += "\n" + trace;
    }
    return buf.c_str();
}

/**
 * Base exception class for errors indicating incorrect usage of OpenQL.
 *
 * These differ from Exception in that they never include a stack trace.
 */
UserError::UserError(const std::string &msg) noexcept : std::runtime_error(msg) {
}

} // namespace utils
} // namespace ql
