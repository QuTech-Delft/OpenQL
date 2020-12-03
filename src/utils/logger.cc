/** \file
 * Provides macros for logging and the global loglevel variable.
 */

#include "utils/logger.h"
#include "utils/exception.h"

namespace ql {
namespace utils {
namespace logger {

/**
 * The current log level (verbosity).
 */
LogLevel log_level;

/**
 * Converts the string representation of a log level to a LogLevel enum variant.
 * Throws ql::exception if the string could not be converted.
 */
LogLevel log_level_from_string(const Str &level) {
    if (level == "LOG_NOTHING") {
        return LogLevel::LOG_NOTHING;
    } else if (level == "LOG_CRITICAL") {
        return LogLevel::LOG_CRITICAL;
    } else if (level == "LOG_ERROR") {
        return LogLevel::LOG_ERROR;
    } else if (level == "LOG_WARNING") {
        return LogLevel::LOG_WARNING;
    } else if (level == "LOG_INFO") {
        return LogLevel::LOG_INFO;
    } else if(level == "LOG_DEBUG") {
        return LogLevel::LOG_DEBUG;
    } else {
        throw Exception("unknown log level \"" + level + "\"");
    }
}

/**
 * Sets the current log level using its string representation.
 */
void set_log_level(const Str &level) {
    log_level = log_level_from_string(level);
}

} // namespace logger
} // namespace utils
} // namespace ql
