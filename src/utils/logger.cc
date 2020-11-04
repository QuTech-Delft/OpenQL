#include "utils/logger.h"

namespace ql {
namespace utils {
namespace logger {

log_level_t LOG_LEVEL;

void set_log_level(const std::string &level) {
    if (level == "LOG_NOTHING") {
        ql::utils::logger::LOG_LEVEL = ql::utils::logger::log_level_t::LOG_NOTHING;
    } else if (level == "LOG_CRITICAL") {
        ql::utils::logger::LOG_LEVEL = ql::utils::logger::log_level_t::LOG_CRITICAL;
    } else if (level == "LOG_ERROR") {
        ql::utils::logger::LOG_LEVEL = ql::utils::logger::log_level_t::LOG_ERROR;
    } else if (level == "LOG_WARNING") {
        ql::utils::logger::LOG_LEVEL = ql::utils::logger::log_level_t::LOG_WARNING;
    } else if (level == "LOG_INFO") {
        ql::utils::logger::LOG_LEVEL = ql::utils::logger::log_level_t::LOG_INFO;
    } else if(level == "LOG_DEBUG") {
        ql::utils::logger::LOG_LEVEL = ql::utils::logger::log_level_t::LOG_DEBUG;
    } else {
        std::cerr << "[OPENQL] " << __FILE__ << ":" << __LINE__
                  << " Error: Unknown log level" << std::endl;
    }
}

} // namespace logger
} // namespace utils
} // namespace ql
