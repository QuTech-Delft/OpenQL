/** \file
 * Provides the base exception class for OpenQL.
 */

#include "utils/exception.h"

#include <cstring>
#include <cerrno>
#include "backward.hpp"

namespace ql {
namespace utils {

/**
 * Builds the message for Exception.
 */
static Str make_message(
    const Str &msg,
    bool system = false
) noexcept {
    try {

        // Form the message string.
        StrStrm ss{};
        ss << msg;
        if (system) {
            ss << ": " << std::strerror(errno);
        }

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
                        (fn.source.filename.find("exception.cc") == Str::npos)
                        && (fn.source.filename.find("backward.hpp") == Str::npos)
                    ) {
                        trace.skip_n_firsts(i);
                        break;
                    }
                }
            } catch (std::exception &e) {
                (void)e;
            }

            // Append the stack trace to the message.
            ss << std::endl;
            backward::Printer p{};
            p.trace_context_size = 0;
            p.print(trace, ss);

        } catch (std::exception &e) {
            (void)e;
        }

        return ss.str();

    } catch (std::exception &e) {
        (void)e;

        // Don't abort if anything fails while forming the error message. This
        // fallback should only fail if the mere act of copying the msg string
        // is enough for things to die, at which point we've already lost.
        return msg;

    }
}

/**
 * Creates a new exception object with the given message. If system is set,
 * the standard library is queried for the string representation of the current
 * value of errno, and this is appended to the message. Finally, a backtrace is
 * performed and appended to the message after a newline.
 */
Exception::Exception(
    const Str &msg,
    bool system
) noexcept : std::runtime_error(make_message(msg, system)) {
}

/**
 * Creates a new exception object with the given message. If system is set,
 * the standard library is queried for the string representation of the current
 * value of errno, and this is appended to the message.
 */
UserError::UserError(
    const Str &msg,
    bool system
) noexcept : std::runtime_error(make_message(msg, system)) {
}

} // namespace utils
} // namespace ql
