#pragma once

#include <iostream>
#include <sstream>
#include "utils/compat.h"

// helper macro: stringstream to string
// based on https://stackoverflow.com/questions/21924156/how-to-initialize-a-stdstringstream
#define SS2S(values) std::string(static_cast<std::ostringstream&&>(std::ostringstream() << values).str())

#define PRINTLN(x) \
    do {                                                                                                    \
        std::cout << "[OPENQL] " << x << std::endl;                                                         \
    } while (false)

#define EOUT(content) \
    do {                                                                                                    \
        if (::ql::utils::logger::LOG_LEVEL >= ::ql::utils::logger::log_level_t::LOG_ERROR) {                \
            ::std::cerr << "[OPENQL] " __FILE__ ":" << __LINE__ << " Error: " << content << ::std::endl;    \
        }                                                                                                   \
    } while (false)

#define WOUT(content) \
    do {                                                                                                    \
        if (::ql::utils::logger::LOG_LEVEL >= ::ql::utils::logger::log_level_t::LOG_WARNING) {              \
            ::std::cerr << "[OPENQL] " __FILE__ ":" << __LINE__ << " Warning: " << content << ::std::endl;  \
        }                                                                                                   \
    } while (false)

#define IOUT(content) \
    do {                                                                                                    \
        if (::ql::utils::logger::LOG_LEVEL >= ::ql::utils::logger::log_level_t::LOG_INFO) {                 \
            ::std::cout << "[OPENQL] " __FILE__ ":" << __LINE__ << " Info: "<< content << ::std::endl;      \
        }                                                                                                   \
    } while (false)

#define DOUT(content) \
    do {                                                                                                    \
        if (::ql::utils::logger::LOG_LEVEL >= ::ql::utils::logger::log_level_t::LOG_DEBUG) {                \
            ::std::cout << "[OPENQL] " __FILE__ ":" << __LINE__ << " " << content << ::std::endl;           \
        }                                                                                                   \
    } while (false)

#define COUT(content) \
    do {                                                                                                    \
        ::std::cout << "[OPENQL] " __FILE__ ":" << __LINE__ << " " << content << ::std::endl;               \
    } while (false)

#define FATAL(content) \
    do {                                                                                                    \
        ::std::ostringstream fatal_ss{};                                                                    \
        fatal_ss << content;                                                                                \
        ::std::string fatal_s = fatal_ss.str();                                                             \
        EOUT(fatal_s);                                                                                      \
        throw ql::exception("Error : " + fatal_s, false);                                                   \
    } while (false)

#define ASSERT(condition)                                                                                   \
    do {                                                                                                    \
        if (!(condition)) {                                                                                 \
            FATAL("assert " #condition " failed in file " __FILE__ " at line " << __LINE__);                \
        }                                                                                                   \
    } while (false)

namespace ql {
namespace utils {
namespace logger {

enum log_level_t {
    LOG_NOTHING,
    LOG_CRITICAL,
    LOG_ERROR,
    LOG_WARNING,
    LOG_INFO,
    LOG_DEBUG
};

OPENQL_DECLSPEC extern log_level_t LOG_LEVEL;

void set_log_level(const std::string &level);

} // namespace logger
} // namespace utils
} // namespace ql
