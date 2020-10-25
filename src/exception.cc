/**
 * @file    exception.cc
 * @author  Nader KHAMMASSI
 * @contact nader.khammassi@gmail.com
 * @date    15/03/2010
 */

#include "exception.h"

namespace ql {

exception::exception(
    const std::string &message,
    bool system_message
) noexcept : user_message(message) {
    if (system_message) {
        user_message.append(": ");
        user_message.append(strerror(errno));
    }
}

/**
 * explanatory message
 */
const char *exception::what() const noexcept {
    return user_message.c_str();
}

} // namespace ql
