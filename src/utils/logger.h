/** \file
 * Provides macros for logging and the global loglevel variable.
 */

#pragma once

#include <iostream>
#include "utils/exception.h"
#include "utils/compat.h"
#include "utils/str.h"

// helper macro: stringstream to string
// based on https://stackoverflow.com/questions/21924156/how-to-initialize-a-stdstringstream
#define QL_SS2S(values) ::ql::utils::Str(dynamic_cast<::ql::utils::StrStrm&&>(::ql::utils::StrStrm() << values).str())

#define QL_PRINTLN(x) \
    do {                                                                                                    \
        std::cout << "[OPENQL] " << x << std::endl;                                                         \
    } while (false)

#define QL_EOUT(content) \
    do {                                                                                                    \
        if (::ql::utils::logger::log_level >= ::ql::utils::logger::LogLevel::LOG_ERROR) {                   \
            ::std::cerr << "[OPENQL] " __FILE__ ":" << __LINE__ << " Error: " << content << ::std::endl;    \
        }                                                                                                   \
    } while (false)

#define QL_WOUT(content) \
    do {                                                                                                    \
        if (::ql::utils::logger::log_level >= ::ql::utils::logger::LogLevel::LOG_WARNING) {                 \
            ::std::cerr << "[OPENQL] " __FILE__ ":" << __LINE__ << " Warning: " << content << ::std::endl;  \
        }                                                                                                   \
    } while (false)

#define QL_IOUT(content) \
    do {                                                                                                    \
        if (::ql::utils::logger::log_level >= ::ql::utils::logger::LogLevel::LOG_INFO) {                    \
            ::std::cout << "[OPENQL] " __FILE__ ":" << __LINE__ << " Info: "<< content << ::std::endl;      \
        }                                                                                                   \
    } while (false)

#define QL_DOUT(content) \
    do {                                                                                                    \
        if (::ql::utils::logger::log_level >= ::ql::utils::logger::LogLevel::LOG_DEBUG) {                   \
            ::std::cout << "[OPENQL] " __FILE__ ":" << __LINE__ << " " << content << ::std::endl;           \
        }                                                                                                   \
    } while (false)

#define QL_COUT(content) \
    do {                                                                                                    \
        ::std::cout << "[OPENQL] " __FILE__ ":" << __LINE__ << " " << content << ::std::endl;               \
    } while (false)

#define QL_FATAL(content) \
    do {                                                                                                    \
        ::ql::utils::StrStrm fatal_ss{};                                                                    \
        fatal_ss << content;                                                                                \
        ::ql::utils::Str fatal_s = fatal_ss.str();                                                             \
        QL_EOUT(fatal_s);                                                                                   \
        throw ::ql::utils::Exception("Error : " + fatal_s);                                                 \
    } while (false)

#define QL_ASSERT(condition)                                                                                \
    do {                                                                                                    \
        if (!(condition)) {                                                                                 \
            QL_FATAL("assert " #condition " failed in file " __FILE__ " at line " << __LINE__);             \
        }                                                                                                   \
    } while (false)

namespace ql {
namespace utils {
namespace logger {

enum LogLevel {
    LOG_NOTHING,
    LOG_CRITICAL,
    LOG_ERROR,
    LOG_WARNING,
    LOG_INFO,
    LOG_DEBUG
};

QL_GLOBAL extern LogLevel log_level;

LogLevel log_level_from_string(const Str &level);
void set_log_level(const Str &level);

} // namespace logger
} // namespace utils
} // namespace ql
